#include "StatusSettingsPopup.hpp"

using namespace geode::prelude;

StatusSettingsPopup* StatusSettingsPopup::create(BotStatus status) {
    auto pRet = new StatusSettingsPopup();

    if(pRet && pRet->init(status)) {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}

bool StatusSettingsPopup::init(BotStatus newStatus) {
    if(!PopupTemplate::init())
        return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    // background
    auto bg = extension::CCScale9Sprite::create("GJ_square02.png", { 0.f, 0.f, 80.f, 80.f });
    bg->setPosition(winSize / 2);
    bg->setContentSize({ 180, 160 });
    m_mainLayer->addChild(bg);
    
    return true;
}