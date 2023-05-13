#include "GatoBot.hpp"
#include "LoadingCircle.hpp"

#include "FFmpeg.hpp"
#include <ShlObj.h>

#define logRender(renderLogs) if(GatoBot::sharedState()->settings.showConsoleWindow) std::cout << renderLogs

void GatoBot::updateRender() {
    /*
        capture frame data
    */
    auto dir = CCDirector::sharedDirector();
    auto texSize = dir->getOpenGLView()->getFrameSize();

    int frameW = settings.targetWidth;
    int frameH = settings.targetHeight;

    std::vector<GLubyte> frameData;
    frameData.resize(frameW * frameH * 4);

    /* 
        Get frame data 
    */
    // stolen from CCRenderTexture lmao 
    glViewport(0, 0, frameW, frameH);

    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &m_pOldFBO);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_pFBO);
        
    visitPlayLayer(); // draw PlayLayer

    // pixels
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, frameW, frameH, GL_RGBA, GL_UNSIGNED_BYTE, frameData.data());

    // reset viewport
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_pOldFBO);
    dir->setViewport();

    // set data
    threadLock.lock();
    currentFrameData = frameData;
    currentFrameHasData = true;
    threadLock.unlock();

    updateStatusLabel();
}

void drawObjectLayer(CCLayer* m_pObjectLayer) {
    auto bot = GatoBot::sharedState();

    // Object Layer
    /////////////////////////////
    kmGLPushMatrix();

    if(m_pObjectLayer->getGrid() != nullptr) {
        if(m_pObjectLayer->getGrid()->isActive())
            m_pObjectLayer->getGrid()->beforeDraw();
    }

    m_pObjectLayer->transform();
    m_pObjectLayer->sortAllChildren();

    bool drawnObjectLayer = false;
    for(size_t i = 0; i < m_pObjectLayer->getChildrenCount(); i++) {
        auto child = (CCNode*)m_pObjectLayer->getChildren()->objectAtIndex(i);

        if(!strcmp(typeid(*child).name() + 6, "cocos2d::CCDrawNode") && !bot->settings.captureMegaHackDrawNodes) {
            continue;
        }

        if(child->getZOrder() < 0) {
            child->visit();
        }
        else {
            if(!drawnObjectLayer) {
                m_pObjectLayer->draw();
                drawnObjectLayer = true;
            }

            child->visit();
        }
    }

    m_pObjectLayer->setOrderOfArrival(0);

    if(m_pObjectLayer->getGrid() != nullptr) {
        if(m_pObjectLayer->getGrid()->isActive())
            m_pObjectLayer->getGrid()->afterDraw(m_pObjectLayer);
    }

    kmGLPopMatrix();
    /////////////////////////////
}

void GatoBot::visitPlayLayer() {
    auto pLayer = gd::GameManager::sharedState()->getPlayLayer();

    /* 
        only draw wanted elements
    */
    kmGLPushMatrix();

    if(pLayer->getGrid() != nullptr) {
        if(pLayer->getGrid()->isActive())
            pLayer->getGrid()->beforeDraw();
    }

    pLayer->transform();
    pLayer->sortAllChildren();

    // PlayLayer
    bool drawnPlayLayer = false;
    for(size_t i = 0; i < pLayer->getChildrenCount(); i++) {
        auto child = (CCNode*)pLayer->getChildren()->objectAtIndex(i);

        // Mega Hack labels
        if(child->getZOrder() == 99 && !settings.captureMegaHackLabels) {
            continue;
        }

        // Object Layer
        if(pLayer->m_pObjectLayer->m_uID == child->m_uID) {
            drawObjectLayer(pLayer->m_pObjectLayer);
            continue;
        }

        // % label
        if(pLayer->m_percentLabel->m_uID == child->m_uID && !settings.capturePercentage) {
            continue;
        }

        // bot status label
        if(pLayer->m_uiLayer->m_uID == child->m_uID && statusLabel != nullptr) {
            for(size_t c = 0; c < pLayer->m_uiLayer->getChildrenCount(); c++) {
                auto c2 = (CCNode*)pLayer->m_uiLayer->getChildren()->objectAtIndex(c);

                if(c2->m_uID == statusLabel->m_uID) {
                    continue;
                }
                else c2->visit();
            }   
        }
        else {
            if(child->getZOrder() < 0) {
                child->visit();
            }
            else {
                if(!drawnPlayLayer) {
                    pLayer->draw();
                    drawnPlayLayer = true;
                }

                child->visit();
            }
        }
    }

    pLayer->setOrderOfArrival(0);

    if(pLayer->getGrid() != nullptr) {
        if(pLayer->getGrid()->isActive())
            pLayer->getGrid()->afterDraw(pLayer);
    }
    
    kmGLPopMatrix();
}

