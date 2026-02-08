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
	HRESULT hr;

	hr = CoInitialize(NULL);
	if (FAILED(hr)) return;

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
	if (FAILED(hr))
	{
		LogF(L"WASAPILoopback: Failed to create MMDeviceEnumerator: 0x%08X", hr);
		CoUninitialize();
		return;
	}

	hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
	if (FAILED(hr))
	{
		enumerator->Release();
		CoUninitialize();
		return;
	}

	hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&audioClient);
	if (FAILED(hr))
	{
		LogF(L"WASAPILoopback: Failed to activate audio client: 0x%08X", hr);
		device->Release();
		enumerator->Release();
		CoUninitialize();
		return;
	}

	hr = audioClient->GetMixFormat(&mixFormat);
	if (FAILED(hr))
	{
		audioClient->Release();
		device->Release();
		enumerator->Release();
		CoUninitialize();
		return;
	}

	// Request loopback mode
	hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, mixFormat, 0);
	if (FAILED(hr))
	{
		LogF(L"WASAPILoopback: Failed to initialize audio client: 0x%08X", hr);
		CoTaskMemFree(mixFormat);
		audioClient->Release();
		device->Release();
		enumerator->Release();
		CoUninitialize();
		return;
	}

	hr = audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient);
	if (FAILED(hr))
	{
		CoTaskMemFree(mixFormat);
		audioClient->Release();
		device->Release();
		enumerator->Release();
		CoUninitialize();
		return;
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
		CoUninitialize();
		return;
	}
	
	LogF(L"WASAPILoopback: Started capture. Channels: %d, Rate: %d", mixFormat->nChannels, mixFormat->nSamplesPerSec);

	// Capture loop
	UINT32 packetLength = 0;
	BYTE* data = nullptr;
	UINT32 numFramesAvailable;
	DWORD flags;

	int channelCount = mixFormat->nChannels;
	int sampleRate = mixFormat->nSamplesPerSec;

	bool firstPacket = true;

	while (running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		hr = captureClient->GetNextPacketSize(&packetLength);
		while (packetLength != 0)
		{
			hr = captureClient->GetBuffer(&data, &numFramesAvailable, &flags, NULL, NULL);
			if (FAILED(hr)) break;

			if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
			{
				// Handle silence if needed, or just push zeros
			}
			else if (data && numFramesAvailable > 0)
			{
				// Convert to float
				std::vector<float> samples;
				samples.resize(numFramesAvailable * channelCount);

				if (mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || 
				   (mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE && ((WAVEFORMATEXTENSIBLE*)mixFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
				{
					// Already float
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
				// Add other formats as needed (24-bit int, etc.)
                
				if (mutex.try_lock())
				{
					if (callback)
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
		}
	}

	audioClient->Stop();
	
	captureClient->Release();
	CoTaskMemFree(mixFormat);
	audioClient->Release();
	device->Release();
	enumerator->Release();
	CoUninitialize();
}
