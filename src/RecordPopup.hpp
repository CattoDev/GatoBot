#include "GatoBot.hpp"

class GatoBotMenu;

class RecordPopup : public gd::FLAlertLayer {
public:
    GatoBotMenu* parentMenu;
    gd::CCTextInputNode* fpsInput;
    gd::CCTextInputNode* speedInput;
    bool isReplay;

public:
    static RecordPopup* create(GatoBotMenu* parent, bool isReplay) {
        auto pRet = new RecordPopup();
        pRet->parentMenu = parent;

        if(pRet && pRet->init(isReplay)) {
            pRet->autorelease();
            return pRet;
        }
        CC_SAFE_DELETE(pRet);
        return nullptr;
    }

    bool init(bool);
    void keyBackClicked() override;
    static void open(GatoBotMenu*, bool a = false);

    void onStart(CCObject*);
    void onCancel(CCObject*);
};