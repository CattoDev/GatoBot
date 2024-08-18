#pragma once

#include "../Types.hpp"
#include <filesystem>

#include <gdr/gdr.hpp>

// why is constructor protected this format is so ass
struct gdrInput : gdr::Input {
    gdrInput() = default;

    gdrInput(int frame, PlayerButton button, bool player2, bool down) : gdr::Input(frame, static_cast<int>(button), player2, down) {} 
};
struct gdrReplay : gdr::Replay<gdrReplay, gdrInput> {
    gdrReplay();
};

class Macro {
private:
    int m_tps;
    std::vector<StepState> m_allSteps;
    gdr::Level m_levelInfo;

public:
    using ReplayFormat = gdrReplay;

public:
    void prepareMacro(int tps);

    void addStep(StepState& step);
    StepState& getStep(int stepIdx);
    StepState& getLastStep();
    std::vector<PlayerButtonCommand> getLastButtonCommands();
    void clearStepsFrom(int stepIdx);
    int getStepCount();

    bool isEmpty();

    void recordingFinished();

    void saveFile(std::filesystem::path& filePath);
    void loadFile(std::filesystem::path& filePath);
};