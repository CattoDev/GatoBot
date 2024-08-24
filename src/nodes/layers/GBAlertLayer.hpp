#pragma once

#include <functional>
#include <nodes/template/PopupTemplate.hpp>

class GBAlertLayer : public PopupTemplate {
public:
    std::function<void(FLAlertLayer*, cocos2d::CCObject*)> m_callback;

public:
    static GBAlertLayer* create(const char* title, std::string text, const char* btn1, const char* btn2 = nullptr, std::function<void(FLAlertLayer*, cocos2d::CCObject*)> callback = [](FLAlertLayer*, cocos2d::CCObject*){});
    bool init(const char* title, std::string text, const char* btn1, const char* btn2, std::function<void(FLAlertLayer*, cocos2d::CCObject*)> callback);

    void onButton(CCObject*);
};