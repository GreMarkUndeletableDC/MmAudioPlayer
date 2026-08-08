#include "pch.h"
#include "CAudioPlayer.h"


void CAudioPlayer::WaveThread() noexcept
{
    EckLoop()
    {
        eck::WaitObject(m_Event);
        if (m_bExit.load(std::memory_order_acquire))
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
    const eck::CSrwWriteGuard _{ m_Lock };
    BOOL bActive{};

    const auto pBuffer = m_Buffer[idxQueue];
    const auto cbBuffer = DefaultBufferCount * sizeof(INT16);
    RtlZeroMemory(pBuffer, cbBuffer);
    for (auto& Inst : m_vInstance)
    {
        if (Inst.eState != State::Playing)
            continue;
        const auto cTotalSample = Inst.pFile->GetSampleCount();
        const auto idxCurr = Inst.idxCurrSample;
        const auto cSample = std::min(
            UINT(DefaultBufferCount / 2),
            cTotalSample - idxCurr);
        Inst.idxCurrSample += cSample;
        if (cSample)
            bActive = TRUE;

        for (UINT i = 0; i < cSample; ++i)
        {
            const auto pSample = Inst.pFile->GetData() + (idxCurr + i) * 2;

            const auto l = (int)pBuffer[i * 2] + pSample[0] * Inst.byVolume / 255;
            const auto r = (int)pBuffer[i * 2 + 1] + pSample[1] * Inst.byVolume / 255;
            pBuffer[i * 2] = (INT16)std::clamp(l, -32768, 32767);
            pBuffer[i * 2 + 1] = (INT16)std::clamp(r, -32768, 32767);
        }
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

    m_bExit.store(false, std::memory_order_release);

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
    m_bExit.store(true, std::memory_order_release);
    m_Event.Signal();
    eck::WaitObject(m_Thread);

    const eck::CSrwWriteGuard _{ m_Lock };

    m_bPlaying = FALSE;

    waveOutReset(m_hWaveOut);
    waveOutClose(m_hWaveOut);
    m_hWaveOut = nullptr;

    return MMRESULT();
}

UINT CAudioPlayer::InstAdd(const INST_PARAM& Param) noexcept
{
    Param.pFile->Select();
    UINT id;
    const eck::CSrwWriteGuard _{ m_Lock };
    if (m_FreeRange.IsEmpty())
    {
        m_vInstance.emplace_back();
        id = UINT(m_vInstance.size() - 1);
    }
    else
    {
        const auto idx = m_FreeRange.GetFirstSelected();
        m_FreeRange.ExcludeItem(idx);
        id = (UINT)idx;
    }

    auto& Inst = m_vInstance[id];
    Inst.pFile = Param.pFile;
    Inst.eState = Param.eState;
    Inst.byVolume = Param.byVolume;
    Inst.idxCurrSample = 0u;
    if (!m_bPlaying)
        m_Event.Signal();
    return id;
}

void CAudioPlayer::InstRemove(UINT id) noexcept
{
    m_Lock.EnterWrite();
    auto pFile = std::move(m_vInstance[id].pFile);
    if (id == m_vInstance.size() - 1)
    {
        do
            m_vInstance.pop_back();
        while (!m_vInstance.empty() &&
            m_vInstance.back().eState == State::Invalid);
        m_FreeRange.OnSetItemCount((int)m_vInstance.size());
    }
    else
    {
        m_vInstance[id].eState = State::Invalid;
        m_FreeRange.IncludeItem((int)id);
    }
    m_Lock.LeaveWrite();
    pFile->Deselect();
}