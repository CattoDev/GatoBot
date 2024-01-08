#include "PopupTemplate.hpp"

using namespace geode::prelude;
  
void PopupTemplate::show() {
    // add to scene
    CCDirector::sharedDirector()->getRunningScene()->addChild(this, 1000);

    // animate
    auto action = CCEaseExponentialOut::create(
        CCScaleTo::create(.5, 1.f)
    );

    m_mainLayer->setScale(0);
    m_mainLayer->runAction(action);
}

bool PopupTemplate::init() {
    if(!this->initWithColor(ccColor4B {0, 0, 0, 105}))
        return false;

    // incrementForcePrio() is inlined on windows
    this->incrementForcePrio();
    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);

    // create main layer
    m_mainLayer = CCLayer::create();
    this->addChild(m_mainLayer);

    // button menu
    m_buttonMenu = CCMenu::create();
    m_mainLayer->addChild(m_buttonMenu, 10);
    
    return true;
}

void PopupTemplate::incrementForcePrio() {
    // incrementForcePrio() is inlined on windows
    if(m_forcePrioRegistered) return;

    auto dis = CCDirector::sharedDirector()->getTouchDispatcher();
    dis->registerForcePrio(this, 2);

    m_forcePrioRegistered = true;
}

void PopupTemplate::setBackground(CCSize const& size) {
    auto winSize = CCDirector::sharedDirector()->getWinSize();

    // border
    if(!m_bgBorder) {
        m_bgBorder = extension::CCScale9Sprite::create("GJ_square07.png", { 0.f, 0.f, 80.f, 80.f });
        m_bgBorder->setPosition(winSize / 2);

        m_mainLayer->addChild(m_bgBorder);
    }
    m_bgBorder->setContentSize(size);

    // background
    if(!m_bgGradient) {
        m_bgGradient = CCLayerGradient::create({ 125, 0, 0, 255 }, { 175, 175, 0, 255 });
        m_bgGradient->setPosition(winSize / 2 - size / 2); // CCLayerGradient anchor point is goofy
    }
    m_bgGradient->setContentSize(size);

    // round corners
    if(!m_bgClipping) {
        auto stencil = extension::CCScale9Sprite::create("square02_001.png", { 0, 0, 80, 80 });
        stencil->setPosition(winSize / 2);

        m_bgClipping = CCClippingNode::create(stencil);
        m_bgClipping->addChild(m_bgGradient);
        m_bgClipping->setAlphaThreshold(0.1f);

        m_mainLayer->addChild(m_bgClipping, -1);
    }
    m_bgClipping->setContentSize(size);
    m_bgClipping->getStencil()->setContentSize(size);
}