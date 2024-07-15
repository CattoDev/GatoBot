#include "Bot.hpp"

using namespace geode::prelude;

GatoBot* g_botInstance = nullptr;

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
    if(!g_botInstance) {
        g_botInstance = new GatoBot();
        g_botInstance->updateHooks();
    }

    return g_botInstance;
}

PlayLayer* GatoBot::getPlayLayer() {
    return GameManager::sharedState()->getPlayLayer();
}

Result<> GatoBot::changeStatus(BotStatus newStatus) {
    Result<> result = Ok();
    
    if(m_status == newStatus) return result;

    // switch to idle first if not already
    if(m_status != Idle && newStatus != Idle) {
        (void)this->changeStatus(BotStatus::Idle);

        return this->changeStatus(newStatus);
    }

    switch(newStatus) {
        case Recording: {
            this->resetMacro();
        } break;

        case Replaying: {
            GB_LOG("Replaying {} frames at {} FPS (dt: {})", m_loadedMacro.getFrameCount(), std::round(1.f / m_loadedMacro.getDeltaTime()), m_loadedMacro.getDeltaTime());
        } break;

        case Rendering: {
            // set up the renderer 
            result = this->setupRenderer();

            // failed to set up renderer
            if(result.isErr()) {
                newStatus = BotStatus::Idle;
            }
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
            pLayer->resetLevel();
        }
    }

    m_currentFrame = 0;

    m_status = newStatus;
    this->updateHooks();

    GB_LOG("Changed status {} => {}", statToStr(m_status).c_str(), statToStr(newStatus).c_str());

    return result;
}

int GatoBot::getGameFPS() {
    auto dir = CCDirector::sharedDirector();
    float interval = dir->getAnimationInterval();

    // smth gotta break
    return static_cast<int>(std::round(1.f / interval));
}

void GatoBot::setGameSPF(double spf) {
    CCDirector::sharedDirector()->setAnimationInterval(spf);

    CCApplication::sharedApplication()->setAnimationInterval(spf);
}

void GatoBot::setGameFPS(int fps) {
    //CCDirector::sharedDirector()->setAnimationInterval(1.f / static_cast<float>(fps));

    this->setGameSPF((double)(1.f / static_cast<float>(fps)));
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

    //log::debug("{}", offsetof(GJBaseGameLayer, m_started));

    return 
        !(this->isPlayback() && m_currentFrame >= m_loadedMacro.getFrameCount())   
     //&& TEMP_MBO(bool, pLayer, 0x2ac8) // levelStarted (PlayLayer::startGame)
     //&& TEMP_MBO(bool, pLayer, 0x2ad0) // levelStarted (PlayLayer::startGame)
     && pLayer->m_started
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

RenderParams* GatoBot::getRenderParams() {
    return &m_renderParams;
}

void GatoBot::applyRenderParams(const RenderParams& params) {
    m_renderParams = params;
}

void GatoBot::toggleHook(const std::string& hookName, bool toggle) {
    for(auto& h : Mod::get()->getHooks()) {
        if(h->getDisplayName() == hookName) {
            Result<> res;

            if(toggle) res = h->enable();
            else res = h->disable();
            
            if(res.isErr()) {
                log::error("Error: {}", res.unwrapErr());
            }

            log::debug("Hook {}: {}", hookName, toggle);       
            break;
        }
    }
}

void GatoBot::updateHooks() {
    this->toggleHook("cocos2d::CCScheduler::update", m_status != BotStatus::Idle);
    //this->toggleHook("glViewport", m_status == BotStatus::Rendering);
}

void GatoBot::applyWinSize() {
    CCSize& size = m_renderParams.m_newDesignRes;

    if(size.width != 0 && size.height != 0) {
        auto view = CCEGLView::get();
        
        CCDirector::get()->m_obWinSizeInPoints = size;
        view->setDesignResolutionSize(size.width, size.height, ResolutionPolicy::kResolutionExactFit);
        view->m_fScaleX = m_renderParams.m_newScreenScaleX;
        view->m_fScaleY = m_renderParams.m_newScreenScaleY;
    }
}

void GatoBot::restoreWinSize() {
    CCSize& size = m_renderParams.m_originalDesignRes;

    if(size.width != 0 && size.height != 0) {
        auto view = CCEGLView::get();

        CCDirector::get()->m_obWinSizeInPoints = size;
        view->setDesignResolutionSize(size.width, size.height, ResolutionPolicy::kResolutionExactFit);
        view->m_fScaleX = m_renderParams.m_originalScreenScaleX;
        view->m_fScaleY = m_renderParams.m_originalScreenScaleY;
    }
}

void GatoBot::updateBot(float& dt) {
    const bool gamePaused = this->getPlayLayer() != nullptr ? this->getPlayLayer()->m_isPaused : true;

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

            default: break;
        }
    }
}

