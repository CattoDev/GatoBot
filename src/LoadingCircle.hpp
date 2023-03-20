#include "GatoBot.hpp"

class GBLoadingCircle : public gd::FLAlertLayer {
public:
    gd::LoadingCircle* circle;

public:
    static GBLoadingCircle* create() {
        auto pRet = new GBLoadingCircle();
        
        if(pRet && pRet->init()) {
            pRet->autorelease();
            return pRet;
        }
        CC_SAFE_DELETE(pRet);
        return nullptr;
    }

    bool init();

    static void remove();
};