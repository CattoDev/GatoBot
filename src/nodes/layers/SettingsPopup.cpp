#include "SettingsPopup.hpp"
#include "OverlayLayer.hpp"
#include "GBAlertLayer.hpp"

#include "VideoSettingsLayer.hpp"
#include "AudioSettingsLayer.hpp"

#include <core/Bot.hpp>

using namespace geode::prelude;

SettingsPopup* SettingsPopup::create(BotStatus status) {
    auto pRet = new SettingsPopup();

    if(pRet && pRet->init(status)) {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}

bool SettingsPopup::init(BotStatus newStatus) {
    if(!PopupTemplate::init())
        return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    m_renderParams = *(GatoBot::get()->getRenderParams());

    // buttons
    auto applyBtn = this->createButton("Start", "GJ_button_01.png", 1, menu_selector(SettingsPopup::onStart));
    m_buttonMenu->addChild(applyBtn);

    auto cancelBtn = this->createButton("Cancel", "GJ_button_06.png", 1, menu_selector(SettingsPopup::onClose));
    m_buttonMenu->addChild(cancelBtn);

    m_buttonMenu->alignItemsHorizontallyWithPadding(10.f);

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

    // readjust buttons
    applyBtn->setPositionY(-layerSize.height / 2 + 20.f);
    cancelBtn->setPositionY(-layerSize.height / 2 + 20.f);
    
    return true;
}

CCSize SettingsPopup::createMenuForStatus(BotStatus status) {
    CCSize layerSize = status == BotStatus::Rendering ? CCSize { 440, 280 } : CCSize { 260, 180 };

    // Record / Replay
    if(status == BotStatus::Recording || status == BotStatus::Replaying) {
        // fps
        auto fpsLabel = CCLabelBMFont::create("FPS:", "bigFont.fnt");
        fpsLabel->setAnchorPoint(CCPoint { 1.f, .5f });
        fpsLabel->setPosition({ -10.f, 35.f });

        m_buttonMenu->addChild(fpsLabel);

        auto fpsInput = this->createInput("TPS", { 100, 40 }, "0123456789");
        fpsInput->setPosition({ 50.f, 35.f });
        //fpsInput->setTouchEnabled(status == BotStatus::Recording);
        fpsInput->setTouchEnabled(false); // disable TPS input for now
        fpsInput->setLabelNormalColor(ccColor3B { 100, 100, 100 });
        
        fpsInput->setString(std::to_string(this->getFPS()));

        m_buttonMenu->addChild(fpsInput);

        // speed
        auto speedLabel = CCLabelBMFont::create("Speed:", "bigFont.fnt");
        speedLabel->setAnchorPoint(CCPoint { 1.f, .5f });
        speedLabel->setPosition({ -10.f, -15.f });
        speedLabel->limitLabelWidth(90.f, 1.f, .1f);

        m_buttonMenu->addChild(speedLabel);

        auto speedInput = this->createInput("Speed", { 100, 40 }, "0123456789.");
        speedInput->setPosition({ 50.f, -15.f });
        speedInput->setString(std::to_string(GatoBot::get()->getMainSpeed()));

        m_buttonMenu->addChild(speedInput);
    }

    // Render
    if(status == BotStatus::Rendering) {
        // checkboxes section
        {
            m_checkboxesSection = SettingsSection::create("Options", { 100.f, layerSize.height - 70.f });
            m_checkboxesSection->setPosition({ -160.f, 5.f });

            m_buttonMenu->addChild(m_checkboxesSection);

            this->createCheckbox("Include audio", true, "Include game audio in the exported video.");
            this->createCheckbox("Rendering info", true, "Display rendering info on screen.\n(Not shown in the exported video!)");
            this->createCheckbox("Multithreaded\nencoder", true, "Utilize all CPU cores on encoding.");
        }

        // settings menu buttons
        {
            const float sectionWidth = 210.f;
            m_menuTypeSection = SettingsSection::create(nullptr, { sectionWidth, 20.f });
            m_menuTypeSection->setPosition({ 0, layerSize.height / 2 - 40.f });

            m_buttonMenu->addChild(m_menuTypeSection);

            const float buttonWidth = 80.f;

            // video settings button
            auto videoBtn = HighlightedButton::create("Video Settings", this, menu_selector(SettingsPopup::onMenuType), { buttonWidth, 20.f });
            videoBtn->setTag(SettingsMenuType::Video);
            m_menuTypeButtons.push_back(videoBtn);

            m_menuTypeSection->addChild(videoBtn);

            // audio settings button
            auto audioBtn = HighlightedButton::create("Audio Settings", this, menu_selector(SettingsPopup::onMenuType), { buttonWidth, 20.f });
            audioBtn->setTag(SettingsMenuType::Audio);
            m_menuTypeButtons.push_back(audioBtn);

            m_menuTypeSection->addChild(audioBtn);

            // align buttons
            const int btnCount = m_menuTypeButtons.size();
            for(size_t i = 0; i < btnCount; i++) {
                m_menuTypeButtons[i]->setPositionX(-buttonWidth / 2 + i * buttonWidth);
            }
        }

        // settings menus
        {
            CCSize settingsSectionSize = { layerSize.width - 130.f, layerSize.height - 95.f };

            auto scrollMenu = CCMenu::create(); // create a menu inside ScrollLayer so it doesn't fuck up the settings layers
            scrollMenu->setPosition(CCPointZero);

            auto settingsSection = SettingsSection::create(nullptr, settingsSectionSize);
            settingsSection->setPosition(CCPoint { 50.f, -7.5f });
            m_settingsSections.push_back(settingsSection);

            m_buttonMenu->addChild(settingsSection);

            // scroll layer
            CCSize scrollSize { settingsSectionSize.width, settingsSectionSize.height - .5f }; 
            CCRect scrollRect {
                -scrollSize.width / 2,
                -scrollSize.height / 2,
                scrollSize.width,
                scrollSize.height
            };
            m_scrollLayer = ScrollLayer::create(scrollRect, true, true);
            m_scrollLayer->m_contentLayer->addChild(scrollMenu);
            settingsSection->addChild(m_scrollLayer);

            // video settings
            {
                CCSize settingsLayerSize = settingsSectionSize + CCSize { 0.f, 30.f };

                auto settings = VideoSettingsLayer::create(&m_renderParams, settingsLayerSize);
                settings->setPosition(settingsLayerSize / 2);
                settings->setTag(SettingsMenuType::Video);
                m_settingsLayers.push_back(settings);

                scrollMenu->addChild(settings);
            }

            // audio settings
            {
                CCSize settingsLayerSize = settingsSectionSize; //+ CCSize { 0.f, 30.f };

                auto settings = AudioSettingsLayer::create(&m_renderParams, settingsLayerSize);
                settings->setPosition(settingsLayerSize / 2);
                settings->setTag(SettingsMenuType::Audio);
                m_settingsLayers.push_back(settings);

                scrollMenu->addChild(settings);
            }
        }

        // default is video settings
        this->selectMenu(SettingsMenuType::Video);
    }

    // finish
    this->applyRenderSettings(&m_renderParams);
    this->createInputBackgrounds();

    return layerSize;
}

CCMenuItemSpriteExtra* SettingsPopup::createButton(const char* text, const char* texture, float scale, SEL_MenuHandler callback) {
    auto spr = ButtonSprite::create(text, "goldFont.fnt", texture, 1);
    spr->setScale(scale);

    return CCMenuItemSpriteExtra::create(spr, this, callback);
}

CCTextInputNode* SettingsPopup::createInput(const char* caption, CCSize size, std::string filter) {
    auto input = CCTextInputNode::create(size.width, size.height, caption, "bigFont.fnt");
    input->setAllowedChars(filter);

    m_inputNodes.push_back(input);

    return input;
}

void SettingsPopup::createCheckbox(const char* caption, bool toggled, std::string infoText) {
    auto secSize = m_checkboxesSection->getContentSize();
    CCPoint topLeft = { -secSize.width / 2 + 15.f, secSize.height / 2 - 35.f };
    
    // create the checkbox
    auto toggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(SettingsPopup::onToggle), .6f);
    toggle->setPosition(topLeft + CCPoint { 0.f, -25.f * static_cast<float>(m_toggles.size()) });
    toggle->toggle(toggled);
    
    m_checkboxesSection->addChild(toggle);
    m_toggles.push_back(toggle);

    // create caption label
    auto label = CCLabelBMFont::create(caption, "bigFont.fnt");
    label->setAnchorPoint(CCPoint { 0.f, .5f });
    label->setPosition(toggle->getPosition() + CCPoint { 15.f, 0.f });
    label->limitLabelWidth(65.f, .4f, .1f);

    m_checkboxesSection->addChild(label);
}

