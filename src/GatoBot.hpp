#ifndef __GATOBOT_INCLUDE__
#define __GATOBOT_INCLUDE__

#include "includes.hpp"

// exports
#define GB_DLL __declspec(dllexport)

// funny macro
#define MBO(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)

enum ButtonType { None, Pressed, Released };
enum BotStatus { Disabled, Recording, Replaying, Rendering };
enum ReplayType { GatoBotR, MegaHack };
enum ReplayLoadStatus { Success, MissingFrames, Failed };

struct PlayerData {
    cocos2d::CCPoint position;
    double yVel;
    bool isHolding;
    ButtonType action = None;

    PlayerData() {} // ?

    static PlayerData fromPlayer(gd::PlayerObject* player) {
        PlayerData data;
        data.position = MBO(CCPoint, player, 0x67C);
        data.yVel = MBO(double, player, 0x628);
        data.isHolding = MBO(bool, player, 0x611);

        return data;
    }
    void applyToPlayer(gd::PlayerObject* player);
    static PlayerData fromJson(nlohmann::json);
};

struct LevelFrameData {
    int frame;
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

    bool showConsoleWindow = false;
    int targetWidth;
    int targetHeight;
    int targetFPS;
    int targetGameFPS;
    float targetSPF;
    float targetSpeed;
    int divideFramesBy = 1;

    std::string extraArguments;

    bool captureMegaHackLabels = false;
    bool captureMegaHackDrawNodes = false;
    bool capturePercentage = false;
    bool includeLevelSong = false;

    bool loadedHooks = false;
};

class GBLoadingCircle;
class GatoBotMenu;

class GatoBot {
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

    // Rendering stuff
    CCTexture2D* renderingTexture;
    GLint m_pOldFBO;
    GLuint m_pFBO;
    GLint m_pOldRBO;

    float frameDelta;
    float currentMusicOffset;

    // used only for rendering
    float timeFromStart;

    bool player1holding;
    bool player2holding;
    bool gamePaused;
    bool presetSettings;
    bool usingGeode;

    CCLabelBMFont* statusLabel;
    gd::PauseLayer* currentPauseLayer;
    std::vector<GLubyte> currentFrameData;
    bool currentFrameHasData;

    HMODULE cocosBaseAddr;
    std::mutex threadLock;

    std::vector<CheckpointData> practiceCheckpoints;
    std::vector<LevelFrameData> levelFrames;

    // update hook
    LPVOID updateHookAddr;

public:
    static GatoBot* sharedState();
    GB_DLL static void setupBot();
    GB_DLL static void setupGeode(); // geode only
    void preset();

    bool FFmpegInstalled();
    float getTimeForXPos(gd::PlayLayer*);

    void handleFrame(gd::PlayLayer*);
    void handleCheckpoint(gd::PlayLayer*);
    void handleClick(gd::GJBaseGameLayer*, bool, ButtonType);

    void saveReplay(std::string& filepath);
    ReplayLoadStatus loadReplay(std::string data, ReplayType);
    void updateRender();
    void updateStatusLabel();
    LevelFrameData frameFromString(std::string data);

    void patchMemory(void* loc, std::vector<uint8_t>);
    int getCurrentFPS();
    void visitPlayLayer();

    void retryLevel();
    void setSongPitch(float);

    void toggleRecord(int a = 0, float b = 0);
    void toggleReplay(int a = 0, float b = 0);
    void toggleRender();

    void toggleGameFPSCap(bool);

    void resetBasicVariables();

    // callbacks
    void openBotMenu(CCObject*);
    void saveCurrentReplay();
    void loadNewReplay();

    // other shit
    static inline int (*FMOD_Channel_setPosition)();
    static inline int (*FMOD_Channel_setPitch)();
    static inline gdstring (*ZipUtils_compressString)(gdstring, bool, int);
    static inline gdstring (*ZipUtils_decompressString)(gdstring, bool, int);
    static inline void (__thiscall* PauseLayer_onRetry)(gd::PauseLayer*, CCObject*);
};

#endif