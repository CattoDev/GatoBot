#pragma once

#include "Types.hpp"
#include "../Debug.hpp"
#include "macro/Macro.hpp"
#include "renderer/Encoder.hpp"

#include <vector>

// TEMP UNTIL GEODE MEMBERS ARE REVERSE ENGINEERED
#define TEMP_MBO(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)

#include <Geode/binding/PlayLayer.hpp>

class GatoBot {
private:
    BotStatus m_status = BotStatus::Idle;
    Macro m_loadedMacro;
    RenderParams m_renderParams;
    int m_currentFrame = 0;
    float m_firstSPF;
    float m_mainSpeed = 1.f;
    Encoder* m_encoder;

public:
    static GatoBot* get();
    static PlayLayer* getPlayLayer();

    geode::Result<> changeStatus(BotStatus newStatus);

    int getGameFPS();
    void setGameSPF(double spf);
    void setGameFPS(int fps);
    void setMainSpeed(float speed);
    float getMainSpeed();
    bool canPerform();
    BotStatus getStatus();
    bool isPlayback();
    void resetMacro();
    int getCurrentFrameNum();
    Macro& getMacro();
    RenderParams* getRenderParams();
    void applyRenderParams(const RenderParams& params);
    geode::Result<> setupRenderer();
    void toggleHook(const std::string& hookName, bool toggle);
    void updateHooks();

    void updatePlayLayer(float& dt);
    void updateCommon(float& dt);
    void updateRecording();
    void updateReplaying();
    void updateRendering();

    void onLevelReset();
    FrameState createFrameState();
    FrameState& getLastFrameState();
    void loadFrameState(const FrameState& state, bool clearFrames = true);
    void botFinished(BotStatus oldStatus);
    void finishPlayback();
};