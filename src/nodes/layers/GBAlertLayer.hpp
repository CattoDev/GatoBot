#pragma once

#include <functional>
#include <nodes/template/PopupTemplate.hpp>

class GBAlertLayer : public PopupTemplate {
public:
    std::function<void(FLAlertLayer*, geode::prelude::CCObject*)> m_callback;

public:
    static GBAlertLayer* create(const char* title, std::string text, const char* btn1, const char* btn2 = nullptr, std::function<void(FLAlertLayer*, geode::prelude::CCObject*)> callback = [](FLAlertLayer*, geode::prelude::CCObject*){});
    bool init(const char* title, std::string text, const char* btn1, const char* btn2, std::function<void(FLAlertLayer*, geode::prelude::CCObject*)> callback);

    void onButton(CCObject*);
};