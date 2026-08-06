#pragma once
#include "CAudioFile.h"

class CAudioInstance
{
    friend class CAudioPlayer;
public:
    enum class State : BYTE
    {
        Normal,
        Playing,
        Paused,
        Stopped,
    };
private:
    CAudioPlayer* m_pPlayer{};

    RefPtr<CAudioFile> m_pFile{};
    UINT m_idxCurrSample{};
    State m_eState{};
    BYTE m_byVolume{};
public:
    CAudioInstance(RefPtr<CAudioFile> pFile) noexcept
        : m_pFile{ std::move(pFile) }
    {
        m_pFile->Select();
    }
private:
    EckInlineNd static RefPtr<CAudioInstance> Make(RefPtr<CAudioFile> pFile) noexcept
    {
        return RefPtr<CAudioInstance>::Make(std::move(pFile));
    }

    EckInlineNdCe auto& State() noexcept { return m_eState; }
    EckInlineNdCe auto& CurrentSampleIndex() noexcept { return m_idxCurrSample; }

public:
    ~CAudioInstance()
    {
        m_pFile->Deselect();
    }

    EckInlineNdCe auto& GetFile() const noexcept { return m_pFile; }
};