#pragma once
#include "AudioDefine.h"

// 表示一个固定DefaultWaveFormat格式的PCM字节流
// 注意本类加载完毕后只读，二次加载时必须确认未被播放器选入
class CAudioFile
{
public:
    constexpr static WAVEFORMATEX DefaultWaveFormat
    {
        .wFormatTag = WAVE_FORMAT_PCM,
        .nChannels = 2,
        .nSamplesPerSec = 44100,
        .nAvgBytesPerSec = 44100 * ((2 * 16) / 8),
        .nBlockAlign = (2 * 16) / 8,
        .wBitsPerSample = 16,
    };
private:
    eck::CByteBuffer m_rbWave{};
public:
    AudioError LoadFromFile(_In_z_ PCWSTR pszFilePath) noexcept;

    AudioError LoadFromMemory(
        _In_reads_bytes_(cbData) PCVOID pData,
        size_t cbData) noexcept;

    EckInlineNdCe UINT GetSampleCount() const noexcept
    {
        return UINT(m_rbWave.Size() / DefaultWaveFormat.nBlockAlign);
    }

    EckInlineNdCe const UINT16* GetData() const noexcept
    {
        return (const UINT16*)m_rbWave.Data();
    }
};