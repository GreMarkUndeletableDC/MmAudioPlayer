#include "pch.h"
#include "CAudioPlayer.h"


void CAudioPlayer::WaveThread() noexcept
{
    EckLoop()
    {
        eck::WaitObject(m_Event);
        const eck::CSrwWriteGuard _{ m_Lock };
        if (m_bExit)
            break;
        EckCounter(BufferQueueSize, i)
        {
            auto& Wave = m_WaveHeader[i];
            if (!(Wave.dwFlags & WHDR_DONE) && Wave.dwFlags)
                continue;
            waveOutUnprepareHeader(m_hWaveOut, &Wave, sizeof(WAVEHDR));
            MixAudio(i);
            if (Wave.lpData)
            {
                waveOutPrepareHeader(m_hWaveOut, &Wave, sizeof(WAVEHDR));
                waveOutWrite(m_hWaveOut, &Wave, sizeof(WAVEHDR));
            }
        }
    }
}

void CAudioPlayer::MixAudio(size_t idxQueue) noexcept
{
    BOOL bActive{};

    const auto pBuffer = m_Buffer[idxQueue];
    const auto cbBuffer = DefaultBufferCount * sizeof(INT16);
    RtlZeroMemory(pBuffer, cbBuffer);
    for (const auto& pInstance : m_vInstance)
    {
        if (pInstance->State() != CAudioInstance::State::Playing)
            continue;
        auto& idx = pInstance->CurrentSampleIndex();
        const auto cTotalSample = pInstance->GetFile()->GetSampleCount();
        const auto cSample = std::min(
            UINT(DefaultBufferCount / 2),
            (cTotalSample > idx) ? (cTotalSample - idx) : 0u);
        if (cSample)
            bActive = TRUE;

        for (UINT i = 0; i < cSample; ++i)
        {
            const auto pFile = pInstance->GetFile();
            const auto pSample = (const INT16*)pFile->GetData() + (idx + i) * 2;

            const auto l = (int)pBuffer[i * 2] + pSample[0];
            const auto r = (int)pBuffer[i * 2 + 1] + pSample[1];
            pBuffer[i * 2] = (INT16)std::clamp(l, -32768, 32767);
            pBuffer[i * 2 + 1] = (INT16)std::clamp(r, -32768, 32767);
        }
        idx += cSample;
    }

    m_bPlaying = bActive;
    auto& Wave = m_WaveHeader[idxQueue];
    if (bActive)
    {
        Wave.lpData = (PCH)pBuffer;
        Wave.dwBufferLength = (DWORD)cbBuffer;
    }
    else
    {
        Wave.lpData = nullptr;
        Wave.dwBufferLength = 0;
    }
}

MMRESULT CAudioPlayer::Initialize() noexcept
{
    MMRESULT mmr;

    const eck::CSrwWriteGuard _{ m_Lock };

    if (m_hWaveOut)
        return MMSYSERR_NOERROR;

    m_bExit = FALSE;

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
    MMRESULT mmr;

    {
        const eck::CSrwWriteGuard _{ m_Lock };
        m_bPlaying = FALSE;
        m_bExit = TRUE;
    }
    m_Event.Signal();
    eck::WaitObject(m_Thread);

    const eck::CSrwWriteGuard _{ m_Lock };

    waveOutReset(m_hWaveOut);
    waveOutClose(m_hWaveOut);
    m_hWaveOut = nullptr;

    return MMRESULT();
}

RefPtr<CAudioInstance> CAudioPlayer::AddInstance(RefPtr<CAudioFile> pFile) noexcept
{
    auto pInstance = CAudioInstance::Make(std::move(pFile));
    pInstance->State() = CAudioInstance::State::Playing;

    const eck::CSrwWriteGuard _{ m_Lock };
    m_vInstance.emplace_back(pInstance);
    if (!m_bPlaying)
        m_Event.Signal();
    return pInstance;
}

BOOL CAudioPlayer::RemoveInstance(const RefPtr<CAudioInstance>& pInstance) noexcept
{
    const eck::CSrwWriteGuard _{ m_Lock };
    for (auto it = m_vInstance.begin(); it != m_vInstance.end(); ++it)
    {
        if (it->Get() == pInstance.Get())
        {
            m_vInstance.erase(it);
            return TRUE;
        }
    }
    return FALSE;
}
