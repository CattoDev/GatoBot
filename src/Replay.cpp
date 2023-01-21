#include "GatoBot.hpp"

#include <nfd.h>

void GatoBot::toggleReplay(float newSPF, float speed) {
    if(status == Replaying) {
        status = Disabled;

        // reset fps
        auto dir = CCDirector::sharedDirector();
        dir->setAnimationInterval(lastSPF);
        dir->getScheduler()->setTimeScale(1);
        setSongPitch(1);
    }
    else {
        if(newSPF > 0 && speed > 0) {
            auto dir = CCDirector::sharedDirector();

            lastSPF = dir->getAnimationInterval();

            settings.targetSPF = newSPF;

            // Speedhack (Classic Mode)
            dir->setAnimationInterval(newSPF);
            dir->getScheduler()->setTimeScale(speed);
        }

        status = Replaying;
    }

    updateStatusLabel();
}

// for sorting
bool compareFramesForPrac(const LevelFrameData& a, const LevelFrameData& b) {
    return a.frame < b.frame;
}

// split
std::vector<std::string> _splitString(std::string stringData, char* delimiter) {
    size_t index = 0;
    size_t nextIndex = stringData.find(delimiter);
    std::string subStr;

    std::vector<std::string> dataVec;

    while(true) {
        subStr = stringData.substr(index, nextIndex - index);

        dataVec.push_back(subStr);

        if(nextIndex == stringData.npos) break;

        // continue
        index = nextIndex + 1;
        nextIndex = stringData.find(delimiter, index);
    }

    return dataVec;
}

void GatoBot::loadNewReplay() {
    // get file path
    nfdchar_t *outPath = NULL;
    nfdresult_t res = NFD_OpenDialog({"gatobot"}, nullptr, &outPath);

    if(res != NFD_OKAY) {
        free(outPath);
        return;
    } 

    // get data from file
    auto utils = CCFileUtils::sharedFileUtils();
    unsigned long dataSize;
    auto data = utils->getFileData(outPath, "r", &dataSize);

    free(outPath);

    if(dataSize > 0) {
        loadReplay(std::string((char*)data));
        auto alert = gd::FLAlertLayer::create(nullptr, "Success", "OK", nullptr, "Replay loaded!");
        alert->m_pTargetLayer = (CCNode*)botMenu;
        alert->show();
    }
    else {
        // error
        auto alert = gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "Failed to load replay!");
        alert->m_pTargetLayer = (CCNode*)botMenu;
        alert->show();
    }
}

void GatoBot::loadReplay(std::string replayDataCompressed) {
    // remove currently loaded frames
    levelFrames.clear();

    // decompress string
    auto replayData = std::string(ZipUtils_decompressString(replayDataCompressed, false, 11).sv());

    // parse replay data
    size_t index = 0;
    size_t nextIndex = replayData.find(";");
    std::string subStr;

    while(true) {
        if(nextIndex >= replayData.length()) break;

        subStr = replayData.substr(index, nextIndex - index);

        // add frame
        auto frame = frameFromString(subStr);
        levelFrames.push_back(frame);

        if(nextIndex == replayData.npos) break;

        // continue
        index = nextIndex + 1;
        nextIndex = replayData.find(";", index);
    }

    // sort clicks if some shit went wrong and they got shuffled
    std::sort(levelFrames.begin(), levelFrames.end(), compareFramesForPrac);
}

LevelFrameData GatoBot::frameFromString(std::string frameData) {
    LevelFrameData frame;

    auto frameDataVec = _splitString(frameData, "_");

    frame.frame = std::stoi(frameDataVec[0]);
    
    // parse players
    auto playerDataVec = _splitString(frameDataVec[1], "~");

    bool player2 = false;
    for(auto playerStr : playerDataVec) {
        PlayerData pData;

        // parse player data
        auto pDataVec = _splitString(playerStr, ",");

        int btnAct = std::stoi(pDataVec[0]);

        if(btnAct == 0) pData.action = ButtonType::None;
        if(btnAct == 1) pData.action = ButtonType::Pressed;
        if(btnAct == 2) pData.action = ButtonType::Released;

        pData.position = CCPoint(
            std::stof(pDataVec[1]),
            std::stof(pDataVec[2])
        );
        pData.yVel = std::stof(pDataVec[3]);

        // set data
        if(!player2) frame.player1 = pData;
        else frame.player2 = pData;

        player2 = true;
    }

    return frame;
}

void PlayerData::applyToPlayer(gd::PlayerObject* player) {
    MBO(CCPoint, player, 0x67C) = position;
    MBO(double, player, 0x628) = yVel;
}