void GatoBot::updateCommon(float& dt) {
    if(m_status != Recording) {
        // out of frames
        if(m_currentFrame >= m_loadedMacro.getFrameCount()) {
            (void)this->changeStatus(BotStatus::Idle);
            return;
        }
    }

    // lock delta
    float deltaTime = m_loadedMacro.getDeltaTime();
    
    dt = deltaTime;
    this->setGameSPF((double)deltaTime / (double)this->getMainSpeed());
}

void GatoBot::onLevelReset() {
    GB_LOG("onLevelReset");

    if(m_status == BotStatus::Idle) return;

    // practice mode
    auto pLayer = this->getPlayLayer();
    const auto checkpoints = pLayer->m_checkpointArray;

    if(!(pLayer->m_isPracticeMode && checkpoints->count())) {
        m_currentFrame = 0;

        if(m_status == Recording) {
            m_loadedMacro.clearFramesFrom(m_currentFrame);
        }

        //TEMP_MBO(double, pLayer, 0x2ac8) = 0;
        //TEMP_MBO(int, pLayer, 0x2b04) = 0;
        //TEMP_MBO(float, pLayer, 0x2d0) = 1.f;

        TEMP_MBO(double, pLayer, 0x3248) = 0;
        TEMP_MBO(int, pLayer, 0x329c) = 0;
        TEMP_MBO(float, pLayer, 0x330) = 1.f;
    }
    else {

    }
}

FrameState GatoBot::createFrameState() {
    FrameState state;

    auto pLayer = this->getPlayLayer();

    state.m_frame = m_currentFrame;

    state.m_unk1 = TEMP_MBO(double, pLayer, 0x3248);
    state.m_unk2 = TEMP_MBO(int, pLayer, 0x329c);
    state.m_unk3 = TEMP_MBO(float, pLayer, 0x330);
    
    return state;
}

FrameState& GatoBot::getLastFrameState() {
    return m_loadedMacro.getLastFrame().m_frameState;
}

void GatoBot::loadFrameState(const FrameState& state, bool clearFrames) {
    GB_LOG("loadFrameState: {}", state.m_frame);

    auto pLayer = this->getPlayLayer();

    m_currentFrame = state.m_frame;
    TEMP_MBO(double, pLayer, 0x3248) = state.m_unk1;
    TEMP_MBO(int, pLayer, 0x329c) = state.m_unk2;
    TEMP_MBO(float, pLayer, 0x330) = state.m_unk3;

    // clear frames after current framestate
    if(clearFrames) {
        m_loadedMacro.clearFramesFrom(m_currentFrame);
    }
}

void GatoBot::botFinished(BotStatus oldStatus) {
    GB_LOG("botFinished => m_firstSPF: {}", m_firstSPF);

    this->setGameSPF(m_firstSPF);
    CCScheduler::get()->setTimeScale(1);

    if(oldStatus == Recording) {
        m_loadedMacro.recordingFinished();
    }

    if(oldStatus == Rendering) {
        m_encoder->encodingFinished();
        CC_SAFE_DELETE(m_encoder);

        CCEGLView::get()->setDesignResolutionSize(m_renderParams.m_originalDesignRes.width, m_renderParams.m_originalDesignRes.height, ResolutionPolicy::kResolutionExactFit);
    }
}

void GatoBot::finishPlayback() {
    if(!this->isPlayback()) return;

    (void)this->changeStatus(BotStatus::Idle);
}