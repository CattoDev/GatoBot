#pragma once

#include <Geode/Geode.hpp>

class PopupTemplate : public FLAlertLayer {
public:
    cocos2d::extension::CCScale9Sprite* m_bgBorder;
    cocos2d::CCLayerGradient* m_bgGradient;
    cocos2d::CCClippingNode* m_bgClipping;

public:
    virtual void show() override;

    bool init() override;
    void incrementForcePrio();
    void setBackground(cocos2d::CCSize const&);
};