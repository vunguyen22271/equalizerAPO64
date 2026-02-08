/*
    This file is part of Equalizer APO, a system-wide equalizer.
    Copyright (C) 2017  Jonas Thedering

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <QFileInfo>
#include <QFileDialog>
#include <QSettings>
#include <QAbstractEventDispatcher>
#include <QStringList>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "helpers/aeffectx.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "Editor/helpers/GUIHelper.h"
#include "Editor/MainWindow.h"
#include "VSTPluginFilterGUIDialog.h"
#include "VSTPluginFilterGUI.h"
#include "ui_VSTPluginFilterGUI.h"

using namespace std;
using namespace std::placeholders;

VSTPluginFilterGUI::VSTPluginFilterGUI(std::shared_ptr<VSTPluginLibrary> library, const std::wstring& chunkData, const std::unordered_map<std::wstring, float>& paramMap)
	: ui(new Ui::VSTPluginFilterGUI), library(library), chunkData(chunkData), paramMap(paramMap)
{
	ui->setupUi(this);
	ui->frame->setVisible(false);
	updatePermissionWarning();

	LogF(L"VSTPluginFilterGUI: Constructor called");

	loopback = new WASAPILoopback();
	loopback->start([this](const std::vector<float>& samples, int channelCount, int sampleRate) {
		std::lock_guard<std::mutex> lock(audioMutex);

		if (!effect || (!embedded && !dialogOpen))
			return;

		// Initialize or Re-initialize if configuration changes
		if (this->currentSampleRate != sampleRate)
		{
			LogF(L"VSTPluginFilterGUI: Audio config change. Rate: %d (Old: %.0f)", sampleRate, this->currentSampleRate);
			
			effect->stopProcessing();
			
			this->currentSampleRate = (float)sampleRate;
			int blockSize = 512;
			
			// Prepare lists of pointers
			int numInputs = effect->numInputs();
			int numOutputs = effect->numOutputs();
			
			// Cap inputs to reasonable number to prevent massive allocation or issues with some plugins
			if (numInputs > 32) numInputs = 32;
			if (numOutputs > 32) numOutputs = 32;

			LogF(L"VSTPluginFilterGUI: Config. Inputs: %d, Outputs: %d, Block: %d", numInputs, numOutputs, blockSize);

			inputPtrs.resize(numInputs);
			outputPtrs.resize(numOutputs);
			outputBuffer.resize(blockSize * numOutputs); // Dummy output buffer

			// Reset buffers
			inputBuffer.clear();
			
			effect->prepareForProcessing(this->currentSampleRate, blockSize);
			effect->startProcessing();
		}

		// Append new samples
		inputBuffer.insert(inputBuffer.end(), samples.begin(), samples.end());

		int blockSize = 512;
		size_t neededSamples = blockSize * channelCount;

		// Process in fixed-size chunks
		while (inputBuffer.size() >= neededSamples)
		{
			int numInputs = effect->numInputs();
			int numOutputs = effect->numOutputs();
			
			if (numInputs > 32) numInputs = 32;
			if (numOutputs > 32) numOutputs = 32;

			// We need a deinterleaved scratch buffer for the inputs
			// Let's use a static vector or member to avoid realloc
			static std::vector<float> deinterleavedInput; 
			if (deinterleavedInput.size() < blockSize * numInputs)
				deinterleavedInput.resize(blockSize * numInputs);

			// Deinterleave and map channels
			for (int ch = 0; ch < numInputs; ch++)
			{
				inputPtrs[ch] = &deinterleavedInput[ch * blockSize];
				
				// If we have source data for this channel
				if (ch < channelCount)
				{
					for (int i = 0; i < blockSize; i++)
					{
						deinterleavedInput[ch * blockSize + i] = inputBuffer[i * channelCount + ch];
					}
				}
				else
				{
					// Silence for unconnected inputs
					memset(inputPtrs[ch], 0, blockSize * sizeof(float));
				}
			}

			// Setup output pointers
			for (int ch = 0; ch < numOutputs; ch++)
			{
				outputPtrs[ch] = &outputBuffer[ch * blockSize];
			}

			// Process
			effect->processReplacing(inputPtrs.data(), outputPtrs.data(), blockSize);

			// Remove used samples
			// Optimization: could just move an index pointer, but erase is safer for logic
			inputBuffer.erase(inputBuffer.begin(), inputBuffer.begin() + neededSamples);
		}
	});

	QString absolutePath = QString::fromStdWString(library->getLibPath());
	QDir pluginsDir(QString::fromStdWString(VSTPluginLibrary::getDefaultPluginPath()));
	QString relativePath = QDir::toNativeSeparators(pluginsDir.relativeFilePath(absolutePath));
	if (relativePath.startsWith(QDir::toNativeSeparators("../../")))
		relativePath = absolutePath;
	ui->pathLineEdit->setText(relativePath);

	QMenu* menu = new QMenu(ui->optionsButton);
	menu->addAction(ui->embedAction);
	ui->optionsButton->setMenu(menu);
}

VSTPluginFilterGUI::~VSTPluginFilterGUI()
{
	delete loopback;

	if (effect != NULL)
	{
		if (embedded)
			on_embedAction_toggled(false);
		delete effect;
		effect = NULL;
	}

	delete ui;
}

void VSTPluginFilterGUI::store(QString& command, QString& parameters)
{
	command = "VSTPlugin";

	QString absolutePath = QString::fromStdWString(library->getLibPath());
	QDir pluginsDir(QString::fromStdWString(VSTPluginLibrary::getDefaultPluginPath()));
	QString relativePath = QDir::toNativeSeparators(pluginsDir.relativeFilePath(absolutePath));
	if (relativePath.startsWith(QDir::toNativeSeparators("../../")))
		relativePath = absolutePath;

	if (relativePath.contains(" "))
		relativePath = "\"" + relativePath + "\"";
	parameters = "Library " + relativePath;

	if (chunkData != L"")
	{
		parameters += " ChunkData \"" + QString::fromStdWString(chunkData) + "\"";
	}
	else
	{
		for (auto it : paramMap)
		{
			QString name = QString::fromStdWString(it.first);
			if (name.contains(" ") || name.contains("\""))
				name = "\"" + name.replace("\"", "\"\"") + "\"";
			parameters += " " + name + " " + QString("%1").arg(it.second);
		}
	}
}

void VSTPluginFilterGUI::loadPreferences(const QVariantMap& prefs)
{
	autoApplyDialog = prefs.value("autoApplyDialog").toBool();

	if (prefs.value("embed").toBool())
		// will also call initPlugin
		ui->embedAction->setChecked(true);
	else
		initPlugin();
}

void VSTPluginFilterGUI::storePreferences(QVariantMap& prefs)
{
	prefs.insert("embed", ui->embedAction->isChecked());
	prefs.insert("autoApplyDialog", autoApplyDialog);
}

void VSTPluginFilterGUI::on_openPanelButton_clicked()
{
	initPlugin();

	if (effect != NULL)
	{
		effect->writeToEffect(chunkData, paramMap);

		VSTPluginFilterGUIDialog dialog(this, effect, autoApplyDialog);
		connect(dialog.getApplyButton(), SIGNAL(pressed()), SLOT(applyDialog()));
		connect(dialog.getAutoApplyCheckBox(), SIGNAL(toggled(bool)), SLOT(autoApplyToggled(bool)));
		connect(QAbstractEventDispatcher::instance(), SIGNAL(aboutToBlock()), SLOT(on_idle()));

		dialogOpen = true;
		if (dialog.exec() == QDialog::Accepted)
		{
			effect->readFromEffect(chunkData, paramMap);
			updateModel();
			updatePermissionWarning();
		}
		dialogOpen = false;
		disconnect(QAbstractEventDispatcher::instance(), SIGNAL(aboutToBlock()), this, SLOT(on_idle()));
	}
}

void VSTPluginFilterGUI::applyDialog()
{
	effect->readFromEffect(chunkData, paramMap);
	updateModel();
	updatePermissionWarning();
}

void VSTPluginFilterGUI::autoApplyToggled(bool checked)
{
	autoApplyDialog = checked;
}

void VSTPluginFilterGUI::initPlugin()
{
	if (effect != NULL)
		return;

	QColor color;
	QString text;
	if (library->getLibPath() == L"")
	{
		text = tr("No file selected.");
		color = Qt::red;
	}
	else
	{
		int result = library->initialize();
		if (result < 0)
		{
			color = Qt::red;

			switch (result)
			{
			case AbstractLibrary::FILE_NOT_FOUND:
				text = tr("File not found.");
				break;
			case AbstractLibrary::LOADING_FAILED:
				text = tr("Library could not be loaded.");
				break;
			case AbstractLibrary::FUNCTIONS_MISSING:
				text = tr("Library does not contain needed functions.");
				break;
			case AbstractLibrary::WRONG_ARCHITECTURE:
#ifdef _WIN64
				int bitDepth = 64;
#else
				int bitDepth = 32;
#endif
				text = tr("Library has the wrong architecture. Only %1-bit libraries are supported.").arg(bitDepth);
				break;
			}
		}
		else
		{
			VSTPluginInstance* newEffect = new VSTPluginInstance(library, 1);
			if (newEffect->initialize())
			{
				{
					std::lock_guard<std::mutex> lock(audioMutex);
					effect = newEffect;
				}
				effect->setLanguage(QLocale().language() == QLocale::German ? 2 : 1);
				effect->setAutomateFunc(bind(&VSTPluginFilterGUI::onAutomate, this));

				color = Qt::black;
				text = QString::fromStdWString(effect->getName());
			}
			else
			{
				delete effect;
				effect = NULL;

				color = Qt::red;
				text = tr("Plugin crashed during initialization.");
			}
		}
	}

	QPalette palette = ui->statusLabel->palette();
	palette.setColor(QPalette::Active, QPalette::WindowText, color);
	palette.setColor(QPalette::Inactive, QPalette::WindowText, color);
	ui->statusLabel->setPalette(palette);
	ui->statusLabel->setText(text);
}

void VSTPluginFilterGUI::on_pathLineEdit_editingFinished()
{
	if (QString::fromStdWString(library->getLibPath()) != ui->pathLineEdit->text())
	{
		int oldId = 0;
		if (effect != NULL)
		{
			oldId = effect->uniqueID();
			if (ui->embedAction->isChecked())
				on_embedAction_toggled(false);
			
			std::lock_guard<std::mutex> lock(audioMutex);
			delete effect;
			effect = NULL;
		}

		QDir pluginsDir(QString::fromStdWString(VSTPluginLibrary::getDefaultPluginPath()));
		QString path = ui->pathLineEdit->text();
		if (path.length() > 0)
			path = QDir::toNativeSeparators(QFileInfo(pluginsDir, ui->pathLineEdit->text()).absoluteFilePath());
		library = VSTPluginLibrary::getInstance(path.toStdWString());
		initPlugin();

		if (effect == NULL || oldId == 0 || effect->uniqueID() != oldId)
		{
			chunkData = L"";
			paramMap.clear();
		}

		updateModel();
		updatePermissionWarning();

		if (ui->embedAction->isChecked())
			on_embedAction_toggled(true);
	}
}

void VSTPluginFilterGUI::on_selectButton_clicked()
{
	QDir pluginsDir(QString::fromStdWString(VSTPluginLibrary::getDefaultPluginPath()));

	QSettings settings(QString::fromWCharArray(EDITOR_REGPATH), QSettings::NativeFormat);
	QString lastDir = settings.value("vst/lastDir", "").toString();
	if (lastDir == "")
		lastDir = pluginsDir.absolutePath();

	QFileInfo fileInfo(lastDir);
	QString path = ui->pathLineEdit->text();
	if (path.length() > 0)
		fileInfo.setFile(pluginsDir, path);

	QFileDialog dialog(this, tr("Select VST plugin"), fileInfo.absoluteFilePath(), "*.dll");
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(tr("VST plugins (*.dll)"));
	if (path.length() > 0)
		dialog.selectFile(fileInfo.fileName());
	if (dialog.exec() == QDialog::Accepted)
	{
		QString absolutePath = dialog.selectedFiles().first();
		settings.setValue("vst/lastDir", QDir::toNativeSeparators(QFileInfo(absolutePath).absolutePath()));
		QString relativePath = pluginsDir.relativeFilePath(absolutePath);
		if (relativePath.startsWith("../../"))
			relativePath = absolutePath;
		ui->pathLineEdit->setText(QDir::toNativeSeparators(relativePath));
		on_pathLineEdit_editingFinished();
	}
}

void VSTPluginFilterGUI::on_embedAction_toggled(bool checked)
{
	initPlugin();

	ui->openPanelButton->setVisible(!checked);

	bool enable = checked;
	if (effect == NULL)
		enable = false;

	if (enable != embedded)
	{
		embedded = enable;
		ui->frame->setVisible(enable);
		ui->statusLabel->setVisible(!enable);

		if (enable)
		{
			if (embedPlugin())
			{
				effect->setSizeWindowFunc(bind(&VSTPluginFilterGUI::onSizeWindow, this, _1, _2));
				connect(QAbstractEventDispatcher::instance(), SIGNAL(aboutToBlock()), SLOT(on_idle()));
			}
			else
			{
				embedded = false;
				ui->frame->setVisible(false);
				ui->statusLabel->setVisible(true);

				QPalette palette = ui->statusLabel->palette();
				palette.setColor(QPalette::Active, QPalette::WindowText, Qt::red);
				palette.setColor(QPalette::Inactive, QPalette::WindowText, Qt::red);
				ui->statusLabel->setPalette(palette);
				ui->statusLabel->setText(tr("Plugin crashed when opening panel."));
			}
		}
		else
		{
			if (effect != NULL)
			{
				effect->stopEditing();
				effect->setSizeWindowFunc(nullptr);
			}

			disconnect(QAbstractEventDispatcher::instance(), SIGNAL(aboutToBlock()), this, SLOT(on_idle()));
		}
	}
}

void VSTPluginFilterGUI::on_idle()
{
	if (effect != NULL)
	{
		effect->doIdle();

		if (embedded || autoApplyDialog)
		{
			if (!lastReadTimer.isValid() || lastReadTimer.elapsed() > 1000)
			{
				wstring newChunkData;
				unordered_map<std::wstring, float> newParamMap;
				effect->readFromEffect(newChunkData, newParamMap);
				if (newChunkData != chunkData || newParamMap != paramMap)
				{
					chunkData = newChunkData;
					paramMap = newParamMap;
					updateModel();
					updatePermissionWarning();
				}
				lastReadTimer.restart();
			}
		}
	}
}

void VSTPluginFilterGUI::onAutomate()
{
	if (embedded || autoApplyDialog)
	{
		effect->readFromEffect(chunkData, paramMap);
		updateModel();
		updatePermissionWarning();
	}
}

void VSTPluginFilterGUI::onSizeWindow(int w, int h)
{
	if (embedded)
		ui->frame->setFixedSize(w, h);
}

bool VSTPluginFilterGUI::embedPlugin()
{
	bool result = true;

	__try
	{
		effect->writeToEffect(chunkData, paramMap);

		HWND hwnd = (HWND)ui->frame->winId();
		short width, height;

		effect->startEditing(hwnd, &width, &height);

		ui->frame->setFixedSize(width, height);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		result = false;
	}

	return result;
}

void VSTPluginFilterGUI::updatePermissionWarning()
{
	if (effect == NULL)
	{
		ui->warningTextEdit->setVisible(false);
		return;
	}

	ACCESS_MASK mask = GENERIC_READ;
	try
	{
		mask = RegistryHelper::getFileAccessForUser(library->getLibPath(), SECURITY_LOCAL_SERVICE_RID);
	}
	catch (RegistryException e)
	{
		// ignore
	}

	if ((mask & GENERIC_READ) != GENERIC_READ && (mask & FILE_GENERIC_READ) != FILE_GENERIC_READ)
	{
		QString text = tr("The library is not readable by the audio service.\nChange the file permissions or copy the file to the VSTPlugins directory.");

		ui->warningTextEdit->setPlainText(text);
		QSize textSize = ui->warningTextEdit->fontMetrics().size(0, text);
		ui->warningTextEdit->setFixedSize(textSize + GUIHelper::scale(QSize(40, 15)));
		ui->warningTextEdit->setVisible(true);
		return;
	}

	QStringList files;
	if (chunkData != L"" && chunkData.length() < 100000)
	{
		QByteArray bytes = QByteArray::fromBase64(QString::fromStdWString(chunkData).toUtf8());
		QString string = QString::fromUtf8(bytes.data(), bytes.length());
		QRegularExpression regexp("[A-Za-z]:(?:\\\\[\\w \\(\\)-]+)+\\.[A-Za-z]{3}");
		QRegularExpressionMatchIterator it = regexp.globalMatch(string);
		while (it.hasNext())
		{
			QRegularExpressionMatch m = it.next();
			QString path = m.captured();
			QFile file(path);
			if (file.exists())
			{
				ACCESS_MASK mask = GENERIC_READ;
				try
				{
					mask = RegistryHelper::getFileAccessForUser(path.toStdWString(), SECURITY_LOCAL_SERVICE_RID);
				}
				catch (RegistryException e)
				{
					// ignore
				}

				if ((mask & GENERIC_READ) != GENERIC_READ && (mask & FILE_GENERIC_READ) != FILE_GENERIC_READ)
					files.append(path);
			}
		}
	}

	if (files.isEmpty())
	{
		ui->warningTextEdit->setVisible(false);
		ui->warningTextEdit->setPlainText("");
	}
	else
	{
		files.removeDuplicates();
		QString text = tr("The plugin seemingly accesses these files not readable by the audio service:\n"
				"%0\n"
				"Change the file permissions or copy the files to the config directory.").arg(files.join("\n"));
		ui->warningTextEdit->setPlainText(text);
		QSize textSize = ui->warningTextEdit->fontMetrics().size(0, text);
		ui->warningTextEdit->setFixedSize(textSize + GUIHelper::scale(QSize(40, 15)));
		ui->warningTextEdit->setVisible(true);
	}
}
