#pragma once

#include "template/PopupTemplate.hpp"
#include <core/Types.hpp>

class StatusSettingsPopup : public PopupTemplate {
public:
    BotStatus m_status;
    std::vector<CCTextInputNode*> m_inputNodes;

public:
    static StatusSettingsPopup* create(BotStatus);

    bool init(BotStatus);
    geode::prelude::CCSize createMenuForStatus(BotStatus);
    CCMenuItemSpriteExtra* createButton(const char* caption, const char* texture, float scale, geode::prelude::SEL_MenuHandler cb);
    CCTextInputNode* createInput(const char* caption, geode::prelude::CCSize size, std::string filter = "");

    int getFPS();
    const char* statusToStr(BotStatus);
    void createInputBackgrounds();
    void invalidInput();

    void onStart(CCObject*);
    void onClose(CCObject*);
};