#include "Macro.hpp"

#include "Debug.hpp"
//#include <Geode/../../src/platform/windows/nfdwin.hpp>
#include <fstream>

// for conversions
template<typename T>
void addToByteVector(std::vector<unsigned char>& vec, T val) {
    const size_t size = sizeof(T);
    const int offset = vec.size();
    vec.resize(offset + size);
    memcpy(vec.data() + offset, (char*)&val, size);
}

template<typename T>
T readValFromBytesRaw(unsigned char* vec, size_t index) {
    T val = 0;
    
    const size_t size = sizeof(T);
    memcpy((char*)&val, vec + index, size);

    return val;
}

///////////

void Macro::prepareMacro(int fps) {
    m_fps = fps;

    // allocate frames
    m_allFrames.reserve(100000);
}

void Macro::addFrame(LevelFrame& frame) {
    m_allFrames.push_back(std::move(frame));
}

LevelFrame& Macro::getFrame(int frame) {
    return m_allFrames[frame];
}

void Macro::clearFramesAfter(int frame) {
    //GB_LOG("Macro::clearFramesAfter {} ({})", frame, m_allFrames.size());

    frame = std::min(frame, this->getFrameCount());
    m_allFrames.erase(m_allFrames.begin() + frame, m_allFrames.end());
}

int Macro::getFrameCount() {
    return static_cast<int>(m_allFrames.size());
}

float Macro::getDeltaTime() {
    return 1.f / static_cast<float>(m_fps);
}

void Macro::recordingFinished() {
    // fix commands
    // <buttontype, <<frame, cmd>>>
    std::map<int, std::vector<std::pair<int, PlayerButtonCommand>>> commands;
    std::vector<int> buttonTypes;

    for(auto& frame : m_allFrames) {
        if(!frame.m_commands.size()) continue;

        for(auto& cmd : frame.m_commands) {
            if(!commands[cmd.m_button].size()) {
                buttonTypes.push_back(cmd.m_button);
            }

            commands[cmd.m_button].push_back(std::make_pair(frame.m_frame, cmd));
        }
    }

    for(auto& buttonType : buttonTypes) {
        auto pairs = commands[buttonType];

        PlayerButtonCommand lastOfKind = pairs[0].second;
        for(size_t i = 1; i < pairs.size(); i++) {
            auto& cmdPair = pairs[i];
            PlayerButtonCommand cmd = cmdPair.second;

            // same kind
            if(cmd.m_button == lastOfKind.m_button && cmd.m_rightSide == lastOfKind.m_rightSide) {
                // same holding state
                if(cmd.m_holding == lastOfKind.m_holding) {
                    // remove command
                    size_t iter = 0;
                    for(auto& _cmd : m_allFrames[cmdPair.first].m_commands) {
                        if(_cmd.m_button == cmd.m_button
                        && _cmd.m_holding == cmd.m_holding
                        && _cmd.m_rightSide == cmd.m_rightSide
                        ) {
                            m_allFrames[cmdPair.first].m_commands.erase(m_allFrames[cmdPair.first].m_commands.begin() + iter);
                            break;
                        }
                        iter++;
                    }
                }
                else {
                    lastOfKind = cmd;
                }
            }
        }
    }

    GB_LOG("Macro::recordingFinished");
}

std::vector<unsigned char> convertFrame(const LevelFrame& frame) {
    /*
        FRAME FORMAT:
        - [1] {4 bytes} Frame index
        - [2] {Player data size} Player 1 data
        - [3] {Player data size} Player 2 data
        - [4] {1 byte} Action count
        - [5] {action size (3) * action count} Actions
    */
    std::vector<unsigned char> frameData;

    // frame index
    addToByteVector(frameData, frame.m_frame);

    // player datas
    auto addPlayerData = [&frameData](const PlayerData& playerData) {
        // position
        addToByteVector(frameData, playerData.m_posX);
        addToByteVector(frameData, playerData.m_posY);

        // velocity
        addToByteVector(frameData, playerData.m_yVel);
    };

    addPlayerData(frame.m_player1);
    addPlayerData(frame.m_player2);

    // action count
    const unsigned char actionCount = static_cast<unsigned char>(frame.m_commands.size());
    addToByteVector(frameData, actionCount);

    // actions
    if(actionCount > 0) {
        for(auto& cmd : frame.m_commands) {
            addToByteVector(frameData, static_cast<unsigned char>(cmd.m_button));
            addToByteVector(frameData, static_cast<unsigned char>(cmd.m_holding));
            addToByteVector(frameData, static_cast<unsigned char>(cmd.m_rightSide));
        }
    }

    return frameData;
}