int SettingsPopup::getFPS() {
    /*if(m_status == Recording) {
        return GatoBot::get()->getGameFPS();
    }
    else {
        return GatoBot::get()->getMacro().getFPS();
    }*/

    //return 240;
    return m_renderParams.m_tps;
}

const char* SettingsPopup::statusToStr(BotStatus status) {
    switch(status) {
        case BotStatus::Recording: {
            return "Record";
        } break;
        case BotStatus::Replaying: {
            return "Replay";
        } break;
        default: {
            return "Render";
        } break;
    };
}

void SettingsPopup::applyRenderSettings(RenderParams* params) {
    if(m_status != BotStatus::Rendering) return;

    auto videoSettings = typeinfo_cast<VideoSettingsLayer*>(m_settingsLayers.at(0));
    auto audioSettings = typeinfo_cast<AudioSettingsLayer*>(m_settingsLayers.at(1));
    auto preset = videoSettings->getPresets().at(2);

    // default settings
    if(params->m_width <= 0 || params->m_height <= 0 || params->m_fps <= 0) {
        params->m_width = preset.m_width;
        params->m_height = preset.m_height;
        params->m_fps = preset.m_fps;
        params->m_videoBitrate = preset.m_videoBitrate;
        params->m_audioBitrate = 128;
        params->m_codec = "libx264";
    }

    // apply settings
    videoSettings->applyRenderParams();
    audioSettings->applyRenderParams();
}

