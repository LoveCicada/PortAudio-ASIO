#include "audio/DeviceEnumerator.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "pa_asio.h"

namespace {

bool ContainsIgnoreCase(const char* haystack, const char* needle)
{
    if(!haystack || !needle)
        return false;

    const size_t nlen = std::strlen(needle);
    if(nlen == 0)
        return true;

    for(const char* p = haystack; *p; ++p)
    {
        size_t i = 0;
        for(; i < nlen && p[i]; ++i)
        {
            if(std::tolower(static_cast<unsigned char>(p[i])) !=
               std::tolower(static_cast<unsigned char>(needle[i])))
            {
                break;
            }
        }
        if(i == nlen)
            return true;
    }
    return false;
}

void AppendLegalBufferSizes(
    long minFrames,
    long maxFrames,
    long preferredFrames,
    long granularity,
    std::vector<long>* outSizes)
{
    if(minFrames <= 0 || maxFrames <= 0 || minFrames > maxFrames)
        return;

    if(granularity == -1)
    {
        for(long size = minFrames; size <= maxFrames; size *= 2)
        {
            if(size < minFrames)
                continue;
            outSizes->push_back(size);
            if(size == 0)
                break;
        }
    }
    else if(granularity > 0)
    {
        for(long size = minFrames; size <= maxFrames; size += granularity)
            outSizes->push_back(size);
    }
    else
    {
        outSizes->push_back(minFrames);
        if(maxFrames != minFrames)
            outSizes->push_back(maxFrames);
    }

    if(preferredFrames > 0)
        outSizes->push_back(preferredFrames);

    std::sort(outSizes->begin(), outSizes->end());
    outSizes->erase(
        std::unique(outSizes->begin(), outSizes->end()),
        outSizes->end());
}

} // namespace

bool DeviceEnumerator::Initialize(std::string* errorOut)
{
    Shutdown();

    const PaError err = Pa_Initialize();
    if(err != paNoError)
    {
        if(errorOut)
            *errorOut = Pa_GetErrorText(err);
        return false;
    }

    m_initialized = true;
    m_devices.clear();
    m_defaultListIndex = 0;

    const PaHostApiIndex asioHostApi = Pa_HostApiTypeIdToHostApiIndex(paASIO);
    if(asioHostApi < 0)
    {
        if(errorOut)
            *errorOut = "ASIO host API is not available.";
        Pa_Terminate();
        m_initialized = false;
        return false;
    }

    int danteListIndex = -1;

    const int deviceCount = Pa_GetDeviceCount();
    for(int i = 0; i < deviceCount; ++i)
    {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if(!info || info->hostApi != asioHostApi || info->maxOutputChannels <= 0)
            continue;

        AsioDeviceInfo entry;
        entry.deviceIndex = i;
        entry.name = info->name ? info->name : "(unnamed)";
        entry.maxOutputChannels = info->maxOutputChannels;
        entry.defaultSampleRate = info->defaultSampleRate;
        m_devices.push_back(entry);

        if(danteListIndex < 0 &&
           ContainsIgnoreCase(entry.name.c_str(), "Dante Virtual Soundcard"))
        {
            danteListIndex = static_cast<int>(m_devices.size()) - 1;
        }
    }

    if(m_devices.empty())
    {
        if(errorOut)
            *errorOut = "No ASIO output devices found.";
        Pa_Terminate();
        m_initialized = false;
        return false;
    }

    m_defaultListIndex = (danteListIndex >= 0) ? danteListIndex : 0;
    return true;
}

void DeviceEnumerator::Shutdown()
{
    if(m_initialized)
    {
        Pa_Terminate();
        m_initialized = false;
    }
    m_devices.clear();
    m_defaultListIndex = 0;
}

bool DeviceEnumerator::GetBufferSizes(
    PaDeviceIndex device,
    std::vector<AsioBufferSizeOption>* options,
    std::string* errorOut) const
{
    if(!options)
        return false;

    options->clear();

    long minFrames = 0;
    long maxFrames = 0;
    long preferredFrames = 0;
    long granularity = 0;

    const PaError err = PaAsio_GetAvailableBufferSizes(
        device, &minFrames, &maxFrames, &preferredFrames, &granularity);
    if(err != paNoError)
    {
        if(errorOut)
            *errorOut = Pa_GetErrorText(err);
        return false;
    }

    std::vector<long> sizes;
    AppendLegalBufferSizes(
        minFrames, maxFrames, preferredFrames, granularity, &sizes);

    if(sizes.empty() && preferredFrames > 0)
        sizes.push_back(preferredFrames);

    for(size_t i = 0; i < sizes.size(); ++i)
    {
        AsioBufferSizeOption opt;
        opt.frames = sizes[i];
        opt.isPreferred = (sizes[i] == preferredFrames);
        options->push_back(opt);
    }

    return !options->empty();
}

int DeviceEnumerator::FindDeviceListIndex(
    const std::vector<AsioDeviceInfo>& devices,
    PaDeviceIndex deviceIndex)
{
    for(size_t i = 0; i < devices.size(); ++i)
    {
        if(devices[i].deviceIndex == deviceIndex)
            return static_cast<int>(i);
    }
    return -1;
}