PlayerData playerDataFromBytes(unsigned char* playerBytes, const size_t& size) {
    PlayerData data;

    // position
    if(size >= 8) {
        data.m_posX = readValFromBytesRaw<float>(playerBytes, 0);
        data.m_posY = readValFromBytesRaw<float>(playerBytes, 4);
    }
    // velocity
    if(size >= 16) {
        data.m_yVel = readValFromBytesRaw<double>(playerBytes, 8);
    }

    return data;
}

void Macro::saveFile(std::string filePath) {
    /*
        MACRO FORMAT:
        - [1] {4 bytes} Frame count
        - [2] {2 bytes} FPS count
        - [3] {1 byte} Player data size
        - [4] {?} Frame datas
    */
    std::vector<unsigned char> macroData;

    // frame count (constantly 4 bytes)
    addToByteVector(macroData, this->getFrameCount());

    // FPS count
    addToByteVector(macroData, static_cast<unsigned short>(m_fps));

    // player data size (VERY prone to change)
    const unsigned char playerDataSize = sizeof(PlayerData);
    macroData.push_back(playerDataSize); 

    // write frames
    for(auto& frame : m_allFrames) {
        const auto frameBytes = convertFrame(frame);

        // TODO: CHANGE TO MEMCPY
        for(auto& byte : frameBytes) {
            macroData.push_back(byte);
        }
    }

    // write to file
    std::ofstream file(filePath, std::ios::out | std::ios::binary);

    file.write((char*)macroData.data(), macroData.size());

    file.close();
}

void Macro::loadFile(std::string filePath) {
    // TEMP: clear before loading
    this->clearFramesAfter(0);

    // read file
    std::ifstream file(filePath, std::ios::in | std::ios::binary);

    std::vector<unsigned char> macroData(std::istreambuf_iterator<char>(file), {});
    file.close();

    auto ptr = macroData.data();

    // frame count (first 4 bytes)
    const int frameCount = readValFromBytesRaw<int>(ptr, 0);

    // FPS count
    m_fps = readValFromBytesRaw<unsigned short>(ptr, 4);

    // player data size (5th byte)
    const unsigned char playerDataSize = readValFromBytesRaw<unsigned char>(ptr, 6);

    // process frames
    size_t currentFrameOffset = 7;
    for(size_t currentFrame = 0; currentFrame < frameCount; currentFrame++) {
        // frame index
        const int frame = readValFromBytesRaw<int>(ptr, currentFrameOffset);
        currentFrameOffset += 0x4;

        // player 1 data
        const auto player1 = playerDataFromBytes(ptr + currentFrameOffset, playerDataSize);
        currentFrameOffset += playerDataSize;

        // player 2 data
        const auto player2 = playerDataFromBytes(ptr + currentFrameOffset, playerDataSize);
        currentFrameOffset += playerDataSize;

        // get action count
        const unsigned char actionCount = readValFromBytesRaw<unsigned char>(ptr, currentFrameOffset);
        currentFrameOffset += 0x1;

        // get actions
        std::vector<PlayerButtonCommand> actions;
        if(actionCount > 0) {
            for(size_t i = 0; i < actionCount; i++) {
                PlayerButtonCommand cmd;

                cmd.m_button = readValFromBytesRaw<unsigned char>(ptr, currentFrameOffset);
                cmd.m_holding = readValFromBytesRaw<unsigned char>(ptr, currentFrameOffset + 1);
                cmd.m_rightSide = readValFromBytesRaw<unsigned char>(ptr, currentFrameOffset + 2);

                actions.push_back(std::move(cmd));

                currentFrameOffset += 0x3;
            }
        }

        m_allFrames.push_back({ frame, player1, player2, std::move(actions) });
    }

    GB_LOG("Macro: loaded {} frames", m_allFrames.size());
}