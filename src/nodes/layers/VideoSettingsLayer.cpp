#include "VideoSettingsLayer.hpp"

using namespace geode::prelude;

bool VideoSettingsLayer::init(RenderParams* renderParams, const CCSize& size) {
    if(!CCLayer::init()) return false;

    m_renderParams = renderParams;
    this->setContentSize(size);

    float top = size.height / 2 - 2.5f;
    float left = -size.width / 2 + 7.5f;
    float right = size.width / 2 - 7.5f;

    // resolution inputs
    {
        this->createLabel("Resolution", CCPoint { left, top - 12.5f }, 100.f);

        CCPoint inputsPosLeft = { left + 20.f, top - 32.5f };

        // width
        this->createInput("0123456789", CCSize { 40.f, 20.f }, inputsPosLeft, [renderParams](std::string rawStr) { 
            return applyInt(rawStr.c_str(), &renderParams->m_width);
        })->m_inputNode->setMaxCharCount(4);

        // height
        this->createInput("0123456789", CCSize { 40.f, 20.f }, inputsPosLeft + CCPoint { 55.f, 0.f }, [renderParams](std::string rawStr) { 
            return applyInt(rawStr.c_str(), &renderParams->m_height);
        })->m_inputNode->setMaxCharCount(4);

        // FPS
        this->createInput("0123456789", CCSize { 30.f, 20.f }, inputsPosLeft + CCPoint { 105.f, 0.f }, [renderParams](std::string rawStr) { 
            return applyInt(rawStr.c_str(), &renderParams->m_fps);
        })->m_inputNode->setMaxCharCount(3);

        this->createLabel("X", inputsPosLeft + CCPoint { 27.5f, .5f }, 10.f, kCCTextAlignmentCenter);
        this->createLabel("@", inputsPosLeft + CCPoint { 82.5f, .5f }, 10.f, kCCTextAlignmentCenter);
    }

    // codec options
    {
        auto labelPos = CCPoint { left, top - 60.f };

        this->createLabel("Codec", labelPos, 100.f);

        CCPoint inputsPosLeft = labelPos + CCPoint { 50.f, -25.f };

        // codec input
        auto input = this->createInput(geode::getCommonFilterAllowedChars(CommonFilter::Name), CCSize { 100.f, 30.f }, inputsPosLeft, [renderParams](std::string rawStr) { 
            renderParams->m_codec = rawStr;
            
            return Ok();
        });
        input->m_inputNode->setMaxCharCount(20);
        input->m_inputNode->setEnabled(false);
    }

    // presets
    {
        CCPoint labelPos = { right - 60.f, top - 12.5f };

        this->createLabel("Presets", labelPos, 100.f, kCCTextAlignmentCenter);

        // preset buttons
        m_presetMenu = CCMenu::create();
        m_presetMenu->setContentSize(CCSize { 120.f, 100.f });
        m_presetMenu->setLayout(
            RowLayout::create()
                ->setGrowCrossAxis(true)
        );
        m_presetMenu->setPosition(labelPos + CCPoint { 0.f, -40.f });

        this->addChild(m_presetMenu);

        this->createPresetButton("360p", { 640, 360, 60, 2000, "libx264" });
        this->createPresetButton("480p", { 854, 480, 60, 4000, "libx264" });
        this->createPresetButton("720p", { 1280, 720, 60, 6000, "libx264" });
        this->createPresetButton("1080p", { 1920, 1080, 60, 10000, "libx264" });
        this->createPresetButton("4K", { 3840, 2160, 60, 50000, "libx264" });
        //this->createPresetButton("8K", { 7680, 4320, 60, 100000, "libx265" });
    }

    // video bitrate
    {
        CCPoint labelPos = { left, top - 115.f };

        this->createLabel("Bitrate (Kb/s)", labelPos, 100.f);

        CCPoint inputsPosLeft = labelPos + CCPoint { 50.f, -25.f };

        // bitrate input
        this->createInput("0123456789", CCSize { 100.f, 30.f }, inputsPosLeft, [renderParams](std::string rawStr) { 
            return applyInt(rawStr.c_str(), &renderParams->m_videoBitrate);
        })->m_inputNode->setMaxCharCount(10);
    }

    // output video path
    {
        CCPoint labelPos = { left, top - 170.f };

        this->createLabel("Output path", labelPos, 100.f);

        // filepath input
        auto pathInput = this->createInput(geode::getCommonFilterAllowedChars(CommonFilter::Any), CCSize { size.width - 50.f, 30.f }, CCPoint { -20.f, labelPos.y - 25.f }, [renderParams](std::string rawStr) {
            std::filesystem::path filePath = rawStr;
            Result<> res = Ok();

            if(filePath.empty()) res = Err("Output video file path is empty!");

            // verify extension
            if(!filePath.has_extension() || filePath.extension() != ".mp4") {
                filePath.replace_extension(".mp4");
            }
            
            renderParams->m_outputPath = filePath.string();
            
            return res;
        });
        pathInput->m_inputNode->setPlaceholder("output video path");
        
        // stupid ass placeholder label fix
        // (the label doesn't appear initially)
        pathInput->m_inputNode->getInputNode()->onClickTrackNode(true);
        pathInput->m_inputNode->getInputNode()->onClickTrackNode(false);

        // file pick button
        auto btnMenu = CCMenu::create();
        btnMenu->setPosition(CCPointZero);
        this->addChild(btnMenu);

        auto filePickBtnSpr = CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png");
        auto filePickBtn = CCMenuItemSpriteExtra::create(filePickBtnSpr, this, menu_selector(VideoSettingsLayer::onPickFile));

        filePickBtn->setPosition(CCPoint { size.width / 2 - 22.5f, labelPos.y - 25.f });

        btnMenu->addChild(filePickBtn);
    }
    
    return true;
}

