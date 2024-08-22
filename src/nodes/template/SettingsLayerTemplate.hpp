#pragma once

#include <Geode/Geode.hpp>
#include <core/Types.hpp>

#include <functional>
#include <any>

class SettingsInput : public cocos2d::CCMenu {
public:
    enum ValueType { String, Int, Float };

    using ConvertFunc = std::function<geode::Result<>(std::string)>;

public:
    geode::TextInput* m_inputNode;
    ConvertFunc m_convertFunc;

private:
    bool init(const std::string& filter, const cocos2d::CCSize& size, const ConvertFunc& func);

public:
    void setString(const std::string& string);
    geode::Result<> apply();
    static SettingsInput* create(const std::string& filter, const cocos2d::CCSize& size, const ConvertFunc& func);
};

class SettingsLayerTemplate : public cocos2d::CCLayer {
protected:
    RenderParams* m_renderParams;
    std::vector<SettingsInput*> m_inputNodes;

public:
    std::vector<SettingsInput*>& getInputNodes();

    void createDarkBG(const cocos2d::CCSize& size, const cocos2d::CCPoint& position);
    void createLabel(const char* text, const cocos2d::CCPoint& position, float maxWidth, cocos2d::CCTextAlignment alignment = cocos2d::CCTextAlignment::kCCTextAlignmentLeft);
    SettingsInput* createInput(const std::string& filter, const cocos2d::CCSize& size, const cocos2d::CCPoint& position, const SettingsInput::ConvertFunc& convertFunc);

    static geode::Result<> applyInt(const char* rawStr, int* value);
};