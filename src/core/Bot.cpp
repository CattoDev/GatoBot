#include "Bot.hpp"

using namespace geode::prelude;

GatoBot* g_instance = nullptr;

// debug func (temp)
std::string statToStr(BotStatus stat) {
    std::string str;

    switch(stat) {
        case Idle: {
            str = "Idle";
        } break;

        case Recording: {
            str = "Recording";
        } break;

        case Replaying: {
            str = "Replaying";
        } break;

        case Rendering: {
            str = "Rendering";
        } break;
    }

    return str;
}

GatoBot* GatoBot::get() {
    if(!g_instance) {
        g_instance = new GatoBot();
    }

    return g_instance;
}

PlayLayer* GatoBot::getPlayLayer() {
    //return TEMP_MBO(PlayLayer*, GameManager::sharedState(), 0x198);
    return GameManager::sharedState()->getPlayLayer();
}

void GatoBot::changeStatus(BotStatus newStatus) {
    if(m_status == newStatus) return;

    // switch to idle first if not already
    if(m_status != Idle && newStatus != Idle) {
        this->changeStatus(BotStatus::Idle);
        this->changeStatus(newStatus);
        
        return;
    }

    switch(newStatus) {
        case Recording: {
            this->resetMacro();
        } break;

        case Replaying: {
            GB_LOG("Replaying {} frames at {} FPS (dt: {})", m_loadedMacro.getFrameCount(), std::round(1.f / m_loadedMacro.getDeltaTime()), m_loadedMacro.getDeltaTime());
        } break;

        case Rendering: {
            this->setupRenderer();
        } break;
        
        default: { // Idle
            this->botFinished(m_status);
        } break;
    }

    if(newStatus != Idle) {
        // save animation interval (SPF)
        m_firstSPF = CCDirector::sharedDirector()->getAnimationInterval();

        // restart attempt
        if(auto pLayer = this->getPlayLayer()) {
            //pLayer->resetLevel();
        }
    }

    m_currentFrame = 0;

    GB_LOG("Changed status {} => {}", statToStr(m_status).c_str(), statToStr(newStatus).c_str());

    m_status = newStatus;
}

int GatoBot::getGameFPS() {
    auto dir = CCDirector::sharedDirector();
    float interval = dir->getAnimationInterval();

    // smth gotta break
    return static_cast<int>(std::round(1.f / interval));
}

void GatoBot::setGameFPS(int fps) {
    CCDirector::sharedDirector()->setAnimationInterval(1.f / static_cast<float>(fps));
}

void GatoBot::setMainSpeed(float speed) {
    m_mainSpeed = speed;
}

float GatoBot::getMainSpeed() {
    return m_mainSpeed;
}

bool GatoBot::canPerform() {
    auto pLayer = this->getPlayLayer();

    if(!pLayer)
        return false;

    return 
        !(this->isPlayback() && m_currentFrame >= m_loadedMacro.getFrameCount())   
     //&& TEMP_MBO(bool, pLayer, 0x2ac8) // levelStarted (PlayLayer::startGame)
     && TEMP_MBO(bool, pLayer, 0x2ad0) // levelStarted (PlayLayer::startGame)
    ;
}

BotStatus GatoBot::getStatus() {
    return m_status;
}

bool GatoBot::isPlayback() {
    return m_status == Replaying || m_status == Rendering;
}

void GatoBot::resetMacro() {
    Macro macro;

    macro.prepareMacro(this->getGameFPS());
    m_loadedMacro = std::move(macro);
}

int GatoBot::getCurrentFrameNum() {
    return m_currentFrame;
}

Macro& GatoBot::getMacro() {
    return m_loadedMacro;
}

void GatoBot::updatePlayLayer(float& dt) {
    // game paused
    // [PlayLayer + 0x2ef7] not anymore
    // PlayLayer + 0x2f17
    const bool gamePaused = this->getPlayLayer() != nullptr ? TEMP_MBO(bool, this->getPlayLayer(), 0x2f17) : true;

    if(m_status != BotStatus::Idle && !gamePaused) {
        this->updateCommon(dt);

        switch(m_status) {
            case Recording: {
                this->updateRecording();
            } break;

            case Replaying: {
                this->updateReplaying();
            } break;

            case Rendering: {
                this->updateRendering();
            } break;
        }
    }
}

