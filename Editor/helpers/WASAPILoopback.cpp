#include "WASAPILoopback.h"
#include "helpers/LogHelper.h"
#include <chrono>

#pragma comment(lib, "ole32.lib")

using namespace std;

WASAPILoopback::WASAPILoopback() : running(false)
{
}

WASAPILoopback::~WASAPILoopback()
{
	stop();
}

bool WASAPILoopback::start(AudioCallback callback)
{
	if (running)
		return true;

	this->callback = callback;
	running = true;
	captureThread = std::thread(&WASAPILoopback::captureThreadFunc, this);
	return true;
}

void WASAPILoopback::stop()
{
	running = false;
	if (captureThread.joinable())
		captureThread.join();
}

void WASAPILoopback::captureThreadFunc()
{
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) return;

	while (running)
	{
		IMMDeviceEnumerator* enumerator = nullptr;
		IMMDevice* device = nullptr;
		IAudioClient* audioClient = nullptr;
		IAudioCaptureClient* captureClient = nullptr;
		WAVEFORMATEX* mixFormat = nullptr;

		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
		if (FAILED(hr))
		{
			LogF(L"WASAPILoopback: Failed to create MMDeviceEnumerator: 0x%08X", hr);
			if (running) std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
		if (FAILED(hr))
		{
			// This often fails if no device is connected
			enumerator->Release();
			if (running) std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&audioClient);
		if (FAILED(hr))
		{
			LogF(L"WASAPILoopback: Failed to activate audio client: 0x%08X", hr);
			device->Release();
			enumerator->Release();
			if (running) std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		hr = audioClient->GetMixFormat(&mixFormat);
		if (FAILED(hr))
		{
			audioClient->Release();
			device->Release();
			enumerator->Release();
			if (running) std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, mixFormat, 0);
		if (FAILED(hr))
		{
			LogF(L"WASAPILoopback: Failed to initialize audio client: 0x%08X", hr);
			CoTaskMemFree(mixFormat);
			audioClient->Release();
			device->Release();
			enumerator->Release();
			if (running) std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		hr = audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient);
		if (FAILED(hr))
		{
			CoTaskMemFree(mixFormat);
			audioClient->Release();
			device->Release();
			enumerator->Release();
			if (running) std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		hr = audioClient->Start();
		if (FAILED(hr))
		{
			LogF(L"WASAPILoopback: Failed to start audio client: 0x%08X", hr);
			captureClient->Release();
			CoTaskMemFree(mixFormat);
			audioClient->Release();
			device->Release();
			enumerator->Release();
			if (running) std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}
		
		LogF(L"WASAPILoopback: Started capture. Channels: %d, Rate: %d", mixFormat->nChannels, mixFormat->nSamplesPerSec);

		LPWSTR currentDeviceId = nullptr;
		device->GetId(&currentDeviceId);

		int channelCount = mixFormat->nChannels;
		int sampleRate = mixFormat->nSamplesPerSec;
		bool firstPacket = true;
		auto lastDeviceCheck = std::chrono::steady_clock::now();

		while (running)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			// Periodically check if the default device has changed
			auto now = std::chrono::steady_clock::now();
			if (now - lastDeviceCheck > std::chrono::seconds(1))
			{
				lastDeviceCheck = now;
				IMMDevice* defaultDevice = nullptr;
				if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &defaultDevice)))
				{
					LPWSTR defaultDeviceId = nullptr;
					if (SUCCEEDED(defaultDevice->GetId(&defaultDeviceId)))
					{
						if (currentDeviceId && defaultDeviceId && wcscmp(currentDeviceId, defaultDeviceId) != 0)
						{
							LogF(L"WASAPILoopback: Default device change detected (%s -> %s). Restarting.", currentDeviceId, defaultDeviceId);
							CoTaskMemFree(defaultDeviceId);
							defaultDevice->Release();
							break;
						}
						CoTaskMemFree(defaultDeviceId);
					}
					defaultDevice->Release();
				}
			}

			UINT32 packetLength = 0;
			hr = captureClient->GetNextPacketSize(&packetLength);
			if (FAILED(hr))
			{
				LogF(L"WASAPILoopback: GetNextPacketSize failed: 0x%08X", hr);
				break;
			}

			while (packetLength != 0)
			{
				BYTE* data = nullptr;
				UINT32 numFramesAvailable;
				DWORD flags;

				hr = captureClient->GetBuffer(&data, &numFramesAvailable, &flags, NULL, NULL);
				if (FAILED(hr)) break;

				if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && data && numFramesAvailable > 0)
				{
					std::vector<float> samples;
					samples.resize(numFramesAvailable * channelCount);

					if (mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || 
					   (mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE && ((WAVEFORMATEXTENSIBLE*)mixFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
					{
						memcpy(samples.data(), data, numFramesAvailable * channelCount * sizeof(float));
					}
					else if (mixFormat->wBitsPerSample == 16)
					{
						short* src = (short*)data;
						for (size_t i = 0; i < samples.size(); ++i)
						{
							samples[i] = src[i] / 32768.0f;
						}
					}
					
					if (mutex.try_lock())
					{
						if (callback && running)
							callback(samples, channelCount, sampleRate);
						mutex.unlock();
						
						if (firstPacket)
						{
							LogF(L"WASAPILoopback: Captured first packet. Frames: %u", numFramesAvailable);
							firstPacket = false;
						}
					}
				}

				hr = captureClient->ReleaseBuffer(numFramesAvailable);
				if (FAILED(hr)) break;

				hr = captureClient->GetNextPacketSize(&packetLength);
				if (FAILED(hr)) break;
			}
			
			if (FAILED(hr)) break;
		}

		LogF(L"WASAPILoopback: Capture cycle ended. Cleaning up for possible restart...");

		if (currentDeviceId) CoTaskMemFree(currentDeviceId);

		audioClient->Stop();
		captureClient->Release();
		CoTaskMemFree(mixFormat);
		audioClient->Release();
		device->Release();
		enumerator->Release();

		if (running)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	CoUninitialize();
}
