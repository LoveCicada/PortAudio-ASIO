#pragma once

#include <cstdint>
#include <string>
#include <vector>

class WavFileReader
{
public:
    bool Load(const std::wstring& path, std::string* errorOut);

    double sampleRate() const { return m_sampleRate; }
    int channelCount() const { return m_channelCount; }
    std::uint64_t frameCount() const { return m_frameCount; }
    const std::vector<float>& samples() const { return m_samples; }

private:
    double m_sampleRate = 0.0;
    int m_channelCount = 0;
    std::uint64_t m_frameCount = 0;
    std::vector<float> m_samples;
};
