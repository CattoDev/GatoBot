#pragma once

#include <string>
#include <map>
#include <any>

class GatoBotSettings {
    std::map<std::string, std::any> settings = {};

public:
    template<typename T>
    void setOption(std::string key, T value) {
        settings[key] = value;
    }
    
    template<typename T>
    T getOption(std::string key) {
        if(settings.count(key)) {
            return std::any_cast<T>(settings[key]);
        }
        else return (T)NULL;
    }

    bool optionExists(std::string key);

    void defaultSettings();
    void save();
    void load();
};