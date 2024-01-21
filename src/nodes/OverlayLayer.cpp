#include "OverlayLayer.hpp"

#include "StatusSettingsPopup.hpp"
#include "GBAlertLayer.hpp"
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
    auto bot = GatoBot::get();

    // finish recording
    if(bot->getStatus() == BotStatus::Recording) {
        bot->changeStatus(BotStatus::Idle);
        
        return;
    }

    // macro already loaded
    if(!bot->getMacro().isEmpty()) {
        GBAlertLayer::create("Warning", "<cy>Macro already loaded!</c>\nWould you like to overwrite it?", "Confirm", "Cancel", [](FLAlertLayer* alert, CCObject* pSender) {
            if(as<CCMenuItemSpriteExtra*>(pSender)->getTag() == 1) {
                StatusSettingsPopup::create(BotStatus::Recording)->show();
            }
        })->show();
        
        return;
    }
    
    StatusSettingsPopup::create(BotStatus::Recording)->show();
}

void OverlayLayer::onReplay(CCObject*) {
    auto bot = GatoBot::get();

    // finish replaying 
    if(bot->getStatus() == BotStatus::Replaying) {
        bot->changeStatus(BotStatus::Idle);
        
        return;
    }

    // no macro loaded 
    if(GatoBot::get()->getMacro().isEmpty()) {
        GBAlertLayer::create("Error", "No macro loaded to replay!", "OK")->show();
        
        return;
    }

    StatusSettingsPopup::create(BotStatus::Replaying)->show();
}   

void OverlayLayer::onRender(CCObject*) {
    StatusSettingsPopup::create(BotStatus::Rendering)->show();
}

void OverlayLayer::onSave(CCObject*) {
    geode::utils::file::FilePickOptions options;
    options.filters.push_back({ "GatoBot replay file", { "*.gbb" } });

    auto filePath = geode::utils::file::pickFile(
        geode::utils::file::PickMode::SaveFile,
        options
    );

    if(filePath.isOk()) {
        std::string filePathStr = filePath.value().string();
        if(!filePath.value().has_extension()) {
            filePathStr.append(".gbb");
        }

        GatoBot::get()->getMacro().saveFile(filePathStr);

        GBAlertLayer::create("Success", fmt::format("Replay saved to:\n{}", filePathStr), "OK")->show();
    }
}

void OverlayLayer::onLoad(CCObject*) {
    geode::utils::file::FilePickOptions options;
    options.filters.push_back({ "GatoBot replay file", { "*.gbb" } });

    auto filePath = geode::utils::file::pickFile(
        geode::utils::file::PickMode::OpenFile,
        options
    );

    if(filePath.isOk()) {
        GatoBot::get()->getMacro().loadFile(filePath.value().string());

        GBAlertLayer::create("Success", fmt::format("Replay loaded:\n{}", filePath.value().string()), "OK")->show();
    }
}

void OverlayLayer::onAlert(FLAlertLayer* alert, CCObject* pSender) {
    auto btn = as<CCMenuItemSpriteExtra*>(pSender);
    const int choice = btn->getTag();

    if(choice == 1) {
        StatusSettingsPopup::create(BotStatus::Recording)->show();
    }
}