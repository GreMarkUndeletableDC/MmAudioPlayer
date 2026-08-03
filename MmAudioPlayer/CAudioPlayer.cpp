#include "pch.h"
#include "CAudioPlayer.h"


void CAudioPlayer::WaveThread() noexcept
{
    EckLoop()
    {
        eck::WaitObject(m_Event);
        EckCounter(BufferQueueSize, i)
        {
            auto& Wave = m_WaveHeader[i];
            if (!(Wave.dwFlags & WHDR_DONE) && Wave.dwFlags)
                continue;
            waveOutUnprepareHeader(m_hWaveOut, &Wave, sizeof(WAVEHDR));
            MixAudio(i);
            waveOutPrepareHeader(m_hWaveOut, &Wave, sizeof(WAVEHDR));
            waveOutWrite(m_hWaveOut, &Wave, sizeof(WAVEHDR));
        }
    }
}

void CAudioPlayer::MixAudio(size_t idxQueue) noexcept
{
    BOOL bActive{};

    const auto pBuffer = m_Buffer[idxQueue];
    const auto cbBuffer = DefaultBufferCount * sizeof(INT16);
    RtlZeroMemory(pBuffer, cbBuffer);
    for (const auto& pInst : m_vInstance)
    {
        const auto eState = pInst->GetState();
        if (eState == CAudioInstance::State::Playing)
        {
            const auto idx = pInst->GetCurrentSampleIndex();
            const auto cSample = std::min(
                UINT(DefaultBufferCount / 2), pInst->GetTotalSampleCount() - idx);

            pInst->SetCurrentSampleIndex(idx + cSample);
            if (cSample)
                bActive = TRUE;

            for (UINT i = 0; i < cSample; ++i)
            {
                const auto pFile = pInst->GetFile();
                const auto pSample = (const INT16*)pFile->GetData() + (idx + i) * 2;

                const auto l = (int)pBuffer[i * 2] + pSample[0]; 
                const auto r = (int)pBuffer[i * 2 + 1] + pSample[1];
                pBuffer[i * 2] = (INT16)std::clamp(l, -32768, 32767);
                pBuffer[i * 2 + 1] = (INT16)std::clamp(r, -32768, 32767);
            }
        }
    }

    m_bPlaying = bActive;
    if (bActive)
    {
        auto& Wave = m_WaveHeader[idxQueue];
        Wave.lpData = (PCH)pBuffer;
        Wave.dwBufferLength = (DWORD)cbBuffer;
    }
}

MMRESULT CAudioPlayer::Initialize() noexcept
{
    MMRESULT mmr;

    if (m_hWaveOut)
        return MMSYSERR_NOERROR;

    mmr = waveOutOpen(
        &m_hWaveOut,
        WAVE_MAPPER,
        &CAudioFile::DefaultWaveFormat,
        (DWORD_PTR)m_Event.Get(),
        0,
        CALLBACK_EVENT);
    if (mmr != MMSYSERR_NOERROR)
    {
        m_hWaveOut = nullptr;
        return mmr;
    }

    m_Thread.Attach(eck::CrtCreateThread([](void* p) noexcept -> UINT
        {
            ((CAudioPlayer*)p)->WaveThread();
            return 0;
        }, this));

    return MMSYSERR_NOERROR;
}

MMRESULT CAudioPlayer::Uninitialize() noexcept
{
    m_bPlaying = FALSE;
    return MMRESULT();
}

RefPtr<CAudioInstance> CAudioPlayer::AddInstance(RefPtr<CAudioFile> pFile) noexcept
{
    auto& e = m_vInstance.emplace_back(RefPtr<CAudioInstance>::Make(pFile));
    e->SetState(CAudioInstance::State::Playing);
    if (!m_bPlaying)
        m_Event.Signal();
    return e;
}