void SettingsPopup::createInputBackgrounds() {
    for(auto& input : m_inputNodes) {
        auto bg = CCScale9Sprite::create("square02_small.png", CCRect { 0.f, 0.f, 40.f, 40.f });
        bg->setPosition(m_buttonMenu->convertToWorldSpace(input->getPosition()));
        bg->setContentSize(input->getContentSize());
        bg->setOpacity(100);

        m_mainLayer->addChild(bg);
    }
}

void SettingsPopup::invalidInput() {
    geode::log::error("INVALID INPUT");
}

void SettingsPopup::selectMenu(SettingsMenuType menuType) {
    // toggle the correct layer
    for(auto& layer : m_settingsLayers) {
        bool curLayer = menuType == layer->getTag();

        layer->setVisible(curLayer);
        layer->setTouchEnabled(curLayer);

        if(curLayer) {
            m_scrollLayer->m_contentLayer->setContentSize(layer->getContentSize());
            m_scrollLayer->scrollToTop();
        }
    }


    // highlight the correct button
    for(auto& btn : m_menuTypeButtons) {
        btn->toggleHighlight(menuType == btn->getTag());
    }
}

void SettingsPopup::onMenuType(CCObject* sender) {
    const int tag = typeinfo_cast<HighlightedButton*>(sender)->getTag();

    this->selectMenu(static_cast<SettingsMenuType>(tag));
}

