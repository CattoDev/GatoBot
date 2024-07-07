#pragma once

#include <Geode/Geode.hpp>

class PopupTemplate : public FLAlertLayer {
public:
    geode::prelude::extension::CCScale9Sprite* m_bgBorder;
    geode::prelude::CCLayerGradient* m_bgGradient;
    geode::prelude::CCClippingNode* m_bgClipping;

public:
    virtual void show() override;

    bool init() override;
    void incrementForcePrio();
    void setBackground(geode::prelude::CCSize const&);
};