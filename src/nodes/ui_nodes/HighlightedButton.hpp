#pragma once

#include <Geode/Geode.hpp>

class HighlightedButton : public cocos2d::CCMenuItem {
private:
    cocos2d::CCLabelBMFont* m_label;
    cocos2d::CCDrawNode* m_drawNode;
    bool m_highlighted;
    float m_labelScale;

public:
    const int ACTION_TAG = 0x8453;

private:
    bool init(const char* text, CCObject* target, cocos2d::SEL_MenuHandler callback, const cocos2d::CCSize& buttonSize);

public:
    void toggleHighlight(bool toggle);
    
    void selected() override;
    void unselected() override; 
    void activate() override; 

    static HighlightedButton* create(const char* text, CCObject* target, cocos2d::SEL_MenuHandler callback, const cocos2d::CCSize& buttonSize);
};