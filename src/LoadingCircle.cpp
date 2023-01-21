#include "LoadingCircle.hpp"

bool LoadingCircle::init() {
    if(!initWithColor(ccc4(0, 0, 0, 105))) return false;

    circle = gd::LoadingCircle::create();
    circle->setParent(this);
    circle->show();

    GatoBot::sharedState()->loadingCircle = this;

    registerWithTouchDispatcher();
    CCDirector::sharedDirector()->getTouchDispatcher()->incrementForcePrio(2);

    setTouchEnabled(true);
    
    return true;
}

void LoadingCircle::remove() {
    auto self = GatoBot::sharedState()->loadingCircle;

    if(self != nullptr) {
        self->circle->fadeAndRemove();
        self->keyBackClicked();
    }
}