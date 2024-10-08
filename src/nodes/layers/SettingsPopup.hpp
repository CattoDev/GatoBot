#pragma once

#include <core/Types.hpp>

#include <nodes/template/PopupTemplate.hpp>
#include <nodes/layers/SettingsSection.hpp>
#include <nodes/ui_nodes/HighlightedButton.hpp>
#include <nodes/template/SettingsLayerTemplate.hpp>

class SettingsPopup : public PopupTemplate {
public:
    enum SettingsMenuType { Video, Audio };

    BotStatus m_status;
    std::vector<CCTextInputNode*> m_inputNodes;
    std::vector<CCMenuItemToggler*> m_toggles;
    RenderParams m_renderParams;

    SettingsSection* m_checkboxesSection;
    SettingsSection* m_menuTypeSection;
    geode::ScrollLayer* m_scrollLayer;

    std::vector<SettingsSection*> m_settingsSections;
    std::vector<SettingsLayerTemplate*> m_settingsLayers;
    std::vector<HighlightedButton*> m_menuTypeButtons;
    SettingsMenuType m_selectedMenu;

public:
    static SettingsPopup* create(BotStatus);

    bool init(BotStatus);
    cocos2d::CCSize createMenuForStatus(BotStatus);
    CCMenuItemSpriteExtra* createButton(const char* caption, const char* texture, float scale, cocos2d::SEL_MenuHandler cb);
    CCTextInputNode* createInput(const char* caption, cocos2d::CCSize size, std::string filter = "");
    void createCheckbox(const char* caption, bool toggled, std::string infoText = "");

    int getFPS();
    const char* statusToStr(BotStatus);
    void applyRenderSettings(RenderParams* params);
    void createInputBackgrounds();
    void invalidInput();
    void selectMenu(SettingsMenuType menuType);

    void onMenuType(CCObject*);
    void onToggle(CCObject*);
    void onStart(CCObject*);
    void onClose(CCObject*);
};