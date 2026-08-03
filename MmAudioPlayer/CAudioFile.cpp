#include "pch.h"
#include "CAudioFile.h"

#pragma pack(push, 1)
struct RIFF_HEADER
{
    char ChunkId[4];// "RIFF"
    UINT cbChunk;
    char Format[4]; // "WAVE"
};
#pragma pack(pop)


static MMRESULT ConvertAudioFormat(
    const WAVEFORMATEX* pFmtIn,
    const WAVEFORMATEX* pFmtOut,
    _In_reads_bytes_(cbIn) PCVOID pIn,
    size_t cbIn,
    eck::CByteBuffer& rbOut) noexcept
{
    MMRESULT mr;
    HACMSTREAM hAcmStream;

    mr = acmStreamOpen(
        &hAcmStream,
        nullptr,
        (WAVEFORMATEX*)pFmtIn,
        (WAVEFORMATEX*)pFmtOut,
        nullptr,
        0,
        0,
        ACM_STREAMOPENF_NONREALTIME);
    if (mr != MMSYSERR_NOERROR)
        return mr;

    eck::CScopeGuard Guard{ [&]() noexcept
        {
            acmStreamClose(hAcmStream, 0);
        } };

    DWORD cbOut;
    mr = acmStreamSize(hAcmStream, (DWORD)cbIn, &cbOut, ACM_STREAMSIZEF_SOURCE);
    if (mr != MMSYSERR_NOERROR)
        return mr;

    rbOut.ReSize(cbOut);

    ACMSTREAMHEADER StreamHeader
    {
        .cbStruct = sizeof(ACMSTREAMHEADER),
        .pbSrc = (BYTE*)pIn,
        .cbSrcLength = (DWORD)cbIn,
        .pbDst = rbOut.Data(),
        .cbDstLength = cbOut,
    };
    mr = acmStreamPrepareHeader(hAcmStream, &StreamHeader, 0);
    if (mr != MMSYSERR_NOERROR)
        return mr;
    mr = acmStreamConvert(hAcmStream, &StreamHeader,
        ACM_STREAMCONVERTF_START | ACM_STREAMCONVERTF_END);
    acmStreamUnprepareHeader(hAcmStream, &StreamHeader, 0);
    if (mr == MMSYSERR_NOERROR)
        rbOut.ReSize(StreamHeader.cbDstLengthUsed);

    return mr;
}


AudioError CAudioFile::LoadFromFile(_In_z_ PCWSTR pszFilePath) noexcept
{
    NTSTATUS nts;
    const auto rb = eck::ReadInFile(pszFilePath, &nts);
    if (!NT_SUCCESS(nts))
        return { AudioResult::File, nts };
    return LoadFromMemory(rb.Data(), rb.Size());
}

