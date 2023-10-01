#include "GatoBot.hpp"
#include "LoadingCircle.hpp"

#include "FFmpeg.hpp"
#include <ShlObj.h>
#include <fmod/fmod.hpp>

#define logRender(renderLogs) if(GatoBot::sharedState()->settings.getOption<bool>("showConsoleWindow")) std::cout << renderLogs << "\n"

std::mutex RenderThreadLock;

bool GatoBot::updateRendering(float& dt) {
    updateExitText();

    RenderThreadLock.lock();
    bool hasData = currentFrameHasData;
    RenderThreadLock.unlock();

    // wait for next frame
    if(hasData)
        return false;

    if(!inPlayLayer)
        return true;

    auto pLayer = gd::PlayLayer::get();

    // lock delta
    dt = settings.getOption<float>("targetSPF");

    updateReplaying();

    int frameFactor = settings.getOption<int>("frameFactor");
    if(
        (currentFrame % frameFactor == 0 || currentFrame == 0)
     && !MBO(bool, pLayer, 0x39C)
     && MBO(bool, pLayer, 0x2EC)
    ) {
        auto fmod = gd::FMODAudioEngine::sharedEngine();

        // set music pos (for pulses)
        if(fmod->isBackgroundMusicPlaying() && !pLayer->m_hasCompletedLevel) {
            int musicTime = static_cast<int>((getTimeForXPos(pLayer) + MBO(float, pLayer->m_pLevelSettings, 0xFC)) * 1000);
            
            fmod->m_pGlobalChannel->setPosition(musicTime, FMOD_TIMEUNIT_MS);
        }

        // render frame
        renderFrame();

        // video time
        timeFromStart += (1.f / static_cast<float>(settings.getOption<int>("targetFPS")));
    }

    if(isDelayedRendering())
        currentFrame++;

    return true;
}