void SettingsPopup::onToggle(CCObject* sender) {
    const int tag = typeinfo_cast<CCMenuItemToggler*>(sender)->getTag();


}

void SettingsPopup::onStart(CCObject*) {
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

    // apply settings
    auto bot = GatoBot::get();
    bool settingsApplied = true;

    if(m_status == BotStatus::Recording || m_status == BotStatus::Replaying) {
        // Recording / Replaying settings
        VERIFY_NODE(VERIFY_INT, m_inputNodes[0]);
        VERIFY_NODE(VERIFY_FLOAT, m_inputNodes[1]);

        auto FPS = std::stoi(m_inputNodes[0]->getString());

        //bot->getRenderParams()->m_tps = FPS;
        bot->resetMacro();
        bot->setMainSpeed(std::strtof(m_inputNodes[1]->getString().c_str(), nullptr));
    }
    else {
        // Render settings
        for(auto& layer : m_settingsLayers) {
            for(auto& input : layer->getInputNodes()) {
                auto result = input->apply();

                // failed to apply setting
                if(result.isErr()) {
                    geode::log::error("{}", result.unwrapErr());
                    GBAlertLayer::create("Error", fmt::format("<cr>{}</c>", result.unwrapErr()), "OK")->show();

                    settingsApplied = false;
                    break;
                }
            }
        }

        // apply toggles
        m_renderParams.m_includeAudio = m_toggles.at(0)->isToggled();
        m_renderParams.m_renderingLabels = m_toggles.at(1)->isToggled();
        m_renderParams.m_encoderMultithread = m_toggles.at(2)->isToggled();
    }

    if(!settingsApplied) return;

    auto dir = CCDirector::get();
    auto view = CCEGLView::get();

    if(m_status == BotStatus::Rendering) {
        // Rendering settings: change aspect ratio if needed
        auto oldDesignRes = view->getDesignResolutionSize();
        auto newDesignRes = Encoder::getDesignResolution(m_renderParams.m_width, m_renderParams.m_height);

        m_renderParams.m_originalDesignRes = oldDesignRes;
        m_renderParams.m_newDesignRes = newDesignRes;

        m_renderParams.m_originalScreenScaleX = view->m_fScaleX;
        m_renderParams.m_originalScreenScaleY = view->m_fScaleY;

        m_renderParams.m_newScreenScaleX = static_cast<float>(m_renderParams.m_width) / newDesignRes.width;
        m_renderParams.m_newScreenScaleY = static_cast<float>(m_renderParams.m_height) / newDesignRes.height;

        bot->applyRenderParams(m_renderParams);
    }

    // start
    auto result = bot->changeStatus(m_status);
    if(result.isOk()) {
        if(m_status == BotStatus::Rendering) {
            // apply new aspect ratio
            if(m_renderParams.m_originalDesignRes != m_renderParams.m_newDesignRes) {
                bot->applyWinSize();
            }

            // reload PlayLayer
            bot->getPlayLayer()->m_loadingLayer = GJGameLoadingLayer::transitionToLoadingLayer(bot->getPlayLayer()->m_level, false);
            bot->getPlayLayer()->m_loadingLayer->retain();
        }
        // Recording / Replaying settings: just close the menu and begin
        else {
            // close layer
            this->onClose(nullptr);

            // close gatobot menu
            OverlayLayer::close();

            // close PauseLayer and restart
            for(auto child : CCArrayExt<CCNode*>(CCDirector::get()->getRunningScene()->getChildren())) {
                if(auto p = typeinfo_cast<PauseLayer*>(child)) {
                    p->onRestart(nullptr);
                }
            }
        }
    }
    else {
        // error
        GBAlertLayer::create("Error", fmt::format("<cr>{}</c>", result.unwrapErr()), "OK")->show();
    }
}

void SettingsPopup::onClose(CCObject*) {
    this->keyBackClicked();
}