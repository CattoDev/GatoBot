#pragma once

#include <nodes/template/SettingsLayerTemplate.hpp>

struct RenderSettingsPreset {
    int m_width;
    int m_height;
    int m_fps;
    int m_videoBitrate;
    
    std::string m_codec;
};

class VideoSettingsLayer : public SettingsLayerTemplate {
private:
    cocos2d::CCMenu* m_presetMenu;
    std::vector<RenderSettingsPreset> m_presets;

private:
    bool init(RenderParams* renderParams, const cocos2d::CCSize& size);
    void createPresetButton(const char* label, const RenderSettingsPreset& presetData);
    void applyPreset(const RenderSettingsPreset& presetData);

    void onPreset(CCObject*);
    void onPickFile(CCObject*);

public:
    static VideoSettingsLayer* create(RenderParams* renderParams, const cocos2d::CCSize& size);
};