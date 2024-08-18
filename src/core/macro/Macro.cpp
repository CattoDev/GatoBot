#include "Macro.hpp"

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

void Macro::prepareMacro(int tps) {
    m_tps = tps;

    // allocate steps
    m_allSteps.reserve(100000);
}

void Macro::addStep(StepState& step) {
    m_allSteps.push_back(std::move(step));
}

StepState& Macro::getStep(int stepIdx) {
    return m_allSteps.at(stepIdx);
}

StepState& Macro::getLastStep() {
    return m_allSteps.back();
}

std::vector<PlayerButtonCommand> Macro::getLastButtonCommands() {
    for(int i = this->getStepCount() - 1; i >= 0; i--) {
        StepState& state = this->getStep(i);

        if(state.m_commands.size() > 0) {
            return state.m_commands;
        }
    }
    
    return {};
}

void Macro::clearStepsFrom(int stepIdx) {
    stepIdx = std::min(stepIdx, this->getStepCount());
    m_allSteps.erase(m_allSteps.begin() + stepIdx, m_allSteps.end());
}

int Macro::getStepCount() {
    return static_cast<int>(m_allSteps.size());
}

bool Macro::isEmpty() {
    return m_allSteps.size() == 0;
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
    // fix commands (lmao how does this even happen)
    // <buttontype, <<frame, cmd>>>
    std::map<PlayerButton, std::vector<std::pair<int, PlayerButtonCommand>>> commands;
    std::vector<PlayerButton> buttonTypes;

    for(auto& step : m_allSteps) {
        if(!step.m_commands.size()) continue;

        for(auto& cmd : step.m_commands) {
            if(!commands[cmd.m_button].size()) {
                buttonTypes.push_back(cmd.m_button);
            }

            commands[cmd.m_button].push_back(std::make_pair(step.m_step, cmd));
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
                    for(auto& _cmd : m_allSteps[cmdPair.first].m_commands) {
                        if(_cmd.m_button == cmd.m_button
                        && _cmd.m_isPush == cmd.m_isPush
                        && _cmd.m_isPlayer2 == cmd.m_isPlayer2
                        ) {
                            m_allSteps[cmdPair.first].m_commands.erase(m_allSteps[cmdPair.first].m_commands.begin() + iter);
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

    geode::log::debug("Macro::recordingFinished");
}

void Macro::saveFile(std::filesystem::path& filePath) {
    /*
        MACRO FORMAT:
        - [1] {4 bytes} Frame count
        - [2] {4 bytes} FPS count
        - [3] {?} Clicks

        CLICK FORMAT:
        - [1] {4 bytes} Frame
        - [2] {1 byte} Click count
        - [3] {1 byte * Click count} Click data(s)
    */
    /*std::vector<unsigned char> macroData;

    // fix path
    if(!filePath.has_extension()) {
        filePath.replace_extension(".gbb");
    }

    // frame count (constantly 4 bytes)
    addToByteVector(macroData, this->getFrameCount());

    // FPS count
    addToByteVector(macroData, m_fps);

    // process frames to clicks
    for(auto& frame : m_allFrames) {
        if(frame.m_commands.empty()) continue;

        // frame
        addToByteVector(macroData, frame.m_frame);

        // click count
        addToByteVector(macroData, static_cast<unsigned char>(frame.m_commands.size()));

        // clicks
        for(auto& cmd : frame.m_commands) {
            // pack into 1 byte
            addToByteVector(macroData, this->packAction(cmd));
        }
    }

    // write to file
    std::ofstream file(filePath, std::ios::out | std::ios::binary);

    file.write((char*)macroData.data(), macroData.size());

    file.close();*/
}

void Macro::loadFile(std::filesystem::path& filePath) {
    /*// TEMP: clear before loading
    this->clearFramesFrom(0);

    // read file
    std::ifstream file(filePath, std::ios::in | std::ios::binary);

    std::vector<unsigned char> macroData(std::istreambuf_iterator<char>(file), {});
    file.close();

    auto ptr = macroData.data();

    // frame count (first 4 bytes)
    const int frameCount = readValFromBytesRaw<int>(ptr, 0);

    // FPS count
    m_fps = readValFromBytesRaw<int>(ptr, 4);

    // allocate frames
    m_allFrames.resize(frameCount);

    for(size_t i = 0; i < frameCount; i++) {
        m_allFrames[i].m_frame = i;
    }

    // process clicks
    for(size_t i = 8; i < macroData.size();) {
        // frame
        const int frame = readValFromBytesRaw<int>(ptr, i);
        i += 4;

        const unsigned char clickCount = readValFromBytesRaw<unsigned char>(ptr, i);
        i += 1;

        for(size_t c = 0; c < clickCount; c++) {
            const auto action = readValFromBytesRaw<PackedAction>(ptr, i + c);
            i += 1;

            auto cmd = this->unpackAction(action);

            m_allFrames[frame].m_commands.push_back(cmd);
        }
    }

    geode::log::debug("Macro: loaded {} frames", m_allFrames.size());*/
}