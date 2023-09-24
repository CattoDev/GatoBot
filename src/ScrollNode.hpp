#include "includes.hpp"

class GBOptionCell : public gd::TableViewCell {
public:
    CCMenu* m_pButtonMenu;
    gd::CCMenuItemToggler* toggleBtn;

public:
    GBOptionCell(const char*, float, float);

    bool init(gd::CCMenuItemToggler*, const char*);
    void draw() override;
    void updateBGColor(int);
    void addHelpButton(gd::CCMenuItemSpriteExtra*);

    static GBOptionCell* create(gd::CCMenuItemToggler*, const char*);
};

class SettingsPopupScrollLayer : public CCLayer {
public:
    CCSize layerSize;
    CCMenu* cells;

    float itemSize;
    bool useCells;
    bool horizontal;

public:
    static SettingsPopupScrollLayer* create(CCSize);
    bool init(CCSize);
    float getTopScroll();
    float getBottomScroll();

    void setPos(float);
    void setupList();

    void setPosition(CCPoint const&) override;
    void scrollWheel(float, float) override;

    CCRect getAreaRect();
};