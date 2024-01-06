#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include <nodes/OverlayLayer.hpp>

using namespace geode::prelude;

class $modify(GBPauseLayer, PauseLayer) {
    void onBotMenu(CCObject*) {
        OverlayLayer::create()->show();
    }

    void customSetup() {
        PauseLayer::customSetup();

        // add GatoBot button
        auto btnSpr = CCSprite::create("logo.png"_spr);
        btnSpr->setScale(.4f);
        auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(GBPauseLayer::onBotMenu));

        // add to menu
        CCMenu* btnMenu = nullptr;
        for(size_t i = 0; i < this->getChildrenCount(); i++) {
            if(btnMenu = typeinfo_cast<CCMenu*>(this->getChildren()->objectAtIndex(i))) {
                break;
            }
        }

        if(!btnMenu) {
            geode::log::error("Failed to find a suitable CCMenu in PauseLayer!");

            return;
        }

        btnMenu->addChild(btn);

        // set position
        auto dir = CCDirector::sharedDirector();
        auto winSize = dir->getWinSize();

        auto pos = btnMenu->convertToNodeSpace(CCPoint { winSize.width - 75, winSize.height - 35 });

        btn->setPosition(pos);
    }
};