#include "SettingsSection.hpp"

using namespace geode::prelude;

bool SettingsSection::init(const char* title, const cocos2d::CCSize& menuSize) {
    if(!CCMenu::init()) return false;

    // background
    auto bg = extension::CCScale9Sprite::create("GB_squareBG.png"_spr, { 0, 0, 20.f, 20.f });
    bg->setContentSize(menuSize);
    bg->setOpacity(100);

    this->addChild(bg, -1);

    // title label
    if(title != nullptr) {
        auto titleLabel = CCLabelBMFont::create(title, "bigFont.fnt");
        titleLabel->limitLabelWidth(60.f, .7f, .5f);
        titleLabel->setPositionY(menuSize.height / 2 - 10.f);

        this->addChild(titleLabel, 1);
    }
    
    return true;
}

SettingsSection* SettingsSection::create(const char* title, const cocos2d::CCSize& menuSize) {
    auto pRet = new SettingsSection();

    if(pRet && pRet->init(title, menuSize)) {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}