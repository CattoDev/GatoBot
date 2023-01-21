#include "GatoBot.hpp"

class LoadingCircle : public gd::FLAlertLayer {
public:
    gd::LoadingCircle* circle;

public:
    static LoadingCircle* create() {
        auto pRet = new LoadingCircle();
        
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