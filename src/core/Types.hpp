#pragma once

#include <vector>
#include <Geode/binding/PlayerButtonCommand.hpp>

enum BotStatus { Idle, Recording, Replaying, Rendering };

struct PlayerData {
    float m_posX;
    float m_posY;
    double m_yVel;
};

struct LevelFrame {
    int m_frame;

    double m_unk1;
    int m_unk2;
    float m_unk3;

    PlayerData m_player1;
    PlayerData m_player2;

    std::vector<PlayerButtonCommand> m_commands;
};