void GatoBot::renderFrame() {
    auto dir = CCDirector::sharedDirector();
    auto texSize = dir->getOpenGLView()->getFrameSize();

    int frameW = settings.getOption<int>("targetWidth");
    int frameH = settings.getOption<int>("targetHeight");

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
    RenderThreadLock.lock();
    currentFrameData = frameData;
    currentFrameHasData = true;
    RenderThreadLock.unlock();
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

        if(!strcmp(typeid(*child).name() + 6, "cocos2d::CCDrawNode") && !bot->settings.getOption<bool>("MHCaptureDrawNodes"))
            continue;

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
        if(child->getZOrder() == 99 && !settings.getOption<bool>("MHCaptureLabels")) {
            continue;
        }

        // Object Layer
        if(pLayer->m_pObjectLayer->m_uID == child->m_uID) {
            drawObjectLayer(pLayer->m_pObjectLayer);
            continue;
        }

        // % label
        if(pLayer->m_percentLabel->m_uID == child->m_uID && !settings.getOption<bool>("capturePercentage")) {
            continue;
        }

        // render UILayer
        if(pLayer->m_uiLayer->m_uID == child->m_uID) {
            for(size_t c = 0; c < pLayer->m_uiLayer->getChildrenCount(); c++) {
                auto c2 = (CCNode*)pLayer->m_uiLayer->getChildren()->objectAtIndex(c);

                // status label
                if(statusLabel != nullptr && c2->m_uID == statusLabel->m_uID) {
                    continue;
                }

                // exit label
                if(exitLabel != nullptr && c2->m_uID == exitLabel->m_uID) {
                    continue;
                }
                
                c2->visit();
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

void renderingThread() {
    auto bot = GatoBot::sharedState();
    auto dir = CCDirector::sharedDirector();

    int frameW = bot->settings.getOption<int>("targetWidth");
    int frameH = bot->settings.getOption<int>("targetHeight");

    auto pLayer = gd::GameManager::sharedState()->getPlayLayer();
    auto levelData = pLayer->m_level;    

    auto levelSettings = pLayer->m_pLevelSettings;

    FFmpegSettings cmd;

    cmd.frameWidth = frameW;
    cmd.frameHeight = frameH;
    cmd.fps = bot->settings.getOption<int>("targetFPS");

    std::string songPath;
    if(bot->settings.getOption<bool>("includeLevelSong")) {
        songPath = levelData->getAudioFileName();

        if(songPath.find("\\") == songPath.npos) {
            songPath = CCFileUtils::sharedFileUtils()->fullPathForFilename(songPath.c_str(), true);
        }
    }

    if(bot->settings.getOption<bool>("includeLevelSong") && !songPath.empty() && std::filesystem::exists(songPath)) {
        cmd.tempPath = std::filesystem::temp_directory_path().string() + std::string("gatobottemp.mp4");
        cmd.path = bot->settings.getOption<std::string>("outputPath");
    }
    else {
        cmd.tempPath = bot->settings.getOption<std::string>("outputPath");
    }

    cmd.bitrate = bot->settings.getOption<int>("vBitrate");
    cmd.audioBitrate = bot->settings.getOption<int>("aBitrate");
    cmd.codec = bot->settings.getOption<std::string>("codec");
    cmd.extraArgs = bot->settings.getOption<std::string>("extraArgs");

    logRender(cmd.getCommandStr());
    DWORD creationFlag = bot->settings.getOption<bool>("showConsoleWindow") ? 0 : CREATE_NO_WINDOW;
    auto process = subprocess::Popen(cmd.getCommandStr(), creationFlag);

    // rendering loop
    while(bot->status == Rendering || bot->currentFrameHasData) {
        // check if error occured
        if(process.getExitCode() == 1) {
            bot->lastInfoCode = 1;
            bot->changeStatus(Disabled);
                
            break;
        }

        RenderThreadLock.lock();
        if(bot->currentFrameHasData) {
            const auto frameData = bot->currentFrameData;
            bot->currentFrameData.clear();
            bot->currentFrameHasData = false;

            // write frame data to ffmpeg
            process.m_stdin.write(frameData.data(), frameData.size());
        } 

        RenderThreadLock.unlock();
    }

    RenderThreadLock.lock();
    bot->updateStatusLabel();
    bot->lastInfoCode = 2;
    RenderThreadLock.unlock();

    int errCode = process.close();
    if(errCode) {
        logRender("ffmpeg error :sob:");
        bot->lastInfoCode = 1;
        return;
    }

    if(bot->settings.getOption<bool>("includeLevelSong") && !songPath.empty() && std::filesystem::exists(songPath)) {
        cmd.songPath = songPath;
        cmd.songOffset = bot->currentMusicOffset;
        cmd.time = bot->timeFromStart;

        auto songCmd = cmd.getSongCmdStr();

        logRender(songCmd);

        int retCode = subprocess::Popen(songCmd, creationFlag).close();

        std::filesystem::remove(cmd.tempPath); // delete temp

        if (retCode) {
            logRender("Adding audio failed");
            return;
        }
    }

    logRender("Rendering video finished");

    // open folder
    auto path = ILCreateFromPath(bot->settings.getOption<std::string>("outputPath").c_str());
    if(path) {
        SHOpenFolderAndSelectItems(path, 0, 0, 0);
        ILFree(path);
    }

    // remove circle
    RenderThreadLock.lock();
    bot->lastInfoCode = 3;
    bot->status = Disabled;
    RenderThreadLock.unlock();

    logRender("Rendering thread finished");
}

void GatoBot::toggleRendering(bool toggled) {
    if(toggled) {
        if(!levelFrames.size()) {
            gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "<cr>There are no loaded frames to render!</c>")->show();
            return;
        }

        int currentFPS = getCurrentFPS();

        // lower framerate
        int targetFPS = settings.getOption<int>("targetFPS");
        if(currentFPS > targetFPS) {
            if(currentFPS % targetFPS == 0) {
                settings.setOption("frameFactor", currentFPS / targetFPS);
            }
            else {
                // not divisible
                logRender("Division error");
                return;
            }
        }
        else if(currentFPS == targetFPS) settings.setOption("frameFactor", 1);
        else {
            logRender("Division error");
            return;
        }

        settings.setOption("targetSPF", 1.f / static_cast<float>(currentFPS));
        settings.setOption("targetSpeed", 1.f);

        logRender("Frame factor: " << settings.getOption<int>("frameFactor"));

        status = Rendering;
        currentFrame = 0;
        currentFrameData.clear();
        currentFrameHasData = false;

        // make resolution even if it somehow isn't
        int targetWidth = settings.getOption<int>("targetWidth");
        int targetHeight = settings.getOption<int>("targetHeight");

        if(targetWidth % 2 != 0) targetWidth--;
        if(targetHeight % 2 != 0) targetHeight--;

        settings.setOption("targetWidth", targetWidth);
        settings.setOption("targetHeight", targetHeight);
 
        logRender("Starting render with resolution " << targetWidth << "x" << targetHeight);

        // render at higher resolution
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &m_pOldFBO);

        auto data = malloc(targetWidth * targetHeight * 4);
        memset(data, 0, targetWidth * targetHeight * 4);

        renderingTexture = new CCTexture2D();
        renderingTexture->initWithData(data, kCCTexture2DPixelFormat_RGB888, targetWidth, targetHeight, CCSize(static_cast<float>(targetWidth), static_cast<float>(targetHeight)));

        free(data);

        glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &m_pOldRBO);
        glGenFramebuffersEXT(1, &m_pFBO);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_pFBO);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, renderingTexture->getName(), NULL);

        renderingTexture->setAliasTexParameters();
        renderingTexture->autorelease();
        
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_pOldFBO);
        glBindFramebufferEXT(GL_RENDERBUFFER_EXT, m_pOldRBO);

        toggleGameFPSCap(false);
        totalRenderTimeStart = std::clock();

        botStatusChanged();

        // start rendering thread
        std::thread(renderingThread).detach();
    }
    else {
        if(isDelayedRendering())
            return;

        if(renderingTexture != nullptr) {
            renderingTexture->release();
            renderingTexture = nullptr;
        }

        toggleGameFPSCap(true);
        CCDirector::sharedDirector()->setAnimationInterval(lastSPF);

        pauseLevel();
        exitLabel->setOpacity(0);

        status = Disabled;
    }
}

bool GatoBot::isDelayedRendering() {
    if(status != Rendering)
        return false;

    if(endDelayStart == 0)
        return false;

    return timeFromStart - endDelayStart <= settings.getOption<float>("renderDelay");
}