#include "OverlayLayer.hpp"

#include "SettingsPopup.hpp"
#include "GBAlertLayer.hpp"
#include <core/Bot.hpp>

using namespace geode::prelude;

OverlayLayer* g_instance = nullptr;

OverlayLayer::~OverlayLayer() {
    g_instance = nullptr;
}

OverlayLayer* OverlayLayer::get() {
    if(!g_instance) {
        g_instance = new OverlayLayer();

        if(g_instance && g_instance->init()) {
            g_instance->autorelease();
        }
        else {
            CC_SAFE_DELETE(g_instance);
        }
    }

    return g_instance;
}

void OverlayLayer::display() {
    OverlayLayer::get()->show();
}

void OverlayLayer::close() {
    if(g_instance) { 
        g_instance->keyBackClicked();
        g_instance = nullptr;
    }
}

void OverlayLayer::show() {
    // just add to scene
    CCDirector::sharedDirector()->getRunningScene()->addChild(this, 1000);
}

void OverlayLayer::keyBackClicked() {
    PopupTemplate::keyBackClicked();

    g_instance = nullptr;
}

bool OverlayLayer::init() {
    if(!PopupTemplate::init()) 
        return false;

    auto winSize = CCDirector::get()->getWinSize();

    // GatoBot label
    {
        auto label = CCLabelBMFont::create("GatoBot", "goldFont.fnt");
        label->limitLabelWidth(100.f, 1.f, .1f);
        label->setPosition(winSize / 2 + CCPoint { 0.f, 40.f });

        m_mainLayer->addChild(label);
    }

    // add dark button menu bg
    {
        auto bg = CCScale9Sprite::create("GB_squareBG.png"_spr, { 0, 0, 20.f, 20.f });
        bg->setContentSize(CCSize { 250.f, 50.f });
        bg->setOpacity(100);
        bg->setPosition(winSize / 2);

        m_mainLayer->addChild(bg);
    }

    // add buttons
    {
        auto addButton = [this](const char* sprName, float sprScale, SEL_MenuHandler callback) {
            auto spr = CCSprite::createWithSpriteFrameName(sprName);
            spr->setScale(sprScale);
            auto btn = CCMenuItemSpriteExtra::create(spr, this, callback);

            m_buttonMenu->addChild(btn);
        };

        addButton("GB_recordBtn_002.png"_spr, 1.f, menu_selector(OverlayLayer::onRecord));
        addButton("GB_playBtn_001.png"_spr, 1.f, menu_selector(OverlayLayer::onReplay));
        addButton("GB_renderBtn_001.png"_spr, 1.f, menu_selector(OverlayLayer::onRender));
        addButton("GB_saveBtn_002.png"_spr, 1.f, menu_selector(OverlayLayer::onSave));
        addButton("GB_loadBtn_002.png"_spr, 1.f, menu_selector(OverlayLayer::onLoad));

        m_buttonMenu->alignItemsHorizontallyWithPadding(10);
    }
    
    return true;
}

void OverlayLayer::onRecord(CCObject*) {
    auto bot = GatoBot::get();

    // finish recording
    if(bot->getStatus() == BotStatus::Recording) {
        (void)bot->changeStatus(BotStatus::Idle);
        
        return;
    }

    // macro already loaded
    if(!bot->getMacro().isEmpty()) {
        GBAlertLayer::create("Warning", "<cy>Macro already loaded!</c>\nWould you like to overwrite it?", "Confirm", "Cancel", [](FLAlertLayer* alert, CCObject* pSender) {
            if(as<CCMenuItemSpriteExtra*>(pSender)->getTag() == 1) {
                SettingsPopup::create(BotStatus::Recording)->show();
            }
        })->show();
        
        return;
    }
    
    SettingsPopup::create(BotStatus::Recording)->show();
}

void OverlayLayer::onReplay(CCObject*) {
    auto bot = GatoBot::get();

    // finish replaying 
    if(bot->getStatus() == BotStatus::Replaying) {
        (void)bot->changeStatus(BotStatus::Idle);
        
        return;
    }

    // no macro loaded 
    if(GatoBot::get()->getMacro().isEmpty()) {
        GBAlertLayer::create("Error", "No macro loaded to replay!", "OK")->show();
        
        return;
    }

    SettingsPopup::create(BotStatus::Replaying)->show();
}   

void OverlayLayer::onRender(CCObject*) {
    auto bot = GatoBot::get();

    // finish rendering
    if(bot->getStatus() == BotStatus::Rendering) {
        (void)bot->changeStatus(BotStatus::Idle);
        return;
    }

    // open render settings menu
    SettingsPopup::create(BotStatus::Rendering)->show();
}

void OverlayLayer::onSave(CCObject*) {
    geode::utils::file::FilePickOptions options;
    options.filters.push_back({ "GatoBot replay file", { "*.gdr" } });

    geode::utils::file::pick(
        geode::utils::file::PickMode::SaveFile,
        options  
    ).listen(
        [](Result<std::filesystem::path>* result) {
            // failed to choose path
            if(!result->isOk()) {
                return;
            }

            std::filesystem::path filePath = result->unwrap();

            // save macro
            GatoBot::get()->getMacro().saveFile(filePath);

            GBAlertLayer::create("Success", fmt::format("Replay saved to:\n{}", filePath.string()), "OK")->show();
        }
    );
}

void OverlayLayer::onLoad(CCObject*) {
    geode::utils::file::FilePickOptions options;
    options.filters.push_back({ "GatoBot replay file", { "*.gdr" } });

    geode::utils::file::pick(
        geode::utils::file::PickMode::OpenFile,
        options
    ).listen(
        [](Result<std::filesystem::path>* result) {
            // failed to choose path
            if(!result->isOk()) {
                return;
            }

            std::filesystem::path filePath = result->unwrap();

            // load macro
            GatoBot::get()->getMacro().loadFile(filePath);

            GBAlertLayer::create("Success", fmt::format("Replay loaded:\n{}", filePath.string()), "OK")->show();
        },   
        [](auto const&){}
    );
}

void OverlayLayer::onAlert(FLAlertLayer* alert, CCObject* pSender) {
    auto btn = as<CCMenuItemSpriteExtra*>(pSender);
    const int choice = btn->getTag();

    // TODO: make this a bit better
    // this currently only handles the popup of the Record button,
    // asking if you want to overwrite the currently loaded Macro
    if(choice == 1) {
        SettingsPopup::create(BotStatus::Recording)->show();
    }
}