// fun stuff
void renderingThread(GatoBot* bot) {
    auto dir = CCDirector::sharedDirector();

    int frameW = bot->settings.targetWidth;
    int frameH = bot->settings.targetHeight;

    auto pLayer = gd::GameManager::sharedState()->getPlayLayer();
    auto levelData = pLayer->m_level;    

    auto levelSettings = pLayer->m_pLevelSettings;

    FFmpegSettings cmd;

    cmd.frameWidth = frameW;
    cmd.frameHeight = frameH;
    cmd.fps = bot->settings.targetFPS;

    if(bot->settings.includeLevelSong) {
        cmd.tempPath = std::filesystem::temp_directory_path().string() + std::string("gatobottemp.mp4");
        cmd.path = bot->settings.videoPath;
    }
    else {
        cmd.tempPath = bot->settings.videoPath;
    }

    cmd.bitrate = bot->settings.bitrate;
    cmd.audioBitrate = bot->settings.audioBitrate;
    cmd.codec = bot->settings.codec;
    cmd.extraArgs = bot->settings.extraArguments;

    logRender(cmd.getCommandStr());
    DWORD creationFlag = bot->settings.showConsoleWindow ? 0 : CREATE_NO_WINDOW;
    auto process = subprocess::Popen(cmd.getCommandStr(), creationFlag);

    // rendering loop
    while(bot->status == Rendering || bot->currentFrameHasData) {
        // check if error occured
        if(process.getExitCode() == 1) {
            bot->lastInfoCode = 1;
            bot->toggleRender();
                
            break;
        }

        bot->threadLock.lock();
        if(bot->currentFrameHasData) {
            const auto frameData = bot->currentFrameData;
            bot->currentFrameData.clear();
            bot->currentFrameHasData = false;

            // write frame data to ffmpeg
            process.m_stdin.write(frameData.data(), frameData.size());
        } 

        bot->threadLock.unlock();
    }

    bot->updateStatusLabel();

    bot->threadLock.lock();
    bot->lastInfoCode = 2;
    bot->threadLock.unlock();

    int errCode = process.close();
    if(errCode) {
        logRender("ffmpeg error :sob:" << "\n");
        bot->lastInfoCode = 1;
        return;
    }

    if(bot->settings.includeLevelSong) {
        auto songPath = levelData->getAudioFileName();

        if(songPath.find("\\") == songPath.npos) {
            songPath = CCFileUtils::sharedFileUtils()->fullPathForFilename(songPath.c_str(), true);
        }

        if (!songPath.empty() && std::filesystem::exists(songPath)) {
            cmd.songPath = songPath;
            cmd.songOffset = bot->currentMusicOffset;
            cmd.time = bot->timeFromStart;

            auto songCmd = cmd.getSongCmdStr();

            logRender(songCmd);

            int retCode = subprocess::Popen(songCmd, creationFlag).close();

            std::filesystem::remove(cmd.tempPath); // delete temp

            if (retCode) {
                logRender("Adding audio failed" << "\n");
                return;
            }
        }
    }

    GBLoadingCircle::remove();

    logRender("Rendering video finished" << "\n");

    // open folder
    auto path = ILCreateFromPath(bot->settings.videoPath.c_str());
    if(path) {
        SHOpenFolderAndSelectItems(path, 0, 0, 0);
        ILFree(path);
    }
}

void GatoBot::toggleRender() {
    if(status != Rendering) {
        if(levelFrames.size() == 0) {
            gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "<cr>There are no loaded frames to render!</c>")->show();
            return;
        }

        int currentFPS = getCurrentFPS();

        // lower framerate
        if(currentFPS > settings.targetFPS) {
            if(currentFPS % settings.targetFPS == 0) {
                settings.divideFramesBy = currentFPS / settings.targetFPS;
            }
            else {
                // not divisible error
                logRender("Division error" << "\n");
                return;
            }
        }
        else if(currentFPS == settings.targetFPS) settings.divideFramesBy = 1;
        else {
            logRender("Division error" << "\n");

            return;
        }

        logRender("Divide frames by: " << settings.divideFramesBy << "\n");

        status = Rendering;
        currentFrame = 0;
        currentFrameData.clear();
        currentFrameHasData = false;

        // make resolution even if it somehow isn't
        if(settings.targetWidth % 2 != 0) settings.targetWidth--;
        if(settings.targetHeight % 2 != 0) settings.targetHeight--;
 
        logRender("Starting render with resolution " << settings.targetWidth << "x" << settings.targetHeight << "\n");

        // render at higher resolution
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &m_pOldFBO);

        auto data = malloc(settings.targetWidth * settings.targetHeight * 4);
        memset(data, 0, settings.targetWidth * settings.targetHeight * 4);

        renderingTexture = new CCTexture2D();
        renderingTexture->initWithData(data, kCCTexture2DPixelFormat_RGB888, settings.targetWidth, settings.targetHeight, CCSize(static_cast<float>(settings.targetWidth), static_cast<float>(settings.targetHeight)));

        free(data);

        glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &m_pOldRBO);
        glGenFramebuffersEXT(1, &m_pFBO);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_pFBO);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, renderingTexture->getName(), NULL);

        renderingTexture->setAliasTexParameters();
        
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_pOldFBO);
        glBindFramebufferEXT(GL_RENDERBUFFER_EXT, m_pOldRBO);

        // disable speedhack anticheat
        patchMemory(reinterpret_cast<void*>(gd::base + 0x202ad0), {0x90, 0x90, 0x90, 0x90, 0x90});

        toggleGameFPSCap(false);
        toggleHook(SchedulerUpdate, true);

        // start rendering thread
        std::thread(renderingThread, this).detach();
    }
    else {
        status = Disabled;

        renderingTexture->release();
        
        toggleGameFPSCap(true);
        toggleHook(SchedulerUpdate, false);

        patchMemory(reinterpret_cast<void*>(gd::base + 0x202ad0), {0xE8, 0x3B, 0xAD, 0x00, 0x00});

        CCDirector::sharedDirector()->setAnimationInterval(lastSPF);

        reinterpret_cast<void(__thiscall*)(gd::PlayLayer*, bool)>(gd::base + 0x20d3c0)(gd::PlayLayer::get(), false);
    }

    updateStatusLabel();

    endDelayStart = 0;
}

void GatoBot::toggleRenderDelayed() {
    if(!settings.delayEnd) {
        toggleRender();
        return;
    }

    /*auto func = CCCallFunc::create(this, callfunc_selector(GatoBot::toggleRender));
    auto seq = CCSequence::create(CCDelayTime::create(time), func, nullptr);

    runAction(seq);*/

    if(endDelayStart == 0)
        endDelayStart = timeFromStart;
}