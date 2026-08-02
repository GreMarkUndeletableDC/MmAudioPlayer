#include "pch.h"

#include "eck\AutoLink.h"
#pragma comment(lib, "Msacm32.lib")

#include "CAudioPlayer.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR pszCmdLine, _In_ int nCmdShow)
{
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
    const auto hr = CoInitialize(nullptr);
    if (FAILED(hr))
        return 0;

    eck::INITPARAM ip{};
    ip.uFlags = eck::EIF_NOINITD2D | eck::EIF_NOINITDWRITE;
    UINT uErr;
    const auto eInitRet = eck::Initialize(hInstance, &ip, &uErr);
    if (eInitRet != eck::StartupStatus::Ok)
        return 0;



    eck::ThreadUninitialize();
    eck::Uninitialize();
    CoUninitialize();
    return 0;
}