#include "GatoBot.hpp"

GatoBot* instance = nullptr;

GatoBot* GatoBot::sharedState() {
    if(instance == nullptr) {
        instance = new GatoBot();
        instance->status = BotStatus::Disabled;
        instance->currentFrame = 0;
        instance->frameDelta = 0;
        instance->timeFromStart = 0;
        instance->player1holding = false;
        instance->player2holding = false;
        instance->currentPauseLayer = nullptr;
        instance->botMenu = nullptr;
        instance->gamePaused = false;
        instance->statusLabel = nullptr;
        instance->lastInfoCode = 0;
        instance->lastSPF = CCDirector::sharedDirector()->getAnimationInterval();
        instance->currentFrameHasData = false;

        instance->levelFrames.reserve(99999);

        auto frameSize = CCEGLView::sharedOpenGLView()->getFrameSize();
        instance->settings.targetWidth = static_cast<int>(frameSize.width);
        instance->settings.targetHeight = static_cast<int>(frameSize.height);
        instance->settings.targetFPS = instance->getCurrentFPS();
        instance->settings.targetGameFPS = instance->settings.targetFPS;
    }

    return instance;
}

bool GatoBot::FFmpegInstalled() {
    auto utils = CCFileUtils::sharedFileUtils();

    // what is bro cooking :skull:
    auto path = utils->fullPathForFilename("ffmpeg.exe", false);
    return strcmp(path.c_str(), "ffmpeg.exe");
}

int GatoBot::getCurrentFPS() {
    // can't wait for stuff to break
    auto dir = CCDirector::sharedDirector();
    return std::round(1.f / dir->getAnimationInterval() / dir->getScheduler()->getTimeScale());
}

void GatoBot::toggleGameFPSCap(bool toggled) {
    uintptr_t addr = (uintptr_t)cocosBaseAddr + 0xC1710;
    auto patchLoc = reinterpret_cast<void*>(addr);

    std::vector<uint8_t> bytes = {0x75, 0x24};
    if(!toggled) bytes = {0xEB, 0x24};

    patchMemory(patchLoc, bytes);

    /*auto dir = CCDirector::sharedDirector();

    if(!toggled) {
        lastSPF = dir->getAnimationInterval();
        dir->setAnimationInterval(0);
    }
    else dir->setAnimationInterval(lastSPF);*/
}

// https://github.com/matcool/small-gd-mods/blob/main/src/menu-shaders.cpp#L19
void GatoBot::patchMemory(void* patchLoc, std::vector<uint8_t> bytes) {
    DWORD old_prot;
    VirtualProtect(patchLoc, bytes.size(), PAGE_EXECUTE_READWRITE, &old_prot);
    memcpy(patchLoc, bytes.data(), bytes.size());
    VirtualProtect(patchLoc, bytes.size(), old_prot, &old_prot);
}

#define SHOWSTATUS(cstatus, format, ...) if(status == cstatus) snprintf(buf, 100, format, #cstatus, __VA_ARGS__) 

void GatoBot::updateStatusLabel() {
    if(statusLabel == nullptr) return;

    if(status == Disabled) {
        statusLabel->setString(" ");
        return;
    }

    char buf[100];

    SHOWSTATUS(Recording, "%s frame: %d", currentFrame + 1);
    SHOWSTATUS(Replaying, "%s frame: %d/%d", currentFrame, levelFrames.size());
    SHOWSTATUS(Rendering, "%s frame: %d/%d\nTime from start: %f", currentFrame + 1, levelFrames.size(), timeFromStart);

    statusLabel->setString(buf);
}

void GatoBot::retryLevel() {
    GatoBot::PauseLayer_onRetry(currentPauseLayer, nullptr);
}

void GatoBot::setSongPitch(float pitch) {
    auto fmod = gd::FMODAudioEngine::sharedEngine();
    if(fmod->isBackgroundMusicPlaying()) {
        auto channel = fmod->m_pGlobalChannel;

        __asm {
            push pitch;
            push channel;
        }

        FMOD_Channel_setPitch();
    }
}

void GatoBot::resetBasicVariables() {
    if(status == Rendering) toggleRender();
    if(status == Recording) toggleRecord();
    if(status == Replaying) toggleReplay();
}