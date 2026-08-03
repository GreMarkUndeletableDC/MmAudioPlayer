#pragma once
#include "CAudioFile.h"
#include "CAudioInstance.h"

class CAudioPlayer
{
public:
    constexpr static size_t DefaultBufferCount = 44100 * 2 * 50 / 1000;
    constexpr static size_t BufferQueueSize = 2;
private:
    HWAVEOUT m_hWaveOut{};

    std::vector<RefPtr<CAudioInstance>> m_vInstance{};
    BOOLEAN m_bPlaying{};

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

    RefPtr<CAudioInstance> AddInstance(RefPtr<CAudioFile> pFile) noexcept;

    EckInlineNdCe BOOL IsValid() const noexcept { return !!m_hWaveOut; }
};