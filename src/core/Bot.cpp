#include "Bot.hpp"

using namespace geode::prelude;

GatoBot* g_botInstance = nullptr;

// debug func
std::string statToStr(BotStatus stat) {
    std::string str;

    switch(stat) {
        case BotStatus::Idle: {
            str = "Idle";
        } break;

        case BotStatus::Recording: {
            str = "Recording";
        } break;

        case BotStatus::Replaying: {
            str = "Replaying";
        } break;

        case BotStatus::Rendering: {
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
    if(m_status != BotStatus::Idle && newStatus != BotStatus::Idle) {
        (void)this->changeStatus(BotStatus::Idle);

        return this->changeStatus(newStatus);
    }

    switch(newStatus) {
        case BotStatus::Recording: {
            this->resetMacro();
        } break;

        case BotStatus::Replaying: {
            log::debug("Replaying {} steps", m_loadedMacro.getStepCount());
        } break;

        case BotStatus::Rendering: {
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

    m_currentStep = 0;

    log::debug("Changed status {} => {}", statToStr(m_status), statToStr(newStatus));

    m_status = newStatus;
    this->updateHooks();

    return result;
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
        !(this->isPlayback() && m_currentStep >= m_loadedMacro.getStepCount())
       && pLayer->m_started
    ;
}

BotStatus GatoBot::getStatus() {
    return m_status;
}

bool GatoBot::isPlayback() {
    return m_status == BotStatus::Replaying || m_status == BotStatus::Rendering;
}

void GatoBot::resetMacro() {
    Macro macro;

    // TODO: tps bypass
    macro.prepareMacro(240);
    m_loadedMacro = std::move(macro);
}

void GatoBot::queuePlayerCommand(const PlayerButtonCommand& cmd) {
    m_queuedPlayerCommands.push_back(cmd);
}

void GatoBot::clearQueuedCommands() {
    m_queuedPlayerCommands.clear();
}

int GatoBot::getCurrentStepIdx() {
    return m_currentStep;
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

void GatoBot::updateDelta(float& dt) {
    if(m_status == BotStatus::Idle) return;

    switch(m_status) {
        case BotStatus::Recording:
        case BotStatus::Replaying: {
            dt = CCDirector::get()->getAnimationInterval() * m_mainSpeed;
        } break;

        case BotStatus::Rendering: {
            dt = m_renderParams.m_spt;
        } break;

        default: break;
    }
}

void GatoBot::updateBot() {
    if(m_status == BotStatus::Idle) return;

    this->updateCommon();

    switch(m_status) {
        case BotStatus::Recording: {
            this->updateRecording();
        } break;

        case BotStatus::Replaying: {
            this->updateReplaying();
        } break;

        case BotStatus::Rendering: {
            this->updateRendering();
        } break;

        default: break;
    }
}

void GatoBot::updateCommon() {
    if(m_status != BotStatus::Recording) {
        // out of steps
        if(m_currentStep >= m_loadedMacro.getStepCount()) {
            (void)this->changeStatus(BotStatus::Idle);
            return;
        }
    }
}

void GatoBot::levelEntered(PlayLayer* pLayer) {
    log::debug("GatoBot::levelEntered");

    // this function had a purpose once
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
        m_currentStep = 0;

        if(m_status == BotStatus::Recording) {
            m_loadedMacro.clearStepsFrom(m_currentStep);
        }
    }
}

void GatoBot::loadStepState(const StepState& state, bool clearStepsFromMacro) {
    log::debug("GatoBot::loadFrameState: {}", state.m_step);

    auto pLayer = this->getPlayLayer();

    m_currentStep = state.m_step;

    // restore player velocity
    pLayer->m_player1->m_yVelocity = state.m_player1.m_yVel;
    pLayer->m_player2->m_yVelocity = state.m_player2.m_yVel;

    // clear steps after current StepState
    if(clearStepsFromMacro) {
        m_loadedMacro.clearStepsFrom(m_currentStep);
    }
}

void GatoBot::botFinished(BotStatus oldStatus) {
    log::debug("GatoBot::botFinished");

    if(oldStatus == BotStatus::Recording) {
        m_loadedMacro.recordingFinished();
    }

    if(oldStatus == BotStatus::Rendering) {
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