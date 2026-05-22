#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include "portaudio.h"
#include "pa_asio.h"

#include "audio/WavFileReader.h"

struct AudioEngineConfig
{
    PaDeviceIndex deviceIndex = paNoDevice;
    int outputChannel = 0;
    long framesPerBuffer = 512;
    bool loopPlayback = false;
};

class AudioEngine
{
public:
    using CompletionCallback = std::function<void()>;

    AudioEngine();
    ~AudioEngine();

    bool IsPlaying() const { return m_playing; }

    bool Start(
        const WavFileReader& wav,
        const AudioEngineConfig& config,
        CompletionCallback onComplete,
        std::string* errorOut);

    void Stop();

private:
    static int StreamCallback(
        const void* input,
        void* output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

    void NotifyComplete();

    PaStream* m_stream = nullptr;
    std::vector<float> m_samples;
    int m_channelCount = 0;
    std::uint64_t m_frameCount = 0;
    std::atomic<std::uint64_t> m_playhead{0};
    bool m_loopPlayback = false;
    bool m_playing = false;

    int m_outputChannel = 0;
    std::vector<int> m_channelSelectors;
    PaAsioStreamInfo m_asioInfo = {};

    CompletionCallback m_onComplete;
};
