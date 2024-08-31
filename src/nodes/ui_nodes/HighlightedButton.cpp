#include "HighlightedButton.hpp"

using namespace geode::prelude;

bool HighlightedButton::init(const char* text, CCObject* target, cocos2d::SEL_MenuHandler callback, const CCSize& buttonSize) {
    if(!CCMenuItem::initWithTarget(target, callback)) return false;
    
    this->setContentSize(buttonSize);
    m_highlighted = false;

    // create label
    m_label = CCLabelBMFont::create(text, "bigFont.fnt");
    m_label->limitLabelWidth(buttonSize.width - 10.f, .6f, .1f);
    m_label->setPosition(buttonSize / 2);

    m_labelScale = m_label->getScale();
    
    this->addChild(m_label);

    // create draw node
    m_drawNode = CCDrawNode::create();
    m_drawNode->setPosition(buttonSize / 2);

    this->addChild(m_drawNode);

    return true;
}

void HighlightedButton::toggleHighlight(bool toggle) {
    if(m_highlighted == toggle) return;

    m_highlighted = toggle;

    if(!toggle) {
        m_drawNode->clear();
        return;
    }

    // draw line
    auto size = this->getContentSize();

    CCPoint verts[4] = {
        { -size.width / 2, -size.height / 2 },
        { size.width / 2, -size.height / 2 },
        { size.width / 2, -size.height / 2 + 1.f },
        { -size.width / 2, -size.height / 2 + 1.f }
    };
    m_drawNode->drawPolygon(verts, 4, { 1.f, 1.f, 1.f, 1.f }, .1f, { 1.f, 1.f, 1.f, 1.f });
}

void HighlightedButton::selected() {
    if(!m_bEnabled) return;

    CCMenuItem::selected();

    m_label->stopActionByTag(ACTION_TAG);

    auto action = CCEaseBounceOut::create(
        CCScaleTo::create(.3f, m_labelScale * 1.2f)
    );
    m_label->runAction(action);
}

void HighlightedButton::unselected() {
    if(!m_bEnabled) return;

    CCMenuItem::unselected();

    m_label->stopActionByTag(ACTION_TAG);

    auto action = CCEaseBounceOut::create(
        CCScaleTo::create(.4f, m_labelScale)
    );
    m_label->runAction(action);
}

void HighlightedButton::activate() {
    if(!m_bEnabled) return;

    m_label->stopAllActions();
    m_label->setScale(m_labelScale);

    CCMenuItem::activate();
}

HighlightedButton* HighlightedButton::create(const char* text, CCObject* target, cocos2d::SEL_MenuHandler callback, const CCSize& buttonSize) {
    auto pRet = new HighlightedButton();

    if(pRet && pRet->init(text, target, callback, buttonSize)) {
        pRet->autorelease();
        return pRet;
    }
    delete pRet;
    return nullptr;
}