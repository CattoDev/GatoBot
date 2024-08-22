#pragma once

#include <nodes/template/SettingsLayerTemplate.hpp>

class AudioSettingsLayer : public SettingsLayerTemplate {
private:
    bool init(RenderParams* renderParams, const cocos2d::CCSize& size);

public:
    void applyRenderParams();

    static AudioSettingsLayer* create(RenderParams* renderParams, const cocos2d::CCSize& size);
};