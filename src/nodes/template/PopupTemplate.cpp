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