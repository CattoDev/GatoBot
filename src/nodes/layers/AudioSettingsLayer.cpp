#include "AudioSettingsLayer.hpp"

using namespace geode::prelude;

bool AudioSettingsLayer::init(RenderParams* renderParams, const CCSize& size) {
    if(!CCLayer::init()) return false;

    m_renderParams = renderParams;
    this->setContentSize(size);

    float top = size.height / 2 - 2.5f;
    float left = -size.width / 2 + 7.5f;
    float right = size.width / 2 - 7.5f;

    {
        CCPoint labelPos { left, top - 12.5f };
        this->createLabel("Bitrate (Kb/s)", labelPos, 100.f);

        // bitrate input
        this->createInput("0123456789", CCSize { 100.f, 30.f }, labelPos + CCPoint { 50.f, -25.f }, [renderParams](std::string rawStr) { 
            return applyInt(rawStr.c_str(), &renderParams->m_audioBitrate);
        })->m_inputNode->setMaxCharCount(10);
    }

    return true;
}

void AudioSettingsLayer::applyRenderParams() {
    // bitrate
    m_inputNodes.at(0)->setString(std::to_string(m_renderParams->m_audioBitrate));
}

AudioSettingsLayer* AudioSettingsLayer::create(RenderParams* renderParams, const cocos2d::CCSize& size) {
    auto pRet = new AudioSettingsLayer();

    if(pRet && pRet->init(renderParams, size)) {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}