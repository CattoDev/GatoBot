#include "Bot.hpp"

using namespace geode::prelude;

GatoBot* g_botInstance = nullptr;

// debug func
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
            log::debug("Replaying {} frames at {} FPS (dt: {})", m_loadedMacro.getFrameCount(), std::round(1.f / m_loadedMacro.getDeltaTime()), m_loadedMacro.getDeltaTime());
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
        if(newStatus != Rendering) {
            auto pLayer = this->getPlayLayer();
            if(pLayer) pLayer->resetLevel();
        }
    }

    m_currentFrame = 0;

    log::debug("Changed status {} => {}", statToStr(m_status), statToStr(newStatus));

    m_status = newStatus;
    this->updateHooks();

    return result;
}

int GatoBot::getGameFPS() {
    auto dir = CCDirector::sharedDirector();
    float interval = dir->getAnimationInterval();

    // smth gotta break
    return static_cast<int>(std::round(1.f / interval));
}

void GatoBot::setGameSPF(double spf) {
    //log::debug("GatoBot::setGameSPF: {}", spf);

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

    return 
        !(this->isPlayback() && m_currentFrame >= m_loadedMacro.getFrameCount())
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

Encoder* GatoBot::getEncoder() {
    return m_encoder;
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

void GatoBot::copyVolume() {
    auto fmod = FMODAudioEngine::sharedEngine();

    m_renderParams.m_originalMusicVolume = fmod->m_musicVolume;
    m_renderParams.m_originalSFXVolume = fmod->m_sfxVolume;
}

void GatoBot::setVolume(float volume) {
    auto fmod = FMODAudioEngine::sharedEngine();

    fmod->m_backgroundMusicChannel->setVolume(volume);
    fmod->m_currentSoundChannel->setVolume(volume);
    fmod->m_musicVolume = volume;

    fmod->m_globalChannel->setVolume(volume);
    fmod->m_sfxVolume = volume;
}

void GatoBot::resetVolume() {
    auto fmod = FMODAudioEngine::sharedEngine();

    fmod->m_backgroundMusicChannel->setVolume(m_renderParams.m_originalMusicVolume);
    fmod->m_currentSoundChannel->setVolume(m_renderParams.m_originalMusicVolume);
    fmod->m_musicVolume = m_renderParams.m_originalMusicVolume;

    fmod->m_globalChannel->setVolume(m_renderParams.m_originalSFXVolume);
    fmod->m_sfxVolume = m_renderParams.m_originalSFXVolume;
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

void GatoBot::levelEntered(PlayLayer* pLayer) {
    log::debug("GatoBot::levelEntered");

    // get the frame delta factor offsets
    // for each platform cuz I'm too lazy
    // to add bindings
    // TODO: use actual members (ty sleepyut)
    #define MBO_PTR(_type, _class, _offset) reinterpret_cast<_type*>(reinterpret_cast<uintptr_t>(_class) + _offset)

    #ifdef GEODE_IS_WINDOWS
    m_frameDeltaFactorPtrs.m_unk1 = MBO_PTR(double, pLayer, 0x3248);
    m_frameDeltaFactorPtrs.m_unk2 = MBO_PTR(int, pLayer, 0x329c);
    m_frameDeltaFactorPtrs.m_unk3 = MBO_PTR(float, pLayer, 0x330);
    #endif

    #ifdef GEODE_IS_ANDROID
    m_frameDeltaFactorPtrs.m_unk1 = MBO_PTR(double, pLayer, 0x1);
    m_frameDeltaFactorPtrs.m_unk2 = MBO_PTR(int, pLayer, 0x1);
    m_frameDeltaFactorPtrs.m_unk3 = MBO_PTR(float, pLayer, 0x1);
    #endif
}

void GatoBot::levelStarted() {
    if(this->getStatus() != BotStatus::Idle) {
        if(m_encoder) {
            m_encoder->levelStarted();
        }
    }
}

void GatoBot::onLevelReset() {
    log::debug("GatoBot::onLevelReset");

    if(m_status == BotStatus::Idle) return;

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

        //TEMP_MBO(double, pLayer, 0x3248) = 0;
        //TEMP_MBO(int, pLayer, 0x329c) = 0;
        //TEMP_MBO(float, pLayer, 0x330) = 1.f;

        *m_frameDeltaFactorPtrs.m_unk1 = 0;
        *m_frameDeltaFactorPtrs.m_unk2 = 0;
        *m_frameDeltaFactorPtrs.m_unk3 = 1.f;
    }
}

FrameState GatoBot::createFrameState() {
    FrameState state;

    auto pLayer = this->getPlayLayer();

    state.m_frame = m_currentFrame;

    state.m_player1 = PlayerData {
        pLayer->m_player1->m_position.x,
        pLayer->m_player1->m_position.y,
        pLayer->m_player1->m_yVelocity
    };

    state.m_player2 = PlayerData {
        pLayer->m_player2->m_position.x,
        pLayer->m_player2->m_position.y,
        pLayer->m_player2->m_yVelocity
    };

    state.m_frameDeltaFactors.m_unk1 = *m_frameDeltaFactorPtrs.m_unk1;
    state.m_frameDeltaFactors.m_unk2 = *m_frameDeltaFactorPtrs.m_unk2;
    state.m_frameDeltaFactors.m_unk3 = *m_frameDeltaFactorPtrs.m_unk3;
    
    return std::move(state);
}

FrameState& GatoBot::getLastFrameState() {
    return m_loadedMacro.getLastFrame().m_frameState;
}

void GatoBot::loadFrameState(const FrameState& state, bool clearFrames) {
    log::debug("GatoBot::loadFrameState: {}", state.m_frame);

    auto pLayer = this->getPlayLayer();

    m_currentFrame = state.m_frame;

    *m_frameDeltaFactorPtrs.m_unk1 = state.m_frameDeltaFactors.m_unk1;
    *m_frameDeltaFactorPtrs.m_unk2 = state.m_frameDeltaFactors.m_unk2;
    *m_frameDeltaFactorPtrs.m_unk3 = state.m_frameDeltaFactors.m_unk3;

    // restore player velocity
    pLayer->m_player1->m_yVelocity = state.m_player1.m_yVel;
    pLayer->m_player2->m_yVelocity = state.m_player2.m_yVel;

    // clear frames after current FrameState
    if(clearFrames) {
        m_loadedMacro.clearFramesFrom(m_currentFrame);
    }
}

void GatoBot::botFinished(BotStatus oldStatus) {
    log::debug("GatoBot::botFinished");

    this->setGameSPF(m_firstSPF);
    CCScheduler::get()->setTimeScale(1);

    if(oldStatus == Recording) {
        m_loadedMacro.recordingFinished();
    }

    if(oldStatus == Rendering) {
        // free encoder
        m_encoder->encodingFinished();
        CC_SAFE_DELETE(m_encoder);

        // reset audio settings
        this->resetVolume();

        auto fmod = FMODAudioEngine::sharedEngine();
        fmod->m_system->setOutput(m_renderParams.m_FMODOutputType);

        // reset aspect ratio
        CCEGLView::get()->setDesignResolutionSize(m_renderParams.m_originalDesignRes.width, m_renderParams.m_originalDesignRes.height, ResolutionPolicy::kResolutionExactFit);
    }
}

void GatoBot::finishPlayback() {
    if(!this->isPlayback()) return;

    (void)this->changeStatus(BotStatus::Idle);
}