#include "DropdownNode.hpp"

using namespace geode::prelude;

bool DropdownNode::init(const std::vector<std::string>& options, cocos2d::CCSize size, float maxExtendedHeight) {
    if(!CCMenu::init()) return false;

    m_options = options;
    m_maxExtendedHeight = maxExtendedHeight;

    // main dark bg
    auto darkBG = CCScale9Sprite::create("GB_squareBG.png"_spr, CCRect { 0.f, 0.f, 20.f, 20.f });
    darkBG->setContentSize(size);

    this->addChild(darkBG, 1);
    
    return true;
}

DropdownNode* DropdownNode::create(const std::vector<std::string>& options, cocos2d::CCSize size, float maxExtendedHeight) {
    auto pRet = new DropdownNode();

    if(pRet && pRet->init(options, size, maxExtendedHeight)) {
        pRet->autorelease();
        return pRet;
    }
    delete pRet;
    return nullptr;
}