#include "SettingsLayerTemplate.hpp"

using namespace geode::prelude;

// I hate geode
class _TextInput : public TextInput {
public:
    extension::CCScale9Sprite*& getBGSprite() {
        return m_bgSprite;
    }
};

bool SettingsInput::init(const std::string& filter, const cocos2d::CCSize& size, const ConvertFunc& func) {
    if(!CCMenu::init()) return false;

    m_convertFunc = func;

    this->setContentSize(size);

    m_inputNode = TextInput::create(size.width + 5.f, "", "bigFont.fnt");
    m_inputNode->setFilter(filter);

    // replace bg
    auto _i = reinterpret_cast<_TextInput*>(m_inputNode);

    _i->getBGSprite()->removeFromParentAndCleanup(true);
    auto bgSpr = CCScale9Sprite::create("GB_squareBG.png"_spr, { 0, 0, 20.f, 20.f });
    bgSpr->setScale(.5f);
    bgSpr->setOpacity(90);
    bgSpr->setContentSize(size * 2);
    bgSpr->setZOrder(-1);
    m_inputNode->addChildAtPosition(bgSpr, Anchor::Center);

    _i->getBGSprite() = bgSpr;

    this->addChild(m_inputNode);
    
    return true;
}

void SettingsInput::setString(const std::string& string) {
    m_inputNode->setString(string);
}

geode::Result<> SettingsInput::apply() {
    return m_convertFunc(m_inputNode->getString());
}

SettingsInput* SettingsInput::create(const std::string& filter, const cocos2d::CCSize& size, const ConvertFunc& func) {
    auto pRet = new SettingsInput();

    if(pRet && pRet->init(filter, size, func)) {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}

std::vector<SettingsInput*>& SettingsLayerTemplate::getInputNodes() {
    return m_inputNodes;
}

void SettingsLayerTemplate::createDarkBG(const cocos2d::CCSize& size, const cocos2d::CCPoint& position) {
    auto bg = CCScale9Sprite::create("GB_squareBG.png"_spr, { 0.f, 0.f, 20.f, 20.f });
    bg->setContentSize(size);
    bg->setPosition(position);
    bg->setOpacity(75);

    this->addChild(bg, -1);
}

void SettingsLayerTemplate::createLabel(const char* text, const cocos2d::CCPoint& position, float maxWidth, cocos2d::CCTextAlignment alignment) {
    auto label = CCLabelBMFont::create(text, "bigFont.fnt");

    label->setPosition(position);
    label->limitLabelWidth(maxWidth, .6f, .1f);

    if(alignment == kCCTextAlignmentLeft) {
        label->setAnchorPoint(CCPoint { 0.f, .5f });
    }

    this->addChild(label);
}

SettingsInput* SettingsLayerTemplate::createInput(const std::string& filter, const CCSize& size, const CCPoint& position, const SettingsInput::ConvertFunc& convertFunc) {
    auto input = SettingsInput::create(filter, size, convertFunc);
    input->setPosition(position);

    this->addChild(input);

    m_inputNodes.push_back(input);

    return input;
}

geode::Result<> SettingsLayerTemplate::applyInt(const char* rawStr, int* value) {
    Result<> res = Ok();

    char *endPtr;
    *value = strtol(rawStr, &endPtr, 10);

    // failed to convert
    if(*endPtr != '\0') res = Err("Invalid int in input node!");

    return res;
}