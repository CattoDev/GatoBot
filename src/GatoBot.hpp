#ifndef __GATOBOT_INCLUDE__
#define __GATOBOT_INCLUDE__

#include "includes.hpp"
#include "Settings.hpp"

// funny macro
#define MBO(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)

enum ButtonType { None, Pressed, Released };
enum BotStatus { Disabled, Recording, Replaying, Rendering };
enum ReplayType { GatoBotR, MegaHack, MegaHackJson };
enum ReplayLoadStatus { Success, MissingFrames, Failed };
enum ToggleHookType { SchedulerUpdate, DrawScene };

struct PlayerData {
    cocos2d::CCPoint position = CCPointZero;
    double yVel;
    bool isHolding;
    ButtonType action = None;
    float rotation = -1;
    bool isSet = false;

    PlayerData() {}

    static PlayerData fromPlayer(gd::PlayerObject* player) {
        PlayerData data;
        data.position = MBO(CCPoint, player, 0x67C);
        data.yVel = MBO(double, player, 0x628);
        //data.isHolding = MBO(bool, player, 0x611);
        data.rotation = player->getRotation();
        data.isSet = true;

        return data;
    }
    void applyToPlayer(gd::PlayerObject* player);
    static PlayerData fromJson(nlohmann::json, PlayerData);
};

struct LevelFrameData {
    int frame = -1;
    PlayerData player1;
    PlayerData player2;
};

struct CheckpointData {
    gd::CheckpointObject* object;
    LevelFrameData frame;
};

struct ReplayLoadResponse {
    ReplayLoadStatus status;
    int fps = -1;
};

struct StatusChangeData {
    int FPS = 0;
    float speed = 0;
};

class GBLoadingCircle;
class GatoBotMenu;

class GatoBot : public CCNode {
public:
    BotStatus status;
    GatoBotSettings settings;
    GatoBotMenu* botMenu;
    int currentFrame;
    double lastSPF;

    int lastInfoCode;
    GBLoadingCircle* loadingCircle;

    ButtonType queuedBtnP1;
    ButtonType queuedBtnP2;
    ButtonType lastBtnP1;
    ButtonType lastBtnP2;

    // Rendering stuff
    CCTexture2D* renderingTexture;
    GLint m_pOldFBO;
    GLuint m_pFBO;
    GLint m_pOldRBO;

    float frameDelta;
    float currentMusicOffset;

    // used only for rendering
    clock_t totalRenderTimeStart;
    clock_t timeFromLastEsc;
    float timeFromStart = 0;
    float endDelayStart = 0;
    bool scheduledPause = false;
    bool inPlayLayer = false;

    bool player1holding;
    bool player2holding;
    bool gamePaused;
    bool presetSettings;
    bool renderSettingsSaved = false;
    bool loadedHooks = false;

    CCLabelBMFont* statusLabel;
    CCLabelBMFont* exitLabel;
    gd::PauseLayer* currentPauseLayer;
    std::vector<GLubyte> currentFrameData;
    bool currentFrameHasData;

    HMODULE cocosBaseAddr;

    std::vector<CheckpointData> practiceCheckpoints;
    std::vector<LevelFrameData> levelFrames;

    // hook addresses
    LPVOID updateHookAddr;

public:
    static GatoBot* sharedState();
    void setupBot();
    static void setupBasicHooks();
    void preset();

    bool FFmpegInstalled();
    float getTimeForXPos(gd::PlayLayer*);

    void handleFrame(gd::PlayLayer*);
    void handleCheckpoint(gd::PlayLayer*);
    bool handleClick(gd::GJBaseGameLayer*, bool, ButtonType);

    void saveReplay(std::string& filepath);
    ReplayLoadResponse loadReplay(std::vector<char>&, ReplayType);
    void updateStatusLabel();
    LevelFrameData frameFromString(std::string data);

    void patchMemory(void* loc, std::vector<uint8_t>);
    int getCurrentFPS();
    void visitPlayLayer();

    void retryLevel();
    void pauseLevel();
    float getSongPitch();
    void setSongPitch(float);
    void organizeReplay();
    bool hasMissingFrames();

    void renderFrame();
    void toggleRendering(bool);
    void botStatusChanged();
    void changeStatus(BotStatus, StatusChangeData d = {});
    void updateExitText();
    bool isDelayedRendering();

    bool updatePlayLayer(float&);
    void updateCommon(float&);
    void updateRecording();
    void updateReplaying();
    bool updateRendering(float&);

    void toggleGameFPSCap(bool);
    void toggleHook(ToggleHookType, bool);
    void toggleAnticheat(bool);

    void resetBasicVariables(bool);
    void checkErrors();

    // callbacks
    void openBotMenu(CCObject*);
    void saveCurrentReplay();
    void loadNewReplay();

    // other shit
    static inline gdstring (*ZipUtils_compressString)(gdstring, bool, int);
    static inline gdstring (*ZipUtils_decompressString)(gdstring, bool, int);
    static inline void (__thiscall* PauseLayer_onRetry)(gd::PauseLayer*, CCObject*);
};

#endif