#ifndef __GATOBOT_INCLUDE__
#define __GATOBOT_INCLUDE__

#include "includes.hpp"

// funny macro
#define MBO(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)

enum ButtonType { None, Pressed, Released };
enum BotStatus { Disabled, Recording, Replaying, Rendering };
enum ReplayType { GatoBotR, MegaHack, MegaHackJson };
enum ReplayLoadStatus { Success, MissingFrames, Failed };
enum ToggleHookType { SchedulerUpdate, NodeVisit };

struct PlayerData {
    cocos2d::CCPoint position = CCPointZero;
    double yVel;
    bool isHolding;
    ButtonType action = None;
    float rotation = -1;
    bool isSet = false;

    PlayerData() {} // ?

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

struct GatoBotSettings {
    int bitrate = 50000; //K
    int audioBitrate = 192; //K
    std::string videoPath = "";
    std::string codec = "h264";

    int targetWidth;
    int targetHeight;
    int targetFPS;
    int targetGameFPS;
    float targetSPF;
    float targetSpeed;
    int divideFramesBy = 1;
    float renderDelay;

    std::string extraArguments;

    bool captureMegaHackLabels = false;
    bool captureMegaHackDrawNodes = false;
    bool capturePercentage = false;
    bool includeLevelSong = false;
    bool showConsoleWindow = false;
    bool fastRender = false;
    bool delayEnd = false;

    bool loadedHooks = false;
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
    int targetFPS;

    // used only for rendering
    float timeFromStart;
    float endDelayStart;

    bool player1holding;
    bool player2holding;
    bool gamePaused;
    bool presetSettings;

    CCLabelBMFont* statusLabel;
    gd::PauseLayer* currentPauseLayer;
    std::vector<GLubyte> currentFrameData;
    bool currentFrameHasData;

    HMODULE cocosBaseAddr;
    std::mutex threadLock;

    std::vector<CheckpointData> practiceCheckpoints;
    std::vector<LevelFrameData> levelFrames;

    // hook addresses
    LPVOID updateHookAddr;

public:
    static GatoBot* sharedState();
    static void setupBot();
    void preset();

    bool FFmpegInstalled();
    float getTimeForXPos(gd::PlayLayer*);

    void handleFrame(gd::PlayLayer*);
    void handleCheckpoint(gd::PlayLayer*);
    void handleClick(gd::GJBaseGameLayer*, bool, ButtonType);

    void saveReplay(std::string& filepath);
    ReplayLoadStatus loadReplay(std::vector<char>&, ReplayType);
    void updateRender();
    void updateStatusLabel();
    LevelFrameData frameFromString(std::string data);

    void patchMemory(void* loc, std::vector<uint8_t>);
    int getCurrentFPS();
    void visitPlayLayer();

    void retryLevel();
    float getSongPitch();
    void setSongPitch(float);
    void organizeReplay();
    bool hasMissingFrames();

    void toggleRecord(int a = 0, float b = 0);
    void toggleReplay(int a = 0, float b = 0);
    void toggleRender();
    void toggleRenderDelayed();

    void toggleGameFPSCap(bool);
    void toggleHook(ToggleHookType, bool);

    void resetBasicVariables(bool);

    // callbacks
    void openBotMenu(CCObject*);
    void saveCurrentReplay();
    void loadNewReplay();

    // other shit
    static inline int (*FMOD_Channel_setPosition)();
    static inline void (*FMOD_Channel_getPitch)(FMOD::Channel*, float*);
    static inline void (*FMOD_Channel_setPitch)();
    static inline gdstring (*ZipUtils_compressString)(gdstring, bool, int);
    static inline gdstring (*ZipUtils_decompressString)(gdstring, bool, int);
    static inline void (__thiscall* PauseLayer_onRetry)(gd::PauseLayer*, CCObject*);
};

#endif