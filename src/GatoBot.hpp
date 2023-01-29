#ifndef __GATOBOT_INCLUDE__
#define __GATOBOT_INCLUDE__

#include "includes.hpp"

// funny macro
#define MBO(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)

/*
    GD's version of std::string
*/
struct GDH_DLL gdstring {
    union
    {
        char inline_data[16];
        char *ptr;
    } m_data;
    size_t m_size = 0;
    size_t m_capacity = 15;

    gdstring(const char *data, size_t size)
    {
        reinterpret_cast<void *(__thiscall *)(void *, const char *, size_t)>(gd::base + 0xf840)(this, data, size);
    }

    explicit gdstring(const std::string_view &str) : gdstring(str.data(), str.size()) {}
    gdstring(const char *str) : gdstring(std::string_view(str)) {}
    gdstring(const std::string &str) : gdstring(str.c_str(), str.size()) {}

    size_t size() const { return m_size; }

    gdstring &operator=(const std::string &other)
    {
        if (m_capacity > 15)
            delete m_data.ptr;
        reinterpret_cast<void *(__thiscall *)(void *, const char *, size_t)>(gd::base + 0xf840)(this, other.c_str(), other.size());
        return *this;
    }

    const char *c_str() const
    {
        if (m_capacity <= 15)
            return m_data.inline_data;
        return m_data.ptr;
    }

    std::string_view sv() const
    {
        return std::string_view(c_str(), m_size);
    }

    operator std::string() const { return std::string(sv()); }
};

enum ButtonType { None, Pressed, Released };
enum BotStatus { Disabled, Recording, Replaying, Rendering };

struct PlayerData {
    cocos2d::CCPoint position;
    double yVel;
    bool isHolding;
    ButtonType action = None;

    static PlayerData fromPlayer(gd::PlayerObject* player) {
        PlayerData data;
        data.position = MBO(CCPoint, player, 0x67C);
        data.yVel = MBO(double, player, 0x628);
        data.isHolding = MBO(bool, player, 0x611);

        return data;
    }
    void applyToPlayer(gd::PlayerObject* player);
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
    int divideFramesBy = 1;

    std::string extraArguments;

    bool captureMegaHackLabels = false;
    bool captureMegaHackDrawNodes = false;
    bool capturePercentage = false;
    bool includeLevelSong = false;

    bool loadedHooks = false;
};

class LoadingCircle;
class GatoBotMenu;

class GatoBot {
public:
    BotStatus status;
    GatoBotSettings settings;
    GatoBotMenu* botMenu;
    int currentFrame;
    double lastSPF;

    int lastInfoCode;
    LoadingCircle* loadingCircle;

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

    CCLabelBMFont* statusLabel;
    gd::PauseLayer* currentPauseLayer;
    std::vector<GLubyte> currentFrameData;
    bool currentFrameHasData;

    HMODULE cocosBaseAddr;
    std::mutex threadLock;

    std::vector<CheckpointData> practiceCheckpoints;
    std::vector<LevelFrameData> levelFrames;

public:
    static GatoBot* sharedState();

    bool FFmpegInstalled();
    float getTimeForXPos(gd::PlayLayer*);

    void handleFrame(gd::PlayLayer*);
    void handleCheckpoint(gd::PlayLayer*);

    void saveReplay(std::string& filepath);
    void loadReplay(std::string data);
    void updateRender();
    void updateStatusLabel();
    LevelFrameData frameFromString(std::string data);

    void patchMemory(void* loc, std::vector<uint8_t>);
    int getCurrentFPS();
    void visitPlayLayer();

    void retryLevel();
    void setSongPitch(float);

    void toggleRecord(float a = 0, float b = 0);
    void toggleReplay(float a = 0, float b = 0);
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