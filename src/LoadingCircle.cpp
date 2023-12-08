#include "LoadingCircle.hpp"

bool GBLoadingCircle::init() {
    if(!initWithColor(ccc4(0, 0, 0, 105))) return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    circle = gd::LoadingCircle::create();
    circle->setParent(this);
    circle->show();

    GatoBot::sharedState()->loadingCircle = this;

    // caption
    captionLabel = CCLabelBMFont::create("", "bigFont.fnt");
    captionLabel->setPosition(winSize / 2 + CCPoint(0, -60));
    captionLabel->setScale(.8f);

    addChild(captionLabel);

    // touch
    registerWithTouchDispatcher();
    CCDirector::sharedDirector()->getTouchDispatcher()->incrementForcePrio(2);

    setTouchEnabled(true);
    retain();
    
    return true;
}

void GBLoadingCircle::remove() {
    auto self = GatoBot::sharedState()->loadingCircle;

    if(self != nullptr) {
        if(self->circle != nullptr)
            self->circle->fadeAndRemove();
        
        self->keyBackClicked();

        GatoBot::sharedState()->loadingCircle = nullptr;
    }
}

void GBLoadingCircle::setCaption(const char* caption) {
    if(auto circle = GatoBot::sharedState()->loadingCircle) {
        circle->captionLabel->setString(caption);
    }
}