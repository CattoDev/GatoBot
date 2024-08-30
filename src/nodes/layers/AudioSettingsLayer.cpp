#include "AudioSettingsLayer.hpp"

using namespace geode::prelude;

bool AudioSettingsLayer::init(RenderParams* renderParams, const CCSize& size) {
    if(!CCLayer::init()) return false;

    m_renderParams = renderParams;
    this->setContentSize(size);

    float top = size.height / 2 - 2.5f;
    float left = -size.width / 2 + 7.5f;
    float right = size.width / 2 - 7.5f;

    // audio bitrate
    {
        CCPoint labelPos { left, top - 12.5f };
        this->createLabel("Bitrate (Kb/s)", labelPos, 100.f);

        // bitrate input
        this->createInput("0123456789", CCSize { 100.f, 30.f }, labelPos + CCPoint { 50.f, -25.f }, [renderParams](std::string rawStr) { 
            return applyInt(rawStr.c_str(), &renderParams->m_audioBitrate);
        })->m_inputNode->setMaxCharCount(10);
    }

    // audio volume
    {
        CCPoint labelPos { left, top - 70.f };
        this->createLabel("Volume (%)", labelPos, 100.f);

        // audio input
        this->createInput("0123456789", CCSize { 100.f, 30.f }, labelPos + CCPoint { 50.f, -25.f }, [renderParams](std::string rawStr) { 
            int val;
            auto res = applyInt(rawStr.c_str(), &val);

            // invalid value
            if(res.isErr()) return res;

            if(val < 0) val = 0;
            renderParams->m_audioVolume = static_cast<float>(val) / 100.f;

            return res;
        })->m_inputNode->setMaxCharCount(4);
    }

    return true;
}

void AudioSettingsLayer::applyRenderParams() {
    // bitrate
    m_inputNodes.at(0)->setString(std::to_string(m_renderParams->m_audioBitrate));

    // volume
    int volume = static_cast<int>(m_renderParams->m_audioVolume * 100.f);
    m_inputNodes.at(1)->setString(std::to_string(volume));
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