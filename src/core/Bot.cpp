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
        //g_instance->m_allFrames.reserve(100000); // allocate
    }

    return g_instance;
}

PlayLayer* GatoBot::getPlayLayer() {
    return TEMP_MBO(PlayLayer*, GameManager::sharedState(), 0x198);
}

void GatoBot::changeStatus(BotStatus newStatus) {
    if(m_status == newStatus) return;

    /*
        TODO: rewrite this entire function
    */

    GB_LOG("Changing status: {} -> {} ({})", std::to_string(m_status), std::to_string(newStatus), statToStr(newStatus));

    if(newStatus == Recording) {
        this->resetMacro();

        // debug
        CCScheduler::get()->setTimeScale(.25f);
    }
    if(newStatus == Replaying) {
        GB_LOG("Replaying {} frames at {} FPS (dt: {})", m_loadedMacro.getFrameCount(), std::round(1.f / m_loadedMacro.getDeltaTime()), m_loadedMacro.getDeltaTime());
    }

    if(newStatus != Idle && m_status == Idle) {
        m_firstSPF = CCDirector::sharedDirector()->getAnimationInterval();
    }
    else {
        this->botFinished();
    }

    //GB_LOG("m_allFrames: {}", m_loadedMacro.getFrameCount());

    m_status = newStatus;
}

int GatoBot::getGameFPS() {
    auto dir = CCDirector::sharedDirector();
    float interval = dir->getAnimationInterval();

    // smth gotta break
    return static_cast<int>(std::round(1.f / interval));
}

bool GatoBot::canPerform() {
    auto pLayer = this->getPlayLayer();

    if(!pLayer)
        return false;

    return 
        !(this->isPlayback() && m_currentFrame >= m_loadedMacro.getFrameCount())   
     && TEMP_MBO(bool, pLayer, 0x2ac8) // levelStarted (PlayLayer::startGame)
    ;
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

bool GatoBot::updatePlayLayer(float& dt) {
    bool canAdvance = true;

    // game paused
    // PlayLayer + 0x2ef7
    bool gamePaused = this->getPlayLayer() != nullptr ? TEMP_MBO(bool, this->getPlayLayer(), 0x2ef7) : true;

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
                //canAdvance = this->updateRendering(dt);
            } break;
        }
    }

    return canAdvance;
}

void GatoBot::updateCommon(float& dt) {
    if(m_status != Recording) {
        // out of frames
        if(m_currentFrame >= m_loadedMacro.getFrameCount()) {
            this->changeStatus(BotStatus::Idle);
            this->botFinished();
            return;
        }
    }

    // lock delta
    float deltaTime = m_loadedMacro.getDeltaTime();
    const float timeScale = CCScheduler::get()->getTimeScale();
    
    float newInterval = deltaTime / timeScale;
    dt = newInterval;
    CCDirector::sharedDirector()->setAnimationInterval(newInterval);

    //GB_LOG("new SPF: {} ({} FPS)", newInterval, std::round(1.f / newInterval));
}

void GatoBot::onLevelReset() {
    GB_LOG("onLevelReset");

    // practice mode
    auto pLayer = this->getPlayLayer();
    bool practiceMode = TEMP_MBO(bool, pLayer, 0x2a74);
    auto checkpoints = TEMP_MBO(CCArray*, pLayer, 0x2df8);

    if(m_status == Recording) {
        if(!practiceMode || !checkpoints->count()) {
            m_currentFrame = 0;
            m_loadedMacro.clearFramesAfter(0);
        }
    }
    else {
        m_currentFrame = 0;
    }
}

void GatoBot::checkpointLoaded(int frame) {
    GB_LOG("checkpointLoaded: {}", frame);

    m_loadedMacro.clearFramesAfter(frame);
    m_currentFrame = frame;
}

void GatoBot::botFinished() {
    CCDirector::sharedDirector()->setAnimationInterval(m_firstSPF);
    CCScheduler::get()->setTimeScale(1);
}