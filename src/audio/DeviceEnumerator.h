#pragma once

#include <string>
#include <vector>

#include "portaudio.h"

struct AsioDeviceInfo
{
    PaDeviceIndex deviceIndex;
    std::string name;
    int maxOutputChannels;
    double defaultSampleRate;
};

struct AsioBufferSizeOption
{
    long frames;
    bool isPreferred;
};

class DeviceEnumerator
{
public:
    bool Initialize(std::string* errorOut);
    void Shutdown();

    bool IsInitialized() const { return m_initialized; }

    const std::vector<AsioDeviceInfo>& devices() const { return m_devices; }
    int defaultDeviceListIndex() const { return m_defaultListIndex; }

    bool GetBufferSizes(
        PaDeviceIndex device,
        std::vector<AsioBufferSizeOption>* options,
        std::string* errorOut) const;

    static int FindDeviceListIndex(
        const std::vector<AsioDeviceInfo>& devices,
        PaDeviceIndex deviceIndex);

private:
    bool m_initialized = false;
    std::vector<AsioDeviceInfo> m_devices;
    int m_defaultListIndex = 0;
};
