#pragma once

#include "Types.hpp"
#include "../Debug.hpp"
#include "macro/Macro.hpp"

#include <vector>

// TEMP UNTIL GEODE MEMBERS ARE REVERSE ENGINEERED
#define TEMP_MBO(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)

#include <Geode/binding/PlayLayer.hpp>

class GatoBot {
//private:
public:
    BotStatus m_status = BotStatus::Idle;
    Macro m_loadedMacro;
    int m_currentFrame = 0;
    float m_firstSPF;

public:
    static GatoBot* get();
    static PlayLayer* getPlayLayer();

    void changeStatus(BotStatus newStatus);

    int getGameFPS();
    bool canPerform();
    bool isPlayback();
    void resetMacro();
    int getCurrentFrameNum();

    bool updatePlayLayer(float& dt);
    void updateCommon(float& dt);
    void updateRecording();
    void updateReplaying();
    bool updateRendering(float& dt);

    void onLevelReset();
    void checkpointLoaded(int frame);
    void botFinished();
    void finishPlayback();
};