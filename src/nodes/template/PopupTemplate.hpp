#pragma once

#include <Geode/Geode.hpp>

class PopupTemplate : public FLAlertLayer {
public:
    virtual void show() override;

    bool init();
    void incrementForcePrio();
};