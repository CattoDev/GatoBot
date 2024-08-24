#include "Macro.hpp"

#include <fstream>
#include <Geode/Geode.hpp>
#include <core/Bot.hpp>

using namespace geode::prelude;

gdrReplay::gdrReplay() : Replay("", "") {
    auto mod = Mod::get();
    const auto botName = mod->getName();
    const auto botVersion = mod->getVersion().toNonVString(false);

    botInfo.name = botName;
    botInfo.version = botVersion;
}

void Macro::prepareMacro(int tps) {
    m_tps = tps;

    // allocate steps
    //m_allSteps.reserve(100000);
}

void Macro::addStep(const StepState& step) {
    m_allSteps.push_back(step);
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

int Macro::getTPS() {
    return m_tps;
}

bool Macro::isEmpty() {
    return m_allSteps.size() == 0;
}

void Macro::recordingFinished() {
    // set level info
    if(auto pLayer = GatoBot::get()->getPlayLayer()) {
        m_levelInfo.id = pLayer->m_level->m_levelID;
        m_levelInfo.name = pLayer->m_level->m_levelName;
    }

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
                    std::vector<PlayerButtonCommand>& cmds = m_allSteps[cmdPair.first].m_commands;

                    auto pos = std::remove_if(cmds.begin(), cmds.end(), [&cmd](PlayerButtonCommand& _cmd) {
                        return _cmd.m_button == cmd.m_button
                            && _cmd.m_isPush == cmd.m_isPush
                            && _cmd.m_isPlayer2 == cmd.m_isPlayer2
                        ;
                    });
                    cmds.erase(pos, cmds.end());
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
    // fix path
    if(!filePath.has_extension()) {
        filePath.replace_extension(".gdr");
    }

    // create gdr macro
    ReplayFormat replay;

    // convert to commands
    for(int stepIdx = 0; stepIdx < this->getStepCount(); stepIdx++) {
        auto& step = m_allSteps.at(stepIdx);

        if(step.m_commands.size() == 0) continue;

        for(auto& cmd : step.m_commands) {
            replay.inputs.push_back(gdrInput { step.m_step, cmd.m_button, cmd.m_isPlayer2, cmd.m_isPush });
        }
    } 

    // set other info
    replay.levelInfo = m_levelInfo;
    replay.framerate = static_cast<float>(m_tps);
    replay.duration = static_cast<float>(this->getStepCount()) / replay.framerate;
    replay.gameVersion = 2.206;

    // export
    auto macroData = replay.exportData(true);
    std::ofstream file(filePath, std::ios::out | std::ios::binary);
    file.write(reinterpret_cast<const char*>(macroData.data()), macroData.size());

    file.close();
}

void Macro::loadFile(std::filesystem::path& filePath) {
    this->clearStepsFrom(0);

    // read file
    std::ifstream file(filePath, std::ios::in | std::ios::binary);

    std::vector<unsigned char> macroData(std::istreambuf_iterator<char>(file), {});
    file.close();

    // load macro
    ReplayFormat replay = ReplayFormat::importData(macroData);
    m_tps = static_cast<int>(replay.framerate);

    // create step states from commands
    int stepCount = static_cast<int>(replay.duration * replay.framerate);

    m_allSteps.resize(stepCount);
    for(int i = 0; i < stepCount; i++) {
        m_allSteps.at(i).m_step = i;
    }

    for(auto& input : replay.inputs) {
        auto& state = m_allSteps.at(input.frame);
        
        state.m_commands.push_back(PlayerButtonCommand { static_cast<PlayerButton>(input.button), input.down, input.player2 });
    }

    geode::log::debug("Macro: loaded {} steps", this->getStepCount());
}