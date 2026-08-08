#pragma once
#include "CAudioFile.h"

class CAudioPlayer
{
public:
    constexpr static size_t DefaultBufferCount = 44100 * 2 * 50 / 1000;
    constexpr static size_t BufferQueueSize = 4;

    enum class State : BYTE
    {
        Invalid,
        Playing,
        Paused,
    };

    struct INST_PARAM
    {
        RefPtr<CAudioFile> pFile{};
        State eState{ State::Playing };
        BYTE byVolume{ 255 };
    };
private:
    struct Instance
    {
        RefPtr<CAudioFile> pFile{};
        UINT idxCurrSample{};
        State eState{};
        BYTE byVolume{};
    };

    HWAVEOUT m_hWaveOut{};

    std::vector<Instance> m_vInstance{};
    eck::CSelectionRange m_FreeRange{};
    BOOLEAN m_bPlaying{};
    std::atomic<bool> m_bExit{};

    eck::CSrwLock m_Lock{};
    eck::CEvent m_Event{};
    eck::CWaitableObject m_Thread{};

    // 以下字段仅由音频线程访问

    WAVEHDR m_WaveHeader[BufferQueueSize]{};
    INT16 m_Buffer[BufferQueueSize][DefaultBufferCount]{};

    void WaveThread() noexcept;

    void MixAudio(size_t idxQueue) noexcept;
public:
    ECK_DISABLE_COPY_MOVE_DEF_CONS(CAudioPlayer)
public:
    MMRESULT Initialize() noexcept;
    MMRESULT Uninitialize() noexcept;

    EckInlineNd BOOL IsValid() noexcept
    {
        const eck::CSrwReadGuard _{ m_Lock };
        return !!m_hWaveOut;
    }

    UINT InstAdd(RefPtr<CAudioFile> pFile) noexcept
    {
        const INST_PARAM Param{ std::move(pFile) };
        return InstAdd(Param);
    }
    UINT InstAdd(const INST_PARAM& Param) noexcept;

    void InstRemove(UINT id) noexcept;

    void InstPause(UINT id) noexcept
    {
        const eck::CSrwWriteGuard _{ m_Lock };
        m_vInstance[id].eState = State::Paused;
    }
    void InstResume(UINT id) noexcept
    {
        const eck::CSrwWriteGuard _{ m_Lock };
        m_vInstance[id].eState = State::Playing;
        if (!m_bPlaying)
            m_Event.Signal();
    }

    void InstSetVolume(UINT id, BYTE byVolume) noexcept
    {
        const eck::CSrwWriteGuard _{ m_Lock };
        m_vInstance[id].byVolume = byVolume;
    }

    BYTE InstGetVolume(UINT id) noexcept
    {
        const eck::CSrwReadGuard _{ m_Lock };
        return m_vInstance[id].byVolume;
    }

    State InstGetState(UINT id) noexcept
    {
        const eck::CSrwReadGuard _{ m_Lock };
        return m_vInstance[id].eState;
    }
};