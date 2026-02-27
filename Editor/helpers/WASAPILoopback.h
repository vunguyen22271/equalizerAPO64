#pragma once

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functional>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

class WASAPILoopback
{
public:
	using AudioCallback = std::function<void(const std::vector<float>&, int channelCount, int sampleRate)>;

	WASAPILoopback();
	~WASAPILoopback();

	bool start(AudioCallback callback);
	void stop();

private:
	void captureThreadFunc();

	std::atomic<bool> running;
	std::thread captureThread;
	AudioCallback callback;
	std::mutex mutex;
};
