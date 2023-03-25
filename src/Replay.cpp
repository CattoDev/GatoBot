#include "GatoBot.hpp"

#include <nfd.h>

using namespace nlohmann; // json

// for sorting
bool compareFramesForPrac(const LevelFrameData& a, const LevelFrameData& b) {
    return a.frame < b.frame;
}

bool compareMHevents(const json& a, const json& b) {
    return a["frame"] < b["frame"];
}

// toggle
void GatoBot::toggleReplay(int FPS, float speed) {
    if(status == Replaying) {
        status = Disabled;

        // reset fps
        auto dir = CCDirector::sharedDirector();
        dir->setAnimationInterval(lastSPF);
        dir->getScheduler()->setTimeScale(1);
        setSongPitch(1);
    }
    else {
        if(FPS > 0 && speed > 0) {
            auto dir = CCDirector::sharedDirector();

            float newSPF = 1.f / (FPS * speed);
            lastSPF = dir->getAnimationInterval();

            settings.targetSPF = newSPF;
            settings.targetSpeed = speed;
            settings.targetFPS = FPS;

            dir->setAnimationInterval(newSPF);
            dir->getScheduler()->setTimeScale(speed);
        }

        status = Replaying;
    }

    updateStatusLabel();
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
    nfdresult_t res = NFD_OpenDialog({"gatobot,mhr.json"}, nullptr, &outPath);

    if(res != NFD_OKAY) {
        free(outPath);
        return;
    } 

    // use ifstream instead
    std::string data;
    std::ifstream file(outPath);
    std::string line;

    while(getline(file, line)) {
        data.append(line);
    }

    ReplayType rType = ReplayType::GatoBotR;

    // MH replay
    if(strstr(outPath, "mhr.json") != NULL) rType = ReplayType::MegaHack;

    free(outPath);

    if(data.length() > 0) {
        auto ret = loadReplay(data, rType);

        if(ret == Success) {
            auto alert = gd::FLAlertLayer::create(nullptr, "Success", "OK", nullptr, "Replay loaded!");
            alert->m_pTargetLayer = (CCNode*)botMenu;
            alert->show();
        }
        if(ret == MissingFrames) {
            auto alert = gd::FLAlertLayer::create(nullptr, "Warning", "OK", nullptr, 360, "This replay seems to have missing frames.\n<cy>Make sure the Mega Hack replay is recorded with the \"Frame Fixes\" accuracy.</c>");
            alert->m_pTargetLayer = (CCNode*)botMenu;
            alert->show();
        }
        if(ret == Failed) {
            // error
            auto alert = gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "Failed to load replay!");
            alert->m_pTargetLayer = (CCNode*)botMenu;
            alert->show();
        }
    }
    else {
        // error
        auto alert = gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "Failed to load replay!");
        alert->m_pTargetLayer = (CCNode*)botMenu;
        alert->show();
    }
}

ReplayLoadStatus GatoBot::loadReplay(std::string replayDataCompressed, ReplayType rType = ReplayType::GatoBotR) {
    ReplayLoadStatus retCode = Success;

    // remove currently loaded frames
    levelFrames.clear();

    // decompress string
    std::string replayData;
    if(rType == ReplayType::GatoBotR)
        replayData = std::string(ZipUtils_decompressString(replayDataCompressed, false, 11).sv());

    else replayData = replayDataCompressed;

    // parse replay data
    if(rType == ReplayType::GatoBotR) {
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
    }
    // parse MegaHack replay
    if(rType == ReplayType::MegaHack) {
        json data = json::parse(replayData);

        auto events = data["events"];

        // sort just in case
        std::sort(events.begin(), events.end(), compareMHevents);

        // allocate (ig?)
        levelFrames.resize(events.back()["frame"] + 1);

        // apply to frames
        for (json::iterator it = events.begin(); it != events.end(); it++) {
            const auto item = it.value();
            const int curFrame = item["frame"];

            if(!item.contains("p2")) {
                auto pData = PlayerData::fromJson(item);

                const auto oldData = levelFrames[curFrame].player1;

                if(oldData.action != None) 
                    pData.action = oldData.action;

                levelFrames[curFrame].player1 = pData;
            }
            else {
                auto pData = PlayerData::fromJson(item);

                const auto oldData = levelFrames[curFrame].player2;

                if(oldData.action != None) 
                    pData.action = oldData.action;

                levelFrames[curFrame].player2 = pData;
            }

            levelFrames[curFrame].frame = curFrame;
        }

        // warning if not frame fixes accuracy
        for(size_t i = 0; i < levelFrames.size(); i++) {
            if(levelFrames[i].frame != i) {
                retCode = MissingFrames;
                break;
            }
        }
    }

    // sort clicks if some shit went wrong and they got shuffled
    std::sort(levelFrames.begin(), levelFrames.end(), compareFramesForPrac);

    return retCode;
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

PlayerData PlayerData::fromJson(json jsonData) {
    PlayerData data;

    data.position = CCPoint(
        jsonData["x"], jsonData["y"]
    );

    data.yVel = jsonData["a"];

    if(jsonData.contains("down")) {
        data.action = jsonData["down"] ? Pressed : Released;
    }

    return data;
}