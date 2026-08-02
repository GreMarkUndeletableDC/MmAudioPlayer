#pragma once
#include "CAudioFile.h"

class CAudioInstance
{
public:
    enum class State : BYTE
    {
        Normal,
        Playing,
        Paused,
        Stopped,
    };
private:
    RefPtr<CAudioFile> m_pFile{};
    UINT m_idxCurrSample{};
    State m_eState{};
    BYTE m_byVolume{};
public:
    CAudioInstance(RefPtr<CAudioFile> pFile) noexcept;

    void SetState(State e) noexcept { m_eState = e; }
    State GetState() const noexcept { return m_eState; }

    void SetCurrentSampleIndex(UINT idx) noexcept { m_idxCurrSample = idx; }
    UINT GetCurrentSampleIndex() const noexcept { return m_idxCurrSample; }
    UINT GetTotalSampleCount() const noexcept
    {
        return m_pFile->GetSampleCount();
    }

    auto GetFile() const noexcept { return m_pFile; }
};