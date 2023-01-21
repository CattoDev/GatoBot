#include "GatoBot.hpp"

class GatoBotMenu : public gd::FLAlertLayer, public gd::FLAlertLayerProtocol {
public:
    int lastHoveredButtonID = 0;
    CCLabelBMFont* buttonLabel;

public:
    static GatoBotMenu* create() {
        auto pRet = new GatoBotMenu();

        if(pRet && pRet->init()) {
            pRet->autorelease();
            return pRet;
        }
        CC_SAFE_DELETE(pRet);
        return nullptr;
    }

    bool init();
    void update(float) override;

    void buttonHovered(gd::CCMenuItemSpriteExtra*);

    gd::CCMenuItemSpriteExtra* createButton(CCSprite*, SEL_MenuHandler);

    void toggleButtons(bool);

    void onOpen(CCObject*);
    void onRecord(CCObject*);
    void onReplay(CCObject*);
    void onRender(CCObject*);
    void onSaveReplay(CCObject*);
    void onLoadReplay(CCObject*);

    void FLAlert_Clicked(gd::FLAlertLayer*, bool) override;
};