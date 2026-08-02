#pragma once
#include "CAudioFile.h"
#include "CAudioInstance.h"

class CAudioPlayer
{
public:
    constexpr static size_t DefaultBufferCount = 44100 * 2 * 100 / 1000;
    constexpr static size_t BufferQueueSize = 2;
private:
    HWAVEOUT m_hWaveOut{};
    std::vector<RefPtr<CAudioInstance>> m_vInstance{};
    eck::CTrivialBuffer<INT16> m_vBuffer{};
    BOOLEAN m_bPlaying{};
    WAVEHDR m_WaveHeader[BufferQueueSize]{};
    eck::CEvent m_Event{};
    eck::CWaitableObject m_Thread{};

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