#include "GBAlertLayer.hpp"

using namespace geode::prelude;

GBAlertLayer* GBAlertLayer::create(const char* title, std::string text, const char* btn1, const char* btn2, std::function<void(FLAlertLayer*, geode::prelude::CCObject*)> callback) {
    auto pRet = new GBAlertLayer();

    if(pRet && pRet->init(title, text, btn1, btn2, callback)) {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}

bool GBAlertLayer::init(const char* title, std::string text, const char* btn1text, const char* btn2text, std::function<void(FLAlertLayer*, geode::prelude::CCObject*)> callback) {
    if(!PopupTemplate::init()) 
        return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    const CCSize layerSize = CCSize { 360, 160 };

    m_callback = callback;

    // bg
    this->setBackground(layerSize);

    auto darkBG = extension::CCScale9Sprite::create("GB_squareBG.png"_spr, { 0, 0, 20.f, 20.f });
    darkBG->setContentSize(layerSize - CCSize { 20.f, 80.f });
    darkBG->setOpacity(100);
    darkBG->setPosition(winSize / 2 + CCPoint { 0.f, 5.f });

    m_mainLayer->addChild(darkBG);

    // title label
    auto titleLabel = CCLabelBMFont::create(title, "bigFont.fnt");
    titleLabel->limitLabelWidth(120, .8f, .1f);
    titleLabel->setPosition(winSize / 2 + CCPoint { 0, layerSize.height / 2 - 15.f });

    m_mainLayer->addChild(titleLabel, 1);

    // text area
    auto textArea = TextArea::create(text, "chatFont.fnt", 1.f, layerSize.width - 60.f, CCPoint { .5f, .5f }, 20.f, 0);
    textArea->setPosition(darkBG->getPosition());
    
    m_mainLayer->addChild(textArea, 1);

    // buttons 
    auto btn1 = CCMenuItemSpriteExtra::create(
        ButtonSprite::create(btn1text, "goldFont.fnt", "GJ_button_06.png", 1),
        this,
        menu_selector(GBAlertLayer::onButton)
    );
    btn1->setTag(1);

    m_buttonMenu->addChild(btn1);

    CCMenuItemSpriteExtra* btn2 = nullptr;
    if(btn2text != nullptr) {
        btn2 = CCMenuItemSpriteExtra::create(
            ButtonSprite::create(btn2text, "goldFont.fnt", "GJ_button_06.png", 1),
            this,
            menu_selector(GBAlertLayer::onButton)
        );
        btn2->setTag(2);

        m_buttonMenu->addChild(btn2);
    }

    m_buttonMenu->alignItemsHorizontallyWithPadding(10);
    btn1->setPositionY(-layerSize.height / 2 + 20.f);
    if(btn2) btn2->setPositionY(-layerSize.height / 2 + 20.f);

    return true;
}

void GBAlertLayer::onButton(CCObject* pSender) {
    m_callback(this, pSender);

    // close layer
    this->keyBackClicked();
}