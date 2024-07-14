#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

//#include <core/Bot.hpp>
#include <nodes/layers/OverlayLayer.hpp>

using namespace geode::prelude;

class $modify(GBPauseLayer, PauseLayer) {
    void onBotMenu(CCObject*) {
        OverlayLayer::display();
    }

    void customSetup() {
        // setup PauseLayer
        PauseLayer::customSetup();

        // add GatoBot button
        auto btnSpr = CCSprite::createWithSpriteFrameName("GB_logo.png"_spr);
        btnSpr->setScale(.4f);
        auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(GBPauseLayer::onBotMenu));

        // add to menu
        CCMenu* btnMenu = nullptr;
        for(size_t i = 0; i < this->getChildrenCount(); i++) {
            btnMenu = typeinfo_cast<CCMenu*>(this->getChildren()->objectAtIndex(i));

            if(btnMenu) break;
        }

        if(!btnMenu) {
            geode::log::error("Failed to find a suitable CCMenu in PauseLayer!");

            return;
        }

        // TODO: find a better way to add the button lol
        auto settingsBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(btnMenu->getChildren()->objectAtIndex(btnMenu->getChildrenCount() - 1));
        auto settingsBtnPos = settingsBtn->getPosition();

        btnMenu->addChild(btn);

        // set position
        //auto dir = CCDirector::sharedDirector();
        //auto winSize = dir->getWinSize();
        //auto pos = btnMenu->convertToNodeSpace(CCPoint { winSize.width - 75, winSize.height - 35 });

        btn->setPosition(settingsBtnPos + CCPoint { -40.f, 0.f });
    }
};