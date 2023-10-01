#include "GatoBot.hpp"

#include <fstream>
#include <sstream>
#include <nfd.h>

void GatoBot::updateRecording() {
    auto pLayer = gd::PlayLayer::get();

    if(pLayer != nullptr) {
        if(
            !MBO(bool, pLayer, 0x39C) // isDead?
        &&  MBO(bool, pLayer, 0x2EC)
        ) {
            handleFrame(pLayer);

            // increment
            currentFrame++;
        }
    }
}

// dumbass
void handlePlayerAction(PlayerData& player, ButtonType& btn) {
    if(player.isHolding) {
        player.action = Pressed;
        btn = Pressed;
    }
    else {
        player.action = Released;
        btn = Released;
    }
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

    // convert data to string
    std::stringstream saveData;
    for(auto frame : levelFrames) {
        // frame count
        saveData << frame.frame;
        saveData << "_";

        bool isP2 = false;

        while(true) {
            PlayerData& pData = isP2 ? frame.player2 : frame.player1;

            if(pData.action == ButtonType::None) saveData << 0;
            if(pData.action == ButtonType::Pressed) saveData << 1;
            if(pData.action == ButtonType::Released) saveData << 2;

            saveData << ",";
            saveData << pData.position.x << std::setprecision(8);
            saveData << ",";
            saveData << pData.position.y << std::setprecision(8);
            saveData << ",";
            saveData << pData.yVel << std::setprecision(8);
            saveData << ",";
            saveData << pData.rotation << std::setprecision(8);

            if(isP2) break;

            saveData << "~";
            isP2 = true;
        }

        saveData << ";";
    }

    // compress!!!!!!!
    auto compressed = ZipUtils_compressString(gdstring(saveData.str()), false, 11);

    // write to file
    std::ofstream saveFile(filePath);
    saveFile << std::string(compressed.sv());

    // done
    saveFile.close();
}

bool GatoBot::handleClick(gd::GJBaseGameLayer* self, bool rightSide, ButtonType btnType) {
    if(status != Recording) return true;
    
    bool twoPlayer = MBO(bool, self->m_pLevelSettings, 0xFA);

    bool playerIsDead = MBO(bool, self, 0x39C);

    if(status == Recording && !playerIsDead) {
        if(MBO(bool, self, 0x2A9) && !rightSide && twoPlayer)
            queuedBtnP2 = btnType;

        else queuedBtnP1 = btnType;
    }

    return true;
}

// unused
void GatoBot::organizeReplay() {
    // organize actions
    ButtonType lastBtnP1 = None;
    ButtonType lastBtnP2 = None;

    for(LevelFrameData& frame : levelFrames) {
        bool isP2 = false;

        while(true) {
            PlayerData &pData = isP2 ? frame.player2 : frame.player1;

            ButtonType& lastBtnVar = isP2 ? lastBtnP2 : lastBtnP1;

            if(pData.action != lastBtnVar) {
                lastBtnVar = pData.action;
            }
            else {
                pData.action = None;
            }

            if(isP2) break;
            else isP2 = true;
        }
    }
}