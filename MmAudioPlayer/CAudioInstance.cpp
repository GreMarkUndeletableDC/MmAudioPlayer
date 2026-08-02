#include "pch.h"
#include "CAudioInstance.h"

CAudioInstance::CAudioInstance(RefPtr<CAudioFile> pFile) noexcept
    : m_pFile{ std::move(pFile) }
{
}
