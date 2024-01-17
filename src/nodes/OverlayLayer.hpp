#pragma once

#include "template/PopupTemplate.hpp"

class OverlayLayer : public PopupTemplate {
public:
    static OverlayLayer* create();
    void show() override;

    bool init();

    void onRecord(CCObject*);
    void onReplay(CCObject*);
    void onRender(CCObject*);
    void onSave(CCObject*);
    void onLoad(CCObject*);

    void onAlert(FLAlertLayer*, CCObject*);
};