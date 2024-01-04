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

    PlayerData m_player1;
    PlayerData m_player2;

    std::vector<PlayerButtonCommand> m_commands;
};