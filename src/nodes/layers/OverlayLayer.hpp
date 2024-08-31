#pragma once

#include <nodes/template/PopupTemplate.hpp>

class OverlayLayer : public PopupTemplate {
public:
    ~OverlayLayer();

public:
    static OverlayLayer* get();
    static void display();
    static void close();
    void show() override;
    void keyBackClicked() override;

    bool init() override;
    geode::Result<> verifyMods();

    void onRecord(CCObject*);
    void onReplay(CCObject*);
    void onRender(CCObject*);
    void onSave(CCObject*);
    void onLoad(CCObject*);

    void onAlert(FLAlertLayer*, CCObject*);
};