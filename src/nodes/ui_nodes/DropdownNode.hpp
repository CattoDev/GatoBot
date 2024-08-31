#pragma once

#include <Geode/Geode.hpp>

class DropdownNode : public cocos2d::CCMenu {
private:
    std::vector<std::string> m_options;
    float m_maxExtendedHeight;

private:
    bool init(const std::vector<std::string>& options, cocos2d::CCSize size, float maxExtendedHeight);

public:
    static DropdownNode* create(const std::vector<std::string>& options, cocos2d::CCSize size, float maxExtendedHeight);
};