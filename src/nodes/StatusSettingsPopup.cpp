#include "StatusSettingsPopup.hpp"

#include <core/Bot.hpp>

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

    // create menu
    m_status = newStatus;
    CCSize layerSize = this->createMenuForStatus(newStatus);

    // bg
    this->setBackground(layerSize);

    // title label
    auto titleStr = this->statusToStr(newStatus);

    auto titleLabel = CCLabelBMFont::create(titleStr, "bigFont.fnt");
    titleLabel->setPosition(winSize / 2 + CCPoint { 0, layerSize.height / 2 - 15.f });
    titleLabel->limitLabelWidth(100, 1, .1);
    m_mainLayer->addChild(titleLabel, 1);

    // buttons
    auto applyBtn = this->createButton("Start", "GJ_button_01.png", 1, menu_selector(StatusSettingsPopup::onStart));
    applyBtn->setPosition({ -60.f, -layerSize.height / 2 + 20.f });
    m_buttonMenu->addChild(applyBtn);

    auto cancelBtn = this->createButton("Cancel", "GJ_button_06.png", 1, menu_selector(StatusSettingsPopup::onClose));
    cancelBtn->setPosition({ 60.f, -layerSize.height / 2 + 20.f });
    m_buttonMenu->addChild(cancelBtn);
    
    return true;
}

CCSize StatusSettingsPopup::createMenuForStatus(BotStatus status) {
    CCSize layerSize = status == Rendering ? CCSize { 400, 280 } : CCSize { 260, 180 };

    // Record / Replay
    if(status == Recording || status == Replaying) {
        // fps
        auto fpsInput = this->createInput("FPS", { 100, 40 }, "0123456789");
        fpsInput->setPosition({ -50.f, 35.f });
        fpsInput->setTouchEnabled(status == Recording);
        fpsInput->setString(std::to_string(this->getFPS()));

        m_buttonMenu->addChild(fpsInput);

        // speed
        auto speedInput = this->createInput("Speed", { 100, 40 }, "0123456789.");
        speedInput->setPosition({ -50.f, -15.f });
        speedInput->setString(std::to_string(GatoBot::get()->getMainSpeed()));

        m_buttonMenu->addChild(speedInput);
    }

    // Render
    if(status == Rendering) {

    }

    // finish
    this->createInputBackgrounds();

    return layerSize;
}

CCMenuItemSpriteExtra* StatusSettingsPopup::createButton(const char* text, const char* texture, float scale, SEL_MenuHandler callback) {
    auto spr = ButtonSprite::create(text, "goldFont.fnt", texture, 1);
    spr->setScale(scale);

    return CCMenuItemSpriteExtra::create(spr, this, callback);
}

CCTextInputNode* StatusSettingsPopup::createInput(const char* caption, CCSize size, std::string filter) {
    auto input = CCTextInputNode::create(size.width, size.height, caption, "bigFont.fnt");
    input->setAllowedChars(filter);

    m_inputNodes.push_back(input);

    return input;
}

int StatusSettingsPopup::getFPS() {
    if(m_status == Recording) {
        return GatoBot::get()->getGameFPS();
    }
    else {
        return GatoBot::get()->getMacro().getFPS();
    }
}

const char* StatusSettingsPopup::statusToStr(BotStatus status) {
    switch(status) {
        case Recording: {
            return "Record";
        } break;
        case Replaying: {
            return "Replay";
        } break;
        default: {
            return "Render";
        } break;
    };
}

void StatusSettingsPopup::createInputBackgrounds() {
    for(auto& input : m_inputNodes) {
        auto bg = extension::CCScale9Sprite::create("square02_small.png", { 0, 0, 40, 40 });
        bg->setPosition(m_buttonMenu->convertToWorldSpace(input->getPosition()));
        bg->setContentSize(input->getContentSize());
        bg->setOpacity(100);

        m_mainLayer->addChild(bg);
    }
}

void StatusSettingsPopup::invalidInput() {
    geode::log::error("INVALID INPUT");
}

void StatusSettingsPopup::onStart(CCObject*) {
    // verify input node data
    #define VERIFY_NODE(verFunc, node) \
        if(verFunc(node)) { this->invalidInput(); return; }

    auto VERIFY_INT = [](CCTextInputNode* inputNode) {
        char* endPtr = nullptr;
        (void)strtol(inputNode->getString().c_str(), &endPtr, 10);

        return *endPtr != '\0';
    };
    auto VERIFY_FLOAT = [](CCTextInputNode* inputNode) {
        char* endPtr = nullptr;
        (void)strtof(inputNode->getString().c_str(), &endPtr);

        return *endPtr != '\0';
    };

    VERIFY_NODE(VERIFY_INT, m_inputNodes[0]);
    VERIFY_NODE(VERIFY_FLOAT, m_inputNodes[1]);

    // set data
    auto bot = GatoBot::get();

    auto FPS = std::stoi(m_inputNodes[0]->getString());
    bot->getMacro().prepareMacro(FPS);
    bot->setGameFPS(FPS);
    bot->setMainSpeed(std::strtof(m_inputNodes[1]->getString().c_str(), nullptr));

    // start
    bot->changeStatus(m_status);

    // close layer
    this->onClose(nullptr);
}

void StatusSettingsPopup::onClose(CCObject*) {
    this->keyBackClicked();
}