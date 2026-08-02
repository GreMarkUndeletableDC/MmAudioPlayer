#pragma once
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
    W32ERR LoadFromFile(_In_z_ PCWSTR pszFilePath) noexcept;
    W32ERR LoadFromMemory(
        _In_reads_bytes_(cbData) PCVOID pData,
        size_t cbData) noexcept;

    UINT GetSampleCount() const noexcept
    {
        return m_rbWave.Size() / DefaultWaveFormat.nBlockAlign;
    }

    UINT16* GetData() noexcept
    {
        return (UINT16*)m_rbWave.Data();
    }
};
