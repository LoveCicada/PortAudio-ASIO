#include "audio/WavFileReader.h"

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace {

#pragma pack(push, 1)
struct RiffChunkHeader
{
    char id[4];
    std::uint32_t size;
};

struct FmtChunkPcm
{
    std::uint16_t audioFormat;
    std::uint16_t numChannels;
    std::uint32_t sampleRate;
    std::uint32_t byteRate;
    std::uint16_t blockAlign;
    std::uint16_t bitsPerSample;
};
#pragma pack(pop)

std::string WideToUtf8(const std::wstring& wide)
{
    if(wide.empty())
        return std::string();

    const int size = WideCharToMultiByte(
        CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
        nullptr, 0, nullptr, nullptr);
    if(size <= 0)
        return std::string();

    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
        &utf8[0], size, nullptr, nullptr);
    return utf8;
}

bool ReadExact(std::ifstream& in, void* dst, std::size_t bytes)
{
    in.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(bytes));
    return static_cast<std::size_t>(in.gcount()) == bytes;
}

float SampleToFloat(const std::uint8_t* src, int bitsPerSample, bool isFloat)
{
    if(isFloat && bitsPerSample == 32)
    {
        float v;
        std::memcpy(&v, src, sizeof(v));
        return v;
    }

    switch(bitsPerSample)
    {
    case 16:
    {
        std::int16_t v;
        std::memcpy(&v, src, sizeof(v));
        return static_cast<float>(v) / 32768.0f;
    }
    case 24:
    {
        std::int32_t v = (src[0]) | (src[1] << 8) | (src[2] << 16);
        if(v & 0x800000)
            v |= 0xFF000000;
        return static_cast<float>(v) / 8388608.0f;
    }
    case 32:
    {
        std::int32_t v;
        std::memcpy(&v, src, sizeof(v));
        return static_cast<float>(v) / 2147483648.0f;
    }
    default:
        return 0.0f;
    }
}

} // namespace

bool WavFileReader::Load(const std::wstring& path, std::string* errorOut)
{
    m_sampleRate = 0.0;
    m_channelCount = 0;
    m_frameCount = 0;
    m_samples.clear();

    const std::string pathUtf8 = WideToUtf8(path);
    std::ifstream in(pathUtf8.c_str(), std::ios::binary);
    if(!in)
    {
        if(errorOut)
            *errorOut = "Cannot open WAV file.";
        return false;
    }

    RiffChunkHeader riff = {};
    if(!ReadExact(in, &riff, sizeof(riff)) ||
       std::strncmp(riff.id, "RIFF", 4) != 0)
    {
        if(errorOut)
            *errorOut = "Not a RIFF file.";
        return false;
    }

    char waveId[4] = {};
    if(!ReadExact(in, waveId, 4) || std::strncmp(waveId, "WAVE", 4) != 0)
    {
        if(errorOut)
            *errorOut = "Not a WAVE file.";
        return false;
    }

    FmtChunkPcm fmt = {};
    bool haveFmt = false;
    std::vector<std::uint8_t> dataBytes;

    while(in)
    {
        RiffChunkHeader chunk = {};
        if(!ReadExact(in, &chunk, sizeof(chunk)))
            break;

        const std::size_t payloadSize = chunk.size;
        if(std::strncmp(chunk.id, "fmt ", 4) == 0)
        {
            if(payloadSize < sizeof(FmtChunkPcm))
            {
                if(errorOut)
                    *errorOut = "fmt chunk too small.";
                return false;
            }
            if(!ReadExact(in, &fmt, sizeof(fmt)))
            {
                if(errorOut)
                    *errorOut = "Failed to read fmt chunk.";
                return false;
            }
            haveFmt = true;
            if(payloadSize > sizeof(fmt))
                in.seekg(static_cast<std::streamoff>(payloadSize - sizeof(fmt)), std::ios::cur);
        }
        else if(std::strncmp(chunk.id, "data", 4) == 0)
        {
            dataBytes.resize(payloadSize);
            if(payloadSize > 0 && !ReadExact(in, dataBytes.data(), payloadSize))
            {
                if(errorOut)
                    *errorOut = "Failed to read data chunk.";
                return false;
            }
        }
        else
        {
            in.seekg(static_cast<std::streamoff>(payloadSize), std::ios::cur);
        }

        if(payloadSize % 2 == 1)
            in.seekg(1, std::ios::cur);
    }

    if(!haveFmt || dataBytes.empty())
    {
        if(errorOut)
            *errorOut = "Missing fmt or data chunk.";
        return false;
    }

    const bool isFloat = (fmt.audioFormat == 3);
    const bool isPcm = (fmt.audioFormat == 1);
    if(!isPcm && !isFloat)
    {
        if(errorOut)
        {
            std::ostringstream oss;
            oss << "Unsupported WAV format (wFormatTag=" << fmt.audioFormat << ").";
            *errorOut = oss.str();
        }
        return false;
    }

    if(fmt.numChannels < 1 || fmt.numChannels > 2)
    {
        if(errorOut)
            *errorOut = "Only mono or stereo WAV is supported.";
        return false;
    }

    int bytesPerSample = 0;
    if(fmt.bitsPerSample == 24)
        bytesPerSample = 3;
    else if((fmt.bitsPerSample % 8) == 0)
        bytesPerSample = fmt.bitsPerSample / 8;

    if(bytesPerSample <= 0)
    {
        if(errorOut)
            *errorOut = "Invalid bits per sample.";
        return false;
    }

    const std::size_t frameSize = static_cast<std::size_t>(fmt.numChannels) *
                                  static_cast<std::size_t>(bytesPerSample);
    if(frameSize == 0 || (dataBytes.size() % frameSize) != 0)
    {
        if(errorOut)
            *errorOut = "Data size is not aligned to frame size.";
        return false;
    }

    const std::uint64_t frames =
        static_cast<std::uint64_t>(dataBytes.size() / frameSize);

    m_sampleRate = static_cast<double>(fmt.sampleRate);
    m_channelCount = static_cast<int>(fmt.numChannels);
    m_frameCount = frames;
    m_samples.resize(static_cast<size_t>(frames * m_channelCount));

    for(std::uint64_t frame = 0; frame < frames; ++frame)
    {
        for(int ch = 0; ch < m_channelCount; ++ch)
        {
            const std::size_t offset =
                static_cast<std::size_t>(frame) * frameSize +
                static_cast<std::size_t>(ch) * static_cast<std::size_t>(bytesPerSample);
            const float sample = SampleToFloat(
                dataBytes.data() + offset,
                fmt.bitsPerSample,
                isFloat);
            m_samples[static_cast<size_t>(frame * m_channelCount + ch)] = sample;
        }
    }

    return true;
}
