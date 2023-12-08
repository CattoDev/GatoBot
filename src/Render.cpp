#include "GatoBot.hpp"
#include "LoadingCircle.hpp"

#include "FFmpeg.hpp"
#include <ShlObj.h>
#include <fmod/fmod.hpp>

#ifndef GB_DEBUG
#define logRender(renderLogs) if(GatoBot::sharedState()->settings.getOption<bool>("showConsoleWindow")) std::cout << renderLogs << "\n" 
#else
#define logRender(renderLogs) std::cout << renderLogs << "\n"
#endif

std::mutex RenderThreadLock;

bool GatoBot::updateRendering(float& dt) {
    updateExitText();

    RenderThreadLock.lock();
    bool hasData = currentFrameHasData;
    RenderThreadLock.unlock();

    if(hasData)
        return false;

    if(!inPlayLayer)
        return true;

    auto pLayer = gd::PlayLayer::get();

    // lock delta
    dt = settings.getOption<float>("targetSPF");

    updateReplaying();

    int frameFactor = settings.getOption<int>("frameFactor");
    if((currentFrame % frameFactor == 0 || currentFrame == 0)
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
    // (stolen from CCRenderTexture lmao)
    // set viewport of custom res
    glViewport(0, 0, frameW, frameH);

    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &m_pOldFBO);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_pFBO);
        
    visitPlayLayer(); // draw PlayLayer

    // read pixels
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

FFmpegSettings GatoBot::setupFFmpegSettings() {
    FFmpegSettings cmd;

    auto dir = CCDirector::sharedDirector();
    auto pLayer = gd::GameManager::sharedState()->getPlayLayer();
    auto levelData = pLayer->m_level; 

    cmd.frameWidth = settings.getOption<int>("targetWidth");
    cmd.frameHeight = settings.getOption<int>("targetHeight");

    cmd.fps = settings.getOption<int>("targetFPS");
    cmd.bitrate = settings.getOption<int>("vBitrate");
    cmd.audioBitrate = settings.getOption<int>("aBitrate");
    cmd.codec = settings.getOption<std::string>("codec");
    cmd.extraArgs = settings.getOption<std::string>("extraArgs");

    // song
    std::string songPath;
    if(settings.getOption<bool>("includeLevelSong")) {
        songPath = levelData->getAudioFileName();

        if(songPath.find("\\") == songPath.npos) {
            songPath = CCFileUtils::sharedFileUtils()->fullPathForFilename(songPath.c_str(), true);
        }
    }

    // output path
    if(settings.getOption<bool>("includeLevelSong") && !songPath.empty() && std::filesystem::exists(songPath)) {
        cmd.tempPath = std::filesystem::temp_directory_path().string() + std::string("gatobottemp.mp4");
        cmd.path = settings.getOption<std::string>("outputPath");

        cmd.songPath = songPath;
        cmd.songOffset = &currentMusicOffset;
        cmd.time = &timeFromStart;
    }
    else {
        cmd.tempPath = settings.getOption<std::string>("outputPath");
    }

    return cmd;
}

void GatoBot::renderingLoop() {
    bool endLoopForced = false;

    logRender("Starting rendering loop");

    auto renderSettings = setupFFmpegSettings();
    logRender(renderSettings.getCommandStr());

    // create ffmpeg process
    DWORD creationFlag = settings.getOption<bool>("showConsoleWindow") ? 0 : CREATE_NO_WINDOW;
    auto process = subprocess::Popen(renderSettings.getCommandStr(), creationFlag);

    // rendering loop
    RenderThreadLock.lock();
    while(status == Rendering || currentFrameHasData) {
        // check for errors
        if (process.getExitCode() == 1) {
            lastInfoCode = 1;
            changeStatus(Disabled);

            endLoopForced = true;
            RenderThreadLock.unlock();
            break;
        }

        if(currentFrameHasData) {
            const auto data = currentFrameData;
            currentFrameData.clear();
            currentFrameHasData = false;

            RenderThreadLock.unlock();

            // send data to ffmpeg
            process.m_stdin.write(data.data(), data.size());
        }
        
        RenderThreadLock.unlock();
    }

    if (!endLoopForced) {
        updateStatusLabel();

        // loading circle
        lastInfoCode = 2;
        checkErrors();

        GBLoadingCircle::setCaption("Rendering video...");

        // stop rendering video
        if(process.close()) {
            logRender("ffmpeg error :sob:");
            lastInfoCode = 1;
            checkErrors();

            return;
        }

        // add music
        this->addMusic(renderSettings);
    }

    logRender("Rendering video finished with code " << endLoopForced);
}

void GatoBot::addMusic(FFmpegSettings& renderSettings) {
    DWORD creationFlag = settings.getOption<bool>("showConsoleWindow") ? 0 : CREATE_NO_WINDOW;

    if(!renderSettings.songPath.empty() && std::filesystem::exists(renderSettings.songPath)) {
        auto songCmd = renderSettings.getSongCmdStr();
        logRender(songCmd);

        int retCode = subprocess::Popen(songCmd, creationFlag).close();

        std::filesystem::remove(renderSettings.tempPath); // delete temp

        if(retCode) {
            logRender("Adding audio failed");
            lastInfoCode = 1;
            return;
        }
    }

    lastInfoCode = 3;
    checkErrors();

    // open folder
    auto path = ILCreateFromPath(settings.getOption<std::string>("outputPath").c_str());
    if(path) {
        SHOpenFolderAndSelectItems(path, 0, 0, 0);
        ILFree(path);
    }
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
        clickSounds.clear();

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

        currentFrameData.resize(targetWidth * targetHeight * 4);

        glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &m_pOldRBO);
        glGenFramebuffersEXT(1, &m_pFBO);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_pFBO);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, renderingTexture->getName(), NULL);

        renderingTexture->setAliasTexParameters();
        renderingTexture->autorelease();
        
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_pOldFBO);
        glBindFramebufferEXT(GL_RENDERBUFFER_EXT, m_pOldRBO);

        totalRenderTimeStart = std::clock();

        botStatusChanged();

        //settings.setOption("includeLevelClicks", true); // temp
        std::thread(&GatoBot::renderingLoop, this).detach();
    }
    else {
        if(isDelayedRendering())
            return;

        if(renderingTexture != nullptr) {
            renderingTexture->release();
            renderingTexture = nullptr;
        }

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

void GatoBot::processClickSound(ButtonType button) {
    if(status == Rendering) {
        clickSounds.push_back({ timeFromStart, button });
    }
}