void GatoBot::updateCommon(float& dt) {
    if(m_status != Recording) {
        // out of frames
        if(m_currentFrame >= m_loadedMacro.getFrameCount()) {
            this->changeStatus(BotStatus::Idle);
            return;
        }
    }

    // lock delta
    float deltaTime = m_loadedMacro.getDeltaTime();
    const float timeScale = CCScheduler::get()->getTimeScale();
    
    float newInterval = deltaTime / timeScale;
    dt = newInterval;
    CCDirector::sharedDirector()->setAnimationInterval(newInterval / m_mainSpeed);
}

void GatoBot::onLevelReset() {
    GB_LOG("onLevelReset");

    if(m_status == BotStatus::Idle) return;

    // practice mode
    auto pLayer = this->getPlayLayer();
    //bool practiceMode = TEMP_MBO(bool, pLayer, 0x2a74);
    auto checkpoints = TEMP_MBO(CCArray*, pLayer, 0x2e18);

    /*if(m_status == Recording) {
        if(!practiceMode || !checkpoints->count()) {
            m_currentFrame = 0;
            m_loadedMacro.clearFramesFrom(0);
        }
        else {
            // 
        }
    }
    else {
        m_currentFrame = 0;
    }*/

    /*if(m_status == Recording) {
        // fix frames 
        if(pLayer->m_isPracticeMode && checkpoints->count()) {
            //this->loadFrameState(this->getLastFrameState());
        }
        else {
            m_currentFrame = 0;
        }

        m_loadedMacro.clearFramesFrom(m_currentFrame);
    }
    else {
        m_currentFrame = 0;
    }

    TEMP_MBO(double, pLayer, 0x2ac8) = 0;
    TEMP_MBO(int, pLayer, 0x2b04) = 0;
    TEMP_MBO(float, pLayer, 0x2d0) = 1.f;*/

    if(!(pLayer->m_isPracticeMode && checkpoints->count())) {
        m_currentFrame = 0;

        if(m_status == Recording) {
            m_loadedMacro.clearFramesFrom(m_currentFrame);
        }

        TEMP_MBO(double, pLayer, 0x2ac8) = 0;
        TEMP_MBO(int, pLayer, 0x2b04) = 0;
        TEMP_MBO(float, pLayer, 0x2d0) = 1.f;
    }
    else {

    }
}

FrameState GatoBot::createFrameState() {
    FrameState state;

    auto pLayer = this->getPlayLayer();

    state.m_frame = m_currentFrame;
    state.m_unk1 = TEMP_MBO(double, pLayer, 0x2ac8);
    state.m_unk2 = TEMP_MBO(int, pLayer, 0x2b04);
    state.m_unk3 = TEMP_MBO(float, pLayer, 0x2d0);
    
    return state;
}

FrameState& GatoBot::getLastFrameState() {
    return m_loadedMacro.getLastFrame().m_frameState;
}

void GatoBot::loadFrameState(const FrameState& state, bool clearFrames) {
    GB_LOG("loadFrameState: {}", state.m_frame);

    auto pLayer = this->getPlayLayer();

    m_currentFrame = state.m_frame;
    TEMP_MBO(double, pLayer, 0x2ac8) = state.m_unk1;
    TEMP_MBO(int, pLayer, 0x2b04) = state.m_unk2;
    TEMP_MBO(float, pLayer, 0x2d0) = state.m_unk3;

    // clear frames after current framestate
    if(clearFrames) {
        m_loadedMacro.clearFramesFrom(m_currentFrame);
    }
}

void GatoBot::botFinished(BotStatus oldStatus) {
    GB_LOG("botFinished => m_firstSPF: {}", m_firstSPF);

    CCDirector::sharedDirector()->setAnimationInterval(m_firstSPF);
    CCScheduler::get()->setTimeScale(1);

    if(oldStatus == Recording) {
        m_loadedMacro.recordingFinished();
    }

    if(oldStatus == Rendering) {
        m_encoder->encodingFinished();
        CC_SAFE_DELETE(m_encoder);
    }
}

void GatoBot::finishPlayback() {
    if(!this->isPlayback()) return;

    this->changeStatus(BotStatus::Idle);
}