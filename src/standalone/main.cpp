#include "../GatoBot.hpp"

// load
DWORD WINAPI ModThread(void* hModule) {
    // debug console
    #ifdef GB_DEBUG
    if(AllocConsole()) {
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    }
    #endif

    // MinHook
    MH_Initialize();

    // GatoBot
    GatoBot::sharedState()->setupBot();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        auto h = CreateThread(0, 0, ModThread, handle, 0, 0);
        if (h)
            CloseHandle(h);
        else
            return FALSE;
    }
    return TRUE;
}