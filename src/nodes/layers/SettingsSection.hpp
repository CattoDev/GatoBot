#pragma once

#include <Geode/Geode.hpp>

class SettingsSection : public cocos2d::CCMenu {
private:
    bool init(const char* title, const cocos2d::CCSize& menuSize);

public:
    static SettingsSection* create(const char* title, const cocos2d::CCSize& menuSize);
};