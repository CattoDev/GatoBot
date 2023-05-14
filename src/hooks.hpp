#include "includes.hpp"

#define HOOKDEF(returntype, funcname, selftype, ...) \
namespace GBHooks { \
    inline returntype(__thiscall* funcname##O)(selftype, ##__VA_ARGS__); \
    returntype __fastcall funcname##H(selftype, uintptr_t, ##__VA_ARGS__); \
}

// hooks
HOOKDEF(bool, PlayLayer_init, gd::PlayLayer*, gd::GJGameLevel*);
HOOKDEF(void, PlayLayer_update, gd::PlayLayer*, float);
HOOKDEF(void, PlayLayer_removeLastCheckpoint, gd::PlayLayer*);
HOOKDEF(void, PlayLayer_resetLevel, gd::PlayLayer*);
HOOKDEF(void, PlayLayer_destroyPlayer, gd::PlayLayer*, gd::PlayerObject*, gd::GameObject*);
HOOKDEF(void, PlayLayer_levelComplete, gd::PlayLayer*);
HOOKDEF(void, PlayLayer_onQuit, gd::PlayLayer*);

HOOKDEF(void, PauseLayer_customSetup, gd::PauseLayer*);
HOOKDEF(void, GJBaseGameLayer_pushButton, gd::GJBaseGameLayer*, int, bool);
HOOKDEF(void, GJBaseGameLayer_releaseButton, gd::GJBaseGameLayer*, int, bool);
HOOKDEF(void, CCScheduler_update, CCScheduler*, float);
HOOKDEF(void, CCDirector_drawScene, CCDirector*);

namespace GBHooks {
    inline void mem_init();
}