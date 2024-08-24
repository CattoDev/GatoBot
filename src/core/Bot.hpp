#pragma once

#include "Types.hpp"
#include "macro/Macro.hpp"
#include "renderer/Encoder.hpp"

#include <vector>

#include <Geode/binding/PlayLayer.hpp>

class GatoBot {
private:
    BotStatus m_status = BotStatus::Idle;
    Macro m_loadedMacro;
    RenderParams m_renderParams;
    std::vector<PlayerButtonCommand> m_queuedPlayerCommands;
    int m_currentStep = 0;
    float m_firstSPF;
    float m_mainSpeed = 1.f;
    //Encoder* m_encoder;
    std::unique_ptr<Encoder> m_encoder;

public:
    static GatoBot* get();
    static PlayLayer* getPlayLayer();

    geode::Result<> changeStatus(BotStatus newStatus);

    void setMainSpeed(float speed);
    float getMainSpeed();
    bool canPerform();
    BotStatus getStatus();
    bool isPlayback();
    void resetMacro();
    void queuePlayerCommand(const PlayerButtonCommand& cmd);
    void clearQueuedCommands();
    int getCurrentStepIdx();
    Macro& getMacro();
    RenderParams* getRenderParams();
    Encoder* getEncoder();
    void applyRenderParams(const RenderParams& params);
    geode::Result<> setupRenderer();
    void queueFrameRender();
    void toggleHook(const std::string& hookName, bool toggle);
    void updateHooks();
    void applyWinSize();
    void restoreWinSize();
    void copyVolume();
    void setVolume(float volume);
    void resetVolume();

    void updateDelta(float& dt);
    void updateBot();
    void updateCommon();
    void updateRecording();
    void updateReplaying();
    void updateRendering();

    void levelEntered(PlayLayer*);
    void levelStarted();
    void onLevelReset();
    StepState createStepState();
    void loadStepState(const StepState& state, bool clearStepsFromMacro = true);
    void botFinished(BotStatus oldStatus);
    void finishPlayback();
};