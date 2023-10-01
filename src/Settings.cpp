#include "Settings.hpp"

// saving system
void GatoBotSettings::save() {
    // soon?
}

void GatoBotSettings::load() {
    defaultSettings();
}

void GatoBotSettings::defaultSettings() {
    setOption("codec", std::string("h264"));
    setOption("fastRender", false);
    setOption("includeLevelSong", true);
    setOption("outputPath", std::string(""));
}

bool GatoBotSettings::optionExists(std::string key) {
    return settings.count(key) > 0;
}