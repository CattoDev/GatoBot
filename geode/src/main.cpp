#include <Geode/Geode.hpp>
#include <filesystem>
#include <Windows.h>

#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>

using namespace geode::prelude;

#define SELF reinterpret_cast<void*>(this)
#define ASSIGNFUNC(func, symbol) func = reinterpret_cast<decltype(func)>(GetProcAddress(modLib, symbol))

// GatoBot funcs
void (*GatoBot_setupBot)();
void (*GatoBot_setupGeode)();

// GatoBot hook exports
void (*GEODE_PAUSELAYER_CUSTOMSETUP)(void*);
void (*GEODE_PLAYLAYER_UPDATE)(void*, float);
bool (*GEODE_PLAYLAYER_INIT)(void*, void*);
void (*GEODE_GJBASEGAMELAYER_PUSHBUTTON)(void*, int, bool);
void (*GEODE_GJBASEGAMELAYER_RELEASEBUTTON)(void*, int, bool);
void (*GEODE_PLAYLAYER_RESETLEVEL)(void*);
void (*GEODE_CCSCHEDULER_UPDATE)(void*, float);
void (*GEODE_PLAYLAYER_REMOVELASTCHECKPOINT)(void*);
void (*GEODE_PLAYLAYER_DESTROYPLAYER)(void*, void*, void*);
void (*GEODE_PLAYLAYER_ONQUIT)(void*);
void (*GEODE_PLAYLAYER_LEVELCOMPLETE)(void*);

class $modify(PauseLayer) {
	void customSetup() {
		GEODE_PAUSELAYER_CUSTOMSETUP(SELF);
	}
};

class $modify(PlayLayer) {
	void update(float dt) {
		GEODE_PLAYLAYER_UPDATE(SELF, dt);
	}

	bool init(GJGameLevel* level) {
		return GEODE_PLAYLAYER_INIT(SELF, reinterpret_cast<void*>(level));
	}

	void resetLevel() {
		GEODE_PLAYLAYER_RESETLEVEL(SELF);
	}

	void removeLastCheckpoint() {
		GEODE_PLAYLAYER_REMOVELASTCHECKPOINT(SELF);
	}

	void destroyPlayer(PlayerObject* player, GameObject* obj) {
		GEODE_PLAYLAYER_DESTROYPLAYER(SELF, reinterpret_cast<void*>(player), reinterpret_cast<void*>(obj));
	}

	void onQuit() {
		GEODE_PLAYLAYER_ONQUIT(SELF);
	}

	void levelComplete() {
		GEODE_PLAYLAYER_LEVELCOMPLETE(SELF);
	}
};

class $modify(GJBaseGameLayer) {
	void pushButton(int i, bool p2) {
		GEODE_GJBASEGAMELAYER_PUSHBUTTON(SELF, i, p2);
	}

	void releaseButton(int i, bool p2) {
		GEODE_GJBASEGAMELAYER_RELEASEBUTTON(SELF, i, p2);
	}
};

class $modify(CCScheduler) {
	void update(float dt) {
		GEODE_CCSCHEDULER_UPDATE(SELF, dt);
	}
};

$on_mod(Enabled) {
	//Loader::get()->openPlatformConsole();

    const auto tempDir = Mod::get()->getTempDir();
	auto dllPath = tempDir / "GatoBot.dll";

	if(ghc::filesystem::exists(dllPath)) {
		// link functions
		auto modLib = GetModuleHandle("GatoBot.dll");
		if(!modLib) modLib = LoadLibraryA(dllPath.string().c_str());

		if(!modLib) { // welp
			Mod::get()->disable(); 
			return;
		}

		ASSIGNFUNC(GEODE_PAUSELAYER_CUSTOMSETUP, "GEODE_PAUSELAYER_CUSTOMSETUP");
		ASSIGNFUNC(GEODE_PLAYLAYER_UPDATE, "GEODE_PLAYLAYER_UPDATE");
		ASSIGNFUNC(GEODE_PLAYLAYER_INIT, "GEODE_PLAYLAYER_INIT");
		ASSIGNFUNC(GEODE_GJBASEGAMELAYER_PUSHBUTTON, "GEODE_GJBASEGAMELAYER_PUSHBUTTON");
		ASSIGNFUNC(GEODE_GJBASEGAMELAYER_RELEASEBUTTON, "GEODE_GJBASEGAMELAYER_RELEASEBUTTON");
		ASSIGNFUNC(GEODE_PLAYLAYER_RESETLEVEL, "GEODE_PLAYLAYER_RESETLEVEL");
		ASSIGNFUNC(GEODE_CCSCHEDULER_UPDATE, "GEODE_CCSCHEDULER_UPDATE");
		ASSIGNFUNC(GEODE_PLAYLAYER_REMOVELASTCHECKPOINT, "GEODE_PLAYLAYER_REMOVELASTCHECKPOINT");
		ASSIGNFUNC(GEODE_PLAYLAYER_DESTROYPLAYER, "GEODE_PLAYLAYER_DESTROYPLAYER");
		ASSIGNFUNC(GEODE_PLAYLAYER_ONQUIT, "GEODE_PLAYLAYER_ONQUIT");
		ASSIGNFUNC(GEODE_PLAYLAYER_LEVELCOMPLETE, "GEODE_PLAYLAYER_LEVELCOMPLETE");

		ASSIGNFUNC(GatoBot_setupBot, "?setupBot@GatoBot@@SAXXZ");
		ASSIGNFUNC(GatoBot_setupGeode, "?setupGeode@GatoBot@@SAXXZ");

		// setup
		GatoBot_setupBot();
		GatoBot_setupGeode(); // GEODE SPECIFIC
	}
	else {
		// welp can't do anything without it
		Mod::get()->disable();
	}
}