AudioError CAudioFile::LoadFromMemory(
    _In_reads_bytes_(cbData) PCVOID pData,
    size_t cbData) noexcept try
{
    MMRESULT mmr;
    eck::CMemoryReader r{ pData, cbData };

    RIFF_HEADER const* pRiffHeader;
    r.SkipPointer(pRiffHeader);

    if (memcmp(pRiffHeader->ChunkId, "RIFF", 4) != 0 ||
        memcmp(pRiffHeader->Format, "WAVE", 4) != 0)
    {
        namespace Tag = eck::MediaTag;

        eck::CStreamView Stream{ pData, cbData };
        Tag::CMediaFile File{ &Stream };
        File.DetectTag();

        Tag::CMpeg Mpeg{ File };
        if (Mpeg.Read() != Tag::Result::Ok)
            return { AudioResult::BadFormat };

        const auto& Info = Mpeg.GetInformation();
        if (Info.eLayer != Tag::CMpeg::Layer::Layer3)
            return { AudioResult::BadFormat };
        const auto nBitrateKbps = Mpeg.GetBitrate();
        const auto nSampleRate = Mpeg.GetSampleRate();
        if (!nBitrateKbps || !nSampleRate)
            return { AudioResult::BadFormat };
        const auto nBitrateBps = nBitrateKbps * 1000;

        MPEGLAYER3WAVEFORMAT Mp3Format{};
        Mp3Format.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
        Mp3Format.wfx.nChannels = Mpeg.GetChannelCount();
        Mp3Format.wfx.nSamplesPerSec = nSampleRate;
        Mp3Format.wfx.nAvgBytesPerSec = nBitrateBps / 8;
        Mp3Format.wfx.nBlockAlign = 1;
        Mp3Format.wfx.wBitsPerSample = 0;
        Mp3Format.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
        Mp3Format.wID = MPEGLAYER3_ID_MPEG;
        Mp3Format.fdwFlags = Info.bPadding ?
            MPEGLAYER3_FLAG_PADDING_ON :
            MPEGLAYER3_FLAG_PADDING_OFF;
        Mp3Format.nFramesPerBlock = 1;
        Mp3Format.nCodecDelay = 0;

        // 计算帧字节大小
        // MPEG-1       (144 * Bitrate) / SampleRate
        // MPEG-2/2.5   ( 72 * Bitrate) / SampleRate
        const auto nCoeff = (Info.eVersion == Tag::CMpeg::Version::Mpeg1) ? 144 : 72;
        auto cbBlock = (nCoeff * nBitrateBps) / nSampleRate;
        if (Info.bPadding)
            cbBlock += 1;// 如果有Padding位，通常物理帧会多出1字节
        Mp3Format.nBlockSize = (WORD)cbBlock;

        eck::CByteBuffer rb{};
        if (Mp3Format.wfx.nSamplesPerSec != DefaultWaveFormat.nSamplesPerSec)
        {
            auto TempFormat{ DefaultWaveFormat };
            TempFormat.nSamplesPerSec = Mp3Format.wfx.nSamplesPerSec;
            TempFormat.nAvgBytesPerSec = TempFormat.nSamplesPerSec * TempFormat.nBlockAlign;

            eck::CByteBuffer rbTemp{};
            mmr = ConvertAudioFormat(
                &Mp3Format.wfx,
                &TempFormat,
                (PCBYTE)pData + Mpeg.GetBeginPosition(),
                cbData - Mpeg.GetBeginPosition(),
                rbTemp);
            if (mmr != MMSYSERR_NOERROR)
                return { AudioResult::AcmConvert, mmr };

            mmr = ConvertAudioFormat(
                &TempFormat,
                &DefaultWaveFormat,
                rbTemp.Data(),
                rbTemp.Size(),
                rb);
            if (mmr != MMSYSERR_NOERROR)
                return { AudioResult::AcmConvert, mmr };
        }
        else
        {
            mmr = ConvertAudioFormat(
                &Mp3Format.wfx,
                &DefaultWaveFormat,
                (PCBYTE)pData + Mpeg.GetBeginPosition(),
                cbData - Mpeg.GetBeginPosition(),
                rb);
            if (mmr != MMSYSERR_NOERROR)
                return { AudioResult::AcmConvert, mmr };
        }

        m_rbWave = std::move(rb);
        return { AudioResult::Ok };
    }

    char ChunkId[4];
    UINT cbChunk;

    WAVEFORMATEX Format{};
    PCVOID pData{};
    UINT cbData{};
    while (!r.IsEnd())
    {
        r >> ChunkId >> cbChunk;
        if (memcmp(ChunkId, "data", 4) == 0)
        {
            pData = r.Data();
            cbData = cbChunk;
        }
        else if (memcmp(ChunkId, "fmt ", 4) == 0)
        {
            const auto* const pFormat = (WAVEFORMATEX*)r.Data();
            memcpy(&Format, pFormat, 16);
        }
        r += cbChunk;
    }

    if (!pData || Format.wFormatTag != WAVE_FORMAT_PCM)
        return { AudioResult::BadFormat };
    if (cbData > 512 * 1024 * 1024)
        return { AudioResult::TooLarge };

    if (memcmp(&DefaultWaveFormat, &Format, 16) == 0)
    {
        m_rbWave.ReSize(cbChunk);
        memcpy(m_rbWave.Data(), pData, cbChunk);
    }
    else
    {
        eck::CByteBuffer rb{};
        mmr = ConvertAudioFormat(
            &Format,
            &DefaultWaveFormat,
            pData,
            cbChunk,
            rb);
        if (mmr != MMSYSERR_NOERROR)
            return { AudioResult::AcmConvert, mmr };
        m_rbWave = std::move(rb);
    }
    return { AudioResult::Ok };
}
catch (const eck::CMemoryReader::Xpt&)
{
    return { AudioResult::BadFormat };
}