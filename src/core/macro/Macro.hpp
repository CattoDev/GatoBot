#pragma once

#include "../Types.hpp"
#include <filesystem>

class Macro {
private:
    int m_tps;
    std::vector<StepState> m_allSteps;

public:
    using PackedAction = unsigned char; // 1 byte

public:
    void prepareMacro(int tps);

    void addStep(StepState& step);
    StepState& getStep(int stepIdx);
    StepState& getLastStep();
    std::vector<PlayerButtonCommand> getLastButtonCommands();
    void clearStepsFrom(int stepIdx);
    int getStepCount();

    bool isEmpty();

    static PackedAction packAction(const PlayerButtonCommand& action);
    static PlayerButtonCommand unpackAction(const PackedAction& action);
    void recordingFinished();

    void saveFile(std::filesystem::path& filePath);
    void loadFile(std::filesystem::path& filePath);
};