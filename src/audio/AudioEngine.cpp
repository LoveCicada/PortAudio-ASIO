#include "audio/AudioEngine.h"

#include <cstring>

#include "pa_asio.h"

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine()
{
    Stop();
}

bool AudioEngine::Start(
    const WavFileReader& wav,
    const AudioEngineConfig& config,
    CompletionCallback onComplete,
    std::string* errorOut)
{
    Stop();

    if(config.deviceIndex == paNoDevice)
    {
        if(errorOut)
            *errorOut = "No output device selected.";
        return false;
    }

    if(wav.frameCount() == 0 || wav.samples().empty())
    {
        if(errorOut)
            *errorOut = "WAV has no audio frames.";
        return false;
    }

    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(config.deviceIndex);
    if(!devInfo)
    {
        if(errorOut)
            *errorOut = "Invalid device index.";
        return false;
    }

    m_samples = wav.samples();
    m_channelCount = wav.channelCount();
    m_frameCount = wav.frameCount();
    m_loopPlayback = config.loopPlayback;
    m_playhead.store(0);
    m_onComplete = onComplete;
    m_outputChannel = config.outputChannel;

    m_channelSelectors.clear();
    if(m_channelCount == 1)
    {
        if(config.outputChannel < 0 ||
           config.outputChannel >= devInfo->maxOutputChannels)
        {
            if(errorOut)
                *errorOut = "Output channel index out of range.";
            return false;
        }
        m_channelSelectors.push_back(config.outputChannel);
    }
    else
    {
        if(devInfo->maxOutputChannels < 2)
        {
            if(errorOut)
                *errorOut = "Device does not have two output channels.";
            return false;
        }
        m_channelSelectors.push_back(0);
        m_channelSelectors.push_back(1);
    }

    PaStreamParameters outputParams = {};
    outputParams.device = config.deviceIndex;
    outputParams.channelCount = m_channelCount;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = devInfo->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    m_asioInfo = {};
    m_asioInfo.size = sizeof(PaAsioStreamInfo);
    m_asioInfo.hostApiType = paASIO;
    m_asioInfo.version = 1;
    m_asioInfo.flags = paAsioUseChannelSelectors;
    m_asioInfo.channelSelectors = m_channelSelectors.data();
    outputParams.hostApiSpecificStreamInfo = &m_asioInfo;

    PaError err = Pa_OpenStream(
        &m_stream,
        nullptr,
        &outputParams,
        wav.sampleRate(),
        static_cast<unsigned long>(config.framesPerBuffer),
        paClipOff,
        StreamCallback,
        this);

    if(err != paNoError)
    {
        m_stream = nullptr;
        if(errorOut)
            *errorOut = Pa_GetErrorText(err);
        return false;
    }

    err = Pa_StartStream(m_stream);
    if(err != paNoError)
    {
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
        if(errorOut)
            *errorOut = Pa_GetErrorText(err);
        return false;
    }

    m_playing = true;
    return true;
}

void AudioEngine::Stop()
{
    if(m_stream)
    {
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
    }
    m_playing = false;
    m_samples.clear();
    m_channelSelectors.clear();
    m_onComplete = nullptr;
}

void AudioEngine::NotifyComplete()
{
    CompletionCallback cb;
    {
        cb = m_onComplete;
        m_onComplete = nullptr;
    }
    if(cb)
        cb();
}

int AudioEngine::StreamCallback(
    const void* /*input*/,
    void* output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* /*timeInfo*/,
    PaStreamCallbackFlags /*statusFlags*/,
    void* userData)
{
    AudioEngine* self = static_cast<AudioEngine*>(userData);
    float* out = static_cast<float*>(output);

    if(!self || self->m_frameCount == 0 || self->m_channelCount <= 0)
        return paComplete;

    const int ch = self->m_channelCount;
    std::uint64_t pos = self->m_playhead.load();

    for(unsigned long i = 0; i < frameCount; ++i)
    {
        if(pos >= self->m_frameCount)
        {
            if(self->m_loopPlayback)
                pos = 0;
            else
            {
                self->m_playhead.store(pos);
                self->NotifyComplete();
                return paComplete;
            }
        }

        const size_t base =
            static_cast<size_t>(pos) * static_cast<size_t>(ch);
        for(int c = 0; c < ch; ++c)
            out[i * static_cast<unsigned long>(ch) + static_cast<unsigned long>(c)] =
                self->m_samples[base + static_cast<size_t>(c)];

        ++pos;
    }

    self->m_playhead.store(pos);
    return paContinue;
}
