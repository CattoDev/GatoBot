#include "OverlayLayer.hpp"

#include "StatusSettingsPopup.hpp"
#include <core/Bot.hpp>

using namespace geode::prelude;

OverlayLayer* OverlayLayer::create() {
    auto pRet = new OverlayLayer();

    if(pRet && pRet->init()) {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}

void OverlayLayer::show() {
    // just add to scene
    CCDirector::sharedDirector()->getRunningScene()->addChild(this, 1000);
}

bool OverlayLayer::init() {
    if(!PopupTemplate::init()) 
        return false;

    // add buttons
    auto addButton = [this](const char* sprName, float sprScale, SEL_MenuHandler callback) {
        auto spr = CCSprite::create(sprName);
        spr->setScale(sprScale);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, callback);

        m_buttonMenu->addChild(btn);
    };

    // temp buttons
    addButton("GJ_button_02.png", 1.f, menu_selector(OverlayLayer::onRecord));
    addButton("GJ_button_02.png", 1.f, menu_selector(OverlayLayer::onReplay));
    addButton("GJ_button_02.png", 1.f, menu_selector(OverlayLayer::onRender));
    addButton("GJ_button_02.png", 1.f, menu_selector(OverlayLayer::onSave));
    addButton("GJ_button_02.png", 1.f, menu_selector(OverlayLayer::onLoad));

    m_buttonMenu->alignItemsHorizontallyWithPadding(10);
    
    return true;
}

void OverlayLayer::onRecord(CCObject*) {
    StatusSettingsPopup::create(BotStatus::Recording)->show();
}

void OverlayLayer::onReplay(CCObject*) {

}

void OverlayLayer::onRender(CCObject*) {

}

void OverlayLayer::onSave(CCObject*) {

}

void OverlayLayer::onLoad(CCObject*) {

}