void VideoSettingsLayer::createPresetButton(const char* label, const RenderSettingsPreset& presetData) {
    auto buttonSpr = ButtonSprite::create(label, 60.f, true, "bigFont.fnt", "GJ_button_04.png", 40.f, .8f);
    buttonSpr->setScale(.5f);
    auto button = CCMenuItemSpriteExtra::create(buttonSpr, this, menu_selector(VideoSettingsLayer::onPreset));
    button->setTag(m_presets.size());

    m_presetMenu->addChild(button);
    m_presets.push_back(std::move(presetData));

    m_presetMenu->updateLayout();
}

void VideoSettingsLayer::applyPreset(const RenderSettingsPreset& presetData) {
    // resolution
    m_inputNodes.at(0)->setString(std::to_string(presetData.m_width));
    m_inputNodes.at(1)->setString(std::to_string(presetData.m_height));
    m_inputNodes.at(2)->setString(std::to_string(presetData.m_fps));

    // codec
    m_inputNodes.at(3)->setString(presetData.m_codec);

    // bitrate
    m_inputNodes.at(4)->setString(std::to_string(presetData.m_videoBitrate));
}

void VideoSettingsLayer::onPreset(CCObject* sender) { 
    auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    const int tag = btn->getTag();

    this->applyPreset(m_presets.at(tag));
}

void VideoSettingsLayer::onPickFile(CCObject*) {
    geode::utils::file::FilePickOptions options;
    options.filters.push_back({ "mp4 video", { "*.mp4" } });

    geode::utils::file::pick(
        geode::utils::file::PickMode::SaveFile,
        options  
    ).listen(
        [this](Result<std::filesystem::path>* result) {
            // failed to choose path
            if(!result->isOk()) {
                return;
            }

            std::filesystem::path filePath = result->unwrap();

            // fix extension
            if(!filePath.has_extension() || filePath.extension() != ".mp4") {
                filePath.replace_extension(".mp4");
            }

            // apply path
            m_inputNodes.at(5)->setString(filePath.string());
        }
    );
}

std::vector<RenderSettingsPreset>& VideoSettingsLayer::getPresets() {
    return m_presets;
}

void VideoSettingsLayer::applyRenderParams() {
    // resolution
    m_inputNodes.at(0)->setString(std::to_string(m_renderParams->m_width));
    m_inputNodes.at(1)->setString(std::to_string(m_renderParams->m_height));
    m_inputNodes.at(2)->setString(std::to_string(m_renderParams->m_fps));

    // codec
    m_inputNodes.at(3)->setString(m_renderParams->m_codec);

    // bitrate
    m_inputNodes.at(4)->setString(std::to_string(m_renderParams->m_videoBitrate));
}

VideoSettingsLayer* VideoSettingsLayer::create(RenderParams* renderParams, const CCSize& size) {
    auto pRet = new VideoSettingsLayer();

    if(pRet && pRet->init(renderParams, size)) {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}