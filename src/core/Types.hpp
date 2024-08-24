#pragma once

#include <vector>
#include <Geode/binding/PlayerButtonCommand.hpp>
#include <Geode/cocos/cocoa/CCGeometry.h>
#include <Geode/fmod/fmod_common.h>

enum class BotStatus { Idle, Recording, Replaying, Rendering };

struct PlayerData {
    float m_posX;
    float m_posY;
    double m_yVel;
};

struct StepState {
    int m_step;

    std::vector<PlayerButtonCommand> m_commands;

    // player datas
    PlayerData m_player1;
    PlayerData m_player2;
};

struct RenderParams {
    std::string m_codec;
    std::string m_outputPath;
    std::string m_songPath;

    int m_width = 0;
    int m_height = 0;
    int m_fps = 0;
    int m_videoBitrate;
    int m_audioBitrate;
    float m_audioVolume = 1.f;

    bool m_includeAudio;

    // set by the bot
    float m_timeSinceFrameRender;
    float m_spf; // seconds per frame
    float m_spt; // seconds per tick
    cocos2d::CCSize m_originalDesignRes;
    cocos2d::CCSize m_newDesignRes;

    float m_originalScreenScaleX;
    float m_originalScreenScaleY;
    float m_newScreenScaleX;
    float m_newScreenScaleY;

    float m_originalMusicVolume;
    float m_originalSFXVolume;
    FMOD_OUTPUTTYPE m_FMODOutputType;
};