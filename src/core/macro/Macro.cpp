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

void Macro::clearFramesFrom(int frame) {
    frame = std::min(frame, this->getFrameCount());
    m_allFrames.erase(m_allFrames.begin() + frame, m_allFrames.end());
}

int Macro::getFrameCount() {
    return static_cast<int>(m_allFrames.size());
}

float Macro::getDeltaTime() {
    return 1.f / static_cast<float>(m_fps);
}

int Macro::getFPS() {
    return m_fps;
}

bool Macro::isEmpty() {
    return m_allFrames.size() == 0;
}

Macro::PackedAction Macro::packAction(const PlayerButtonCommand& action) {
    /*
        PACKED ACTION FORMAT:
        - [1] {4 bits} Player button
        - [2] {2 bits} Holding
        - [3] {2 bits} Right side
    */
    Macro::PackedAction byte = 0;

    // add button to bits
    // 00000000 -> (button)0000;
    byte += static_cast<int>(action.m_button);
    byte = byte << 4;

    // add holding state to bits
    // (button)0000 -> (button)(is holding)00;
    char isHolding = 0;
    isHolding += action.m_isPush;
    isHolding = isHolding << 2;
    byte |= isHolding; // OR

    // add player 2 bool to bits
    // (button)(is holding)00 -> (button)(is holding)(is right side)
    byte += action.m_isPlayer2;

    return byte;
}

PlayerButtonCommand Macro::unpackAction(const Macro::PackedAction& action) {
    return PlayerButtonCommand { 
        static_cast<PlayerButton>((action & 0b11110000) >> 4),
        static_cast<bool>((action & 0b00001100) >> 2),
        static_cast<bool>(action & 0b00000011)
    };
}

void Macro::recordingFinished() {
    // fix commands
    // <buttontype, <<frame, cmd>>>
    std::map<PlayerButton, std::vector<std::pair<int, PlayerButtonCommand>>> commands;
    std::vector<PlayerButton> buttonTypes;

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
            if(cmd.m_button == lastOfKind.m_button && cmd.m_isPlayer2 == lastOfKind.m_isPlayer2) {
                // same holding state
                if(cmd.m_isPush == lastOfKind.m_isPush) {
                    // remove command
                    size_t iter = 0;
                    for(auto& _cmd : m_allFrames[cmdPair.first].m_commands) {
                        if(_cmd.m_button == cmd.m_button
                        && _cmd.m_isPush == cmd.m_isPush
                        && _cmd.m_isPlayer2 == cmd.m_isPlayer2
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
        - [5] {packed action size (1) * action count bytes} Actions
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
            addToByteVector(frameData, Macro::packAction(cmd));
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

void Macro::saveFile(const std::string& filePath) {
    /*
        MACRO FORMAT:
        - [1] {4 bytes} Frame count
        - [2] {2 bytes} FPS count
        - [3] {?} Frame datas
    */
    std::vector<unsigned char> macroData;

    // frame count (constantly 4 bytes)
    addToByteVector(macroData, this->getFrameCount());

    // FPS count
    addToByteVector(macroData, static_cast<unsigned short>(m_fps)); 

    // write frames
    for(auto& frame : m_allFrames) {
        const auto frameBytes = convertFrame(frame);

        // copy frame data
        const size_t size = frameBytes.size();
        const int offset = macroData.size();
        macroData.resize(offset + size);
        memcpy(macroData.data() + offset, frameBytes.data(), size);
    }

    // write to file
    std::ofstream file(filePath, std::ios::out | std::ios::binary);

    file.write((char*)macroData.data(), macroData.size());

    file.close();
}

void Macro::loadFile(const std::string& filePath) {
    // TEMP: clear before loading
    this->clearFramesFrom(0);

    // read file
    std::ifstream file(filePath, std::ios::in | std::ios::binary);

    std::vector<unsigned char> macroData(std::istreambuf_iterator<char>(file), {});
    file.close();

    auto ptr = macroData.data();

    // frame count (first 4 bytes)
    const int frameCount = readValFromBytesRaw<int>(ptr, 0);

    // FPS count
    m_fps = readValFromBytesRaw<unsigned short>(ptr, 4);

    // player data size
    const unsigned char playerDataSize = 16;

    // process frames
    size_t currentFrameOffset = 6;
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
                actions.push_back(Macro::unpackAction(readValFromBytesRaw<Macro::PackedAction>(ptr, currentFrameOffset)));

                currentFrameOffset++;
            }
        }

        m_allFrames.push_back({ frame, player1, player2, std::move(actions) });
    }

    GB_LOG("Macro: loaded {} frames", m_allFrames.size());
}