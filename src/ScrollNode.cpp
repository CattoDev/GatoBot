#include "ScrollNode.hpp"

#define CELLHEIGHT 30

GBOptionCell::GBOptionCell(const char* key, float width, float height) : gd::TableViewCell(key, width, height) {}

void GBOptionCell::draw() {
    // StatsCell::draw
    reinterpret_cast<void(__thiscall*)(gd::TableViewCell*)>(gd::base + 0x59d40)(this);
}

void GBOptionCell::updateBGColor(int index) {
    if (index & 1) m_pBGLayer->setColor(ccc3(0xc2, 0x72, 0x3e));
    else m_pBGLayer->setColor(ccc3(0xa1, 0x58, 0x2c));
    m_pBGLayer->setOpacity(255);
}

bool GBOptionCell::init(gd::CCMenuItemToggler* toggle, const char* lText) {
    m_pBGLayer->setOpacity(255);
    m_pLayer->setVisible(true);

    // toggle
    toggleBtn = toggle;
    m_pButtonMenu = CCMenu::create();
    m_pButtonMenu->setPosition(CCPointZero);
    m_pButtonMenu->addChild(toggle);

    m_pLayer->addChild(m_pButtonMenu, 2);

    toggle->setPosition(CCPoint(15, CELLHEIGHT / 2));

    // label
    auto label = CCLabelBMFont::create(lText, "bigFont.fnt");
    label->setScale(.3f);
    label->setAlignment(CCTextAlignment::kCCTextAlignmentLeft);
    label->setAnchorPoint(CCPoint(0, .5));
    label->setPosition(CCPoint(40, CELLHEIGHT / 2));

    m_pLayer->addChild(label);

    return true;
}

GBOptionCell* GBOptionCell::create(gd::CCMenuItemToggler* toggle, const char* lText) {
    auto pRet = new GBOptionCell(lText, 140, CELLHEIGHT);

    if(pRet && pRet->init(toggle, lText)) {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}

void GBOptionCell::addHelpButton(gd::CCMenuItemSpriteExtra* helpBtn) {
    helpBtn->setPosition(toggleBtn->getPosition() + CCPoint(17.5, 10));

    m_pButtonMenu->addChild(helpBtn, 1);
}

// yeah
SettingsPopupScrollLayer* SettingsPopupScrollLayer::create(CCSize size) {
    auto pRet = new SettingsPopupScrollLayer();

    if(pRet && pRet->init(size)) {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}

bool SettingsPopupScrollLayer::init(CCSize size) {
    if(!CCLayer::init()) return false;

    // default vars
    useCells = true;
    horizontal = false;
    itemSize = CELLHEIGHT;
    layerSize = size;

    cells = CCMenu::create();
    cells->setPosition(CCPoint(0, -layerSize.height / 2));

    // clipping
    // https://github.com/matcool/small-gd-mods/blob/main/src/song-search.cpp#L63
    auto texture = new CCTexture2D();
    texture->autorelease();
    const unsigned char data[3] = {255, 255, 255};
    texture->initWithData(data, kCCTexture2DPixelFormat_RGB888, 1, 1, {1, 1});
    texture->setAliasTexParameters();

    auto stencil = CCSprite::createWithTexture(texture);
    stencil->setScaleX(layerSize.width / stencil->getContentSize().width);
    stencil->setScaleY(layerSize.height / stencil->getContentSize().height);
    stencil->setAnchorPoint({0, .5});

    auto clip = CCClippingNode::create(stencil);
    clip->addChild(cells);
    clip->setAlphaThreshold(0.1f);
    
    addChild(clip);

    return true;
}

float SettingsPopupScrollLayer::getTopScroll() {
    return horizontal ?
        layerSize.width - cells->getChildrenCount() * itemSize
        :
        (float)cells->getChildrenCount() * -itemSize + -getBottomScroll()
    ;
}

float SettingsPopupScrollLayer::getBottomScroll() {
    return horizontal ? 0 : -layerSize.height / 2;
}

void SettingsPopupScrollLayer::scrollWheel(float posDiff, float) {
    // change pos
    float newPos;
    
    if(horizontal)
        newPos = cells->getPositionX() - posDiff;

    else
        newPos = cells->getPositionY() + posDiff;  
    
    setPos(newPos);
}

void SettingsPopupScrollLayer::setPos(float newPos) {
    // lock scrolling
    if(newPos < getTopScroll())
        newPos = getTopScroll();

    if(newPos > getBottomScroll())
        newPos = getBottomScroll();

    // set position
    if(horizontal)
        cells->setPositionX(newPos);

    else
        cells->setPositionY(newPos);
}

void SettingsPopupScrollLayer::setPosition(CCPoint const& newPos) {
    // how did I fuck this up LMFAO
    CCNode::setPosition(newPos + CCPoint(-layerSize.width / 2, 0));
}

void SettingsPopupScrollLayer::setupList() {
    // order cells
    size_t itemAmt = cells->getChildrenCount();
    for(size_t i = 0; i < itemAmt; i++) {
        auto cell = (GBOptionCell*)cells->getChildren()->objectAtIndex(i);
        
        if(useCells)
            cell->updateBGColor(i);
        
        else {
            cell->setPositionX(layerSize.width / 2);
        }

        if(horizontal)
            cell->setPosition({i * itemSize + itemSize / 2, layerSize.height / 2});

        else
            cell->setPositionY((itemAmt - i - 1) * itemSize);
    }

    // scroll to top
    if(horizontal)
        cells->setPositionX(((layerSize.width - (itemAmt * itemSize)) / 2));

    else
        cells->setPositionY(getTopScroll());
}

CCRect SettingsPopupScrollLayer::getAreaRect() {
    CCRect rect;

    auto worldPos = m_pParent->convertToWorldSpace(getPosition());

    worldPos.y -= layerSize.height / 2;

    rect.origin = worldPos;
    rect.size = layerSize;

    return rect;
}