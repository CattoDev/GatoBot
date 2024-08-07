#pragma once

#include <vector>
#include <Geode/binding/PlayerButtonCommand.hpp>
#include <Geode/cocos/cocoa/CCGeometry.h>
#include <Geode/fmod/fmod_common.h>

enum BotStatus { Idle, Recording, Replaying, Rendering };

struct PlayerData {
    float m_posX;
    float m_posY;
    double m_yVel;
};

struct TPSLockData {
    double m_val1;
    int m_val2;
    float m_val3;
};

struct FrameState {
    int m_frame;
    
    // shit that influences frame delta idk what to name these
    double m_unk1;
    int m_unk2;
    float m_unk3;
};

struct LevelFrame {
    int m_frame;

    // PlayerData m_player1;
    // PlayerData m_player2;

    //gd::vector<PlayerButtonCommand> m_commands;
    std::vector<PlayerButtonCommand> m_commands;

    // only used in recording
    FrameState m_frameState;
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
    int m_frameFactor;
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