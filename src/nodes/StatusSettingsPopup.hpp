#pragma once

#include "template/PopupTemplate.hpp"
#include <core/Types.hpp>

class StatusSettingsPopup : public PopupTemplate {
public:
    static StatusSettingsPopup* create(BotStatus);

    bool init(BotStatus);
};