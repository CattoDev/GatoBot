#include "GatoBot.hpp"

#include <fstream>
#include <sstream>
#include <nfd.h>

void GatoBot::toggleRecord(float newSPF, float speed) {
    if(status == Recording) {
        status = Disabled;

        // reset fps
        auto dir = CCDirector::sharedDirector();
        dir->setAnimationInterval(lastSPF);
        dir->getScheduler()->setTimeScale(1);
        setSongPitch(1);
    }
    else {
        levelFrames.clear();

        if(newSPF > 0 && speed > 0) {
            auto dir = CCDirector::sharedDirector();

            lastSPF = dir->getAnimationInterval();

            settings.targetSPF = newSPF;
            settings.targetSpeed = speed;

            // Speedhack (Classic Mode)
            dir->setAnimationInterval(newSPF);
            dir->getScheduler()->setTimeScale(speed);
        }

        status = Recording;
    }

    updateStatusLabel();
}

void GatoBot::handleFrame(gd::PlayLayer* pLayer) {
    auto player1 = PlayerData::fromPlayer(pLayer->m_pPlayer1);
    auto player2 = PlayerData::fromPlayer(pLayer->m_pPlayer2);

    player1.action = queuedBtnP1;
    player2.action = queuedBtnP2;

    queuedBtnP1 = None;
    queuedBtnP2 = None;

    LevelFrameData frame = {currentFrame, player1, player2};
    levelFrames.push_back(frame);
}

void GatoBot::handleCheckpoint(gd::PlayLayer* pLayer) {
    if(status == Recording && !MBO(bool, pLayer, 0x39C)) {
        auto player1 = PlayerData::fromPlayer(pLayer->m_pPlayer1); 
        auto player2 = PlayerData::fromPlayer(pLayer->m_pPlayer2); 

        auto obj = (gd::CheckpointObject*)pLayer->m_checkpoints->lastObject();

        LevelFrameData frame = {currentFrame, player1, player2};
        CheckpointData checkpoint = {obj, frame};

        practiceCheckpoints.push_back(checkpoint);
    }
}

void GatoBot::saveCurrentReplay() {
    nfdchar_t filterList[] = {"gatobot"};

    nfdchar_t *outPath = NULL;
    nfdresult_t res = NFD_SaveDialog(filterList, nullptr, &outPath);

    if(res == NFD_OKAY) {
        auto filePath = std::string(outPath);
        free(outPath);

        saveReplay(filePath);

        auto alert = gd::FLAlertLayer::create(nullptr, "Success", "OK", nullptr, 400, CCString::createWithFormat("Replay saved to: <cg>%s</c>", filePath)->m_sString);
        alert->m_pTargetLayer = (CCNode*)botMenu;
        alert->show();
    }
}

// saving system
void GatoBot::saveReplay(std::string& filePath) {
    // add extention to path if missing
    if(strstr(filePath.c_str(), ".gatobot") == nullptr) {
        filePath.append(".gatobot");
    }

    std::ofstream saveFile(filePath);

    // convert data to string
    std::stringstream saveData;
    for(auto frame : levelFrames) {
        // frame count
        saveData << frame.frame;
        saveData << "_";
        
        // player 1 data
        if(frame.player1.action == ButtonType::None) saveData << 0;
        if(frame.player1.action == ButtonType::Pressed) saveData << 1;
        if(frame.player1.action == ButtonType::Released) saveData << 2;

        saveData << ",";
        saveData << frame.player1.position.x << std::setprecision(8);
        saveData << ",";
        saveData << frame.player1.position.y << std::setprecision(8);
        saveData << ",";
        saveData << frame.player1.yVel << std::setprecision(8);
        saveData << "~";

        // player 2 data
        if(frame.player2.action == ButtonType::None) saveData << 0;
        if(frame.player2.action == ButtonType::Pressed) saveData << 1;
        if(frame.player2.action == ButtonType::Released) saveData << 2;

        saveData << ",";
        saveData << frame.player2.position.x << std::setprecision(8);
        saveData << ",";
        saveData << frame.player2.position.y << std::setprecision(8);
        saveData << ",";
        saveData << frame.player2.yVel << std::setprecision(8);
        
        saveData << ";";
    }

    // compress!!!!!!!
    auto compressed = ZipUtils_compressString(gdstring(saveData.str()), false, 11);

    // write to file
    saveFile << std::string(compressed.sv());

    // done
    saveFile.close();
}

void GatoBot::handleClick(gd::GJBaseGameLayer* self, bool rightSide, ButtonType btnType) {
    bool twoPlayer = MBO(bool, self->m_pLevelSettings, 0xFA);

    bool playerIsDead = MBO(bool, self, 0x39C);

    if(status == Recording && !playerIsDead) {
        if(MBO(bool, self, 0x2A9) && !rightSide && twoPlayer)
            queuedBtnP2 = btnType;

        else queuedBtnP1 = btnType;
    }
}