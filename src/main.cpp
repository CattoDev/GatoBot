// temp
#include <Geode/Geode.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/GameStatsManager.hpp>

#include "core/Bot.hpp"

using namespace geode::prelude;

#ifdef GB_DEBUG
/*class $modify(CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool keyDown, bool idk) {
		if(keyDown) {
			switch(key) {
				case KEY_R: {
					GatoBot::get()->changeStatus(Recording);
				} break;

				case KEY_T: {
					GatoBot::get()->changeStatus(Replaying);
				} break;

				case KEY_I: {
					GatoBot::get()->changeStatus(Idle);
				} break;

				case KEY_O: {
					GatoBot::get()->m_loadedMacro.saveFile("C:/Programming/gdmods/GatoBot/macro.gatobot");
				} break;

				case KEY_P: {
					GatoBot::get()->m_loadedMacro.loadFile("C:/Programming/gdmods/GatoBot/macro.gatobot");
				} break;

				case KEY_F: {
					GatoBot::get()->m_loadedMacro.recordingFinished();
				} break;

				default: 
					break;
			}

			// speed settings
			if(key >= KEY_One && key <= KEY_Nine) {
				int factor = key + 1 - KEY_One;

				const float timeScale = 1.f / static_cast<float>(factor);
				//CCScheduler::get()->setTimeScale(timeScale);
				GatoBot::get()->setMainSpeed(timeScale);

				GB_LOG("Changed speed to {}", timeScale);
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

$on_mod(Loaded) {
	// force practice mode button
	(void)Mod::get()->patch(reinterpret_cast<void*>(geode::base::get() + 0x2b3ae1), {0xEB});
}*/
#endif