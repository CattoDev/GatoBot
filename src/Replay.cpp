#include "GatoBot.hpp"
#include "ConvertTools.hpp"

#include <nfd.h>
#include <iterator>

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
            targetFPS = FPS;

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
    nfdresult_t res = NFD_OpenDialog({"gatobot,mhr,mhr.json"}, nullptr, &outPath);

    if(res != NFD_OKAY) {
        free(outPath);
        return;
    } 

    // replay type
    #define SetReplayT(exte, enumt) if(strstr(outPath, exte) != NULL) rType = ReplayType::##enumt;

    ReplayType rType = ReplayType::GatoBotR;

    SetReplayT(".mhr", MegaHack);
    SetReplayT(".mhr.json", MegaHackJson);

    // read file
    std::ios_base::openmode openMode = std::ios_base::in;

    // MegaHack replays are binary
    if(rType == MegaHack) openMode = std::ios_base::binary;
        
    std::ifstream file(outPath, openMode);
    std::vector<char> data(std::istreambuf_iterator<char>(file), {});

    file.close();

    free(outPath);

    if(data.size() > 0) {
        auto ret = loadReplay(data, rType);

        if(ret == Success) {
            auto alert = gd::FLAlertLayer::create(nullptr, "Success", "OK", nullptr, "Replay loaded!");
            alert->m_pTargetLayer = (CCNode*)botMenu;
            alert->show();
        }
        if(ret == MissingFrames) {
            auto alert = gd::FLAlertLayer::create(nullptr, "Warning", "OK", nullptr, 400, "This replay seems to have missing frames.\n<cy>It is recommended that Mega Hack replays are recorded with the \"Frame Fixes\" accuracy.</c>\n<cg>The replay will work, but</c> <cr>MAY NOT</c> <cg>be as accurate!</c>");
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

ReplayLoadStatus GatoBot::loadReplay(std::vector<char>& replayDataVec, ReplayType rType = ReplayType::GatoBotR) {
    ReplayLoadStatus retCode = Success;

    // to str
    std::string replayDataCompressed = std::string(replayDataVec.begin(), replayDataVec.end());

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

    // parse MegaHack (binary) replay
    if(rType == ReplayType::MegaHack) {
        #define Chunk(n1, n2) n1, n1 + n2

        // basic
        int metaSize = replayDataVec[0x8];
        int eventSize = replayDataVec[0xC + metaSize + 0x8];
        int eventCount = GBConvertTools::MH_HexToInt(std::vector<unsigned char>(Chunk(replayDataVec.begin() + 0xC + metaSize + 0xC, 4)));

        // get last frame
        int lastFrame = GBConvertTools::MH_HexToInt(std::vector<unsigned char>(Chunk(replayDataVec.begin() + 0xC + metaSize + 0x10 + eventSize * (eventCount - 1) + 0x4, sizeof(int))));

        // allocate
        levelFrames.resize(lastFrame + 1);

        // events
        for(int i = 0; i < eventCount; i++) {
            std::vector<char> event(Chunk(replayDataVec.begin() + 0xC + metaSize + 0x10 + eventSize * i, eventSize));

            // frame
            int curFrame = GBConvertTools::MH_HexToInt(std::vector<unsigned char>(Chunk(event.begin() + 0x4, sizeof(int))));

            // other vars
            int eventType = (int)event[0];
            bool btnDown = (int)event[0x2] == 1;
            bool player2 = (int)event[0x3] == 1;

            // position
            CCPoint pos = {
                GBConvertTools::MH_HexToFloat(std::vector<char>(Chunk(event.begin() + 0x8, sizeof(float)))),
                GBConvertTools::MH_HexToFloat(std::vector<char>(Chunk(event.begin() + 0xC, sizeof(float))))
            };

            // rotation and acceleration
            float rotation = GBConvertTools::MH_HexToFloat(std::vector<char>(Chunk(event.begin() + 0x10, sizeof(float))));
            double acceleration = GBConvertTools::MH_HexToDouble(std::vector<char>(Chunk(event.begin() + 0x18, sizeof(double))));

            // convert to GatoBot replay
            PlayerData* pData = player2 ? &levelFrames[curFrame].player2 : &levelFrames[curFrame].player1;

            // button
            if(eventType == 2) // click
                pData->action = btnDown ? ButtonType::Pressed : ButtonType::Released;

            pData->position = pos;
            pData->yVel = acceleration;
            pData->rotation = rotation;
            pData->isSet = true;

            levelFrames[curFrame].frame = curFrame;
        }
    }

    // parse MegaHack json replay
    if(rType == ReplayType::MegaHackJson) {
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

            if(!item.contains("p2"))
                levelFrames[curFrame].player1 = PlayerData::fromJson(item, levelFrames[curFrame].player1);

            else
                levelFrames[curFrame].player2 = PlayerData::fromJson(item, levelFrames[curFrame].player2);

            levelFrames[curFrame].frame = curFrame;
        }
    }

    // missing frames
    if(hasMissingFrames()) retCode = MissingFrames;

    // sort frames if some shit went wrong and they got shuffled
    if(retCode != MissingFrames)
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

        // old macros don't store rotation
        if(pDataVec.size() > 4)
            pData.rotation = std::stof(pDataVec[4]);

        // set data
        if(!player2) frame.player1 = pData;
        else frame.player2 = pData;

        player2 = true;
    }

    return frame;
}

void PlayerData::applyToPlayer(gd::PlayerObject* player) {
    if(isSet) {
        MBO(CCPoint, player, 0x67C) = position;
        MBO(double, player, 0x628) = yVel;

        if(rotation != -1)
            player->setRotation(rotation);
    }
}

PlayerData PlayerData::fromJson(json jsonData, PlayerData original) {
    PlayerData data;

    data.position = CCPoint(
        jsonData["x"], jsonData["y"]
    );

    data.yVel = jsonData["a"];

    if(jsonData.contains("down")) {
        data.action = jsonData["down"] ? Pressed : Released;
    }
    else data.action = original.action;

    data.isSet = true;

    return data;
}

bool GatoBot::hasMissingFrames() {
    bool contains = false;

    for(auto& f : levelFrames) {
        if(f.frame == -1) {
            contains = true;
            break;
        }
    }

    return contains;
}