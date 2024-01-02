// temp
#include <Geode/Geode.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/GameStatsManager.hpp>

#include "core/Bot.hpp"

using namespace geode::prelude;

class $modify(CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool keyDown, bool idk) {

		if(keyDown) {
			switch(key) {
				case KEY_R: {
					GatoBot::get()->changeStatus(Recording);
				} break;

				case KEY_T: {
					GatoBot::get()->changeStatus(Replaying);
				} break;

				default: 
					break;
			}
		}

		return CCKeyboardDispatcher::dispatchKeyboardMSG(key, keyDown, idk);
	}
};

class $modify(GameStatsManager) {
	bool isItemUnlocked(UnlockType, int) {
		return true;
	}
};