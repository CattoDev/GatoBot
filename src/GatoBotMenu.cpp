#include "GatoBotMenu.hpp"
#include "SettingsPopup.hpp"
#include "RecordPopup.hpp"

gd::CCMenuItemSpriteExtra* GatoBotMenu::createButton(CCSprite* spr, SEL_MenuHandler callback) {
    auto bgSpr = CCSprite::create("GJ_button_02.png");
    bgSpr->addChild(spr);
    spr->setPosition(bgSpr->getContentSize() / 2);

    return gd::CCMenuItemSpriteExtra::create(bgSpr, this, callback);
}

bool GatoBotMenu::init() {
    if(!initWithColor(ccc4(0, 0, 0, 130))) return false;

    setTag(0xAE);
    GatoBot::sharedState()->botMenu = this;

    auto dir = CCDirector::sharedDirector();
    auto winSize = dir->getWinSize();
    
    // bg
    auto bg = extension::CCScale9Sprite::create("square02_001.png", {0, 0, 80, 80});
    bg->setOpacity(100);
    bg->setContentSize(CCSize(300, 80));
    bg->setScale(.4);

    addChild(bg);

    // title
    auto titleLabel = CCLabelBMFont::create("GatoBot", "goldFont.fnt");
    titleLabel->setPosition(winSize.width / 2, winSize.height / 2 + 40);

    addChild(titleLabel, 1);

    // buttons
    m_pButtonMenu = CCMenu::create();
    addChild(m_pButtonMenu, 1);

    auto recBtnSpr = CCSprite::create("circle.png");
    recBtnSpr->setScale(2);
    recBtnSpr->setColor(ccc3(255, 0, 0));
    auto recBtn = createButton(recBtnSpr, menu_selector(GatoBotMenu::onRecord));

    m_pButtonMenu->addChild(recBtn);

    auto repBtnSpr = CCSprite::createWithSpriteFrameName("edit_leftBtn2_001.png");
    auto repBtn = createButton(repBtnSpr, menu_selector(GatoBotMenu::onReplay));
    repBtnSpr->setPositionX(repBtnSpr->getPositionX() - 2);

    m_pButtonMenu->addChild(repBtn);

    auto renBtnSpr = CCSprite::createWithSpriteFrameName("gj_ytIcon_001.png");
    renBtnSpr->setScale(1.4);
    auto renBtn = gd::CCMenuItemSpriteExtra::create(renBtnSpr, this, menu_selector(GatoBotMenu::onRender));

    m_pButtonMenu->addChild(renBtn); 

    auto savBtnSpr = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
    savBtnSpr->setScale(1.1);
    auto savBtn = gd::CCMenuItemSpriteExtra::create(savBtnSpr, this, menu_selector(GatoBotMenu::onSaveReplay));

    m_pButtonMenu->addChild(savBtn); 

    auto loaBtnSpr = CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png");
    auto loaBtn = createButton(loaBtnSpr, menu_selector(GatoBotMenu::onLoadReplay));

    m_pButtonMenu->addChild(loaBtn); 

    m_pButtonMenu->alignItemsHorizontallyWithPadding(10);

    // set tags
    for(int i = 0; i < m_pButtonMenu->getChildrenCount(); i++) {
        reinterpret_cast<gd::CCMenuItemSpriteExtra*>(m_pButtonMenu->getChildren()->objectAtIndex(i))->setTag(i + 1);
    }

    // button label
    buttonLabel = CCLabelBMFont::create(" ", "bigFont.fnt");
    buttonLabel->setScale(.5);
    buttonLabel->setPosition(winSize.width / 2, winSize.height / 2 - 40);
    
    addChild(buttonLabel, 1);

    // other
    setTouchEnabled(true);
    setKeypadEnabled(true);

    scheduleUpdate();

    return true;
}

void GatoBotMenu::update(float) {
    auto dir = CCDirector::sharedDirector();

    if(dir->getRunningScene()->getChildByTag(0xA) != nullptr) return;

    auto winSize = dir->getWinSize();
    auto view = dir->getOpenGLView();
    auto mPos = view->getMousePosition();
    auto viewPortRect = view->getViewPortRect();

    mPos.y = viewPortRect.size.height - mPos.y;

    float xRatio = winSize.width / viewPortRect.size.width;
    float yRatio = winSize.height / viewPortRect.size.height;

    float posX = mPos.x * xRatio;
    float posY = mPos.y * yRatio;

    for(size_t i = 0; i < m_pButtonMenu->getChildrenCount(); i++) {
        auto btn = (gd::CCMenuItemSpriteExtra*)m_pButtonMenu->getChildren()->objectAtIndex(i);

        auto size = btn->getScaledContentSize();

        auto locInNode = btn->convertToNodeSpace(CCPoint(posX, posY));
        if(CCRect(0, 0, size.width, size.height).containsPoint(locInNode)) {
            if(lastHoveredButtonID != btn->getTag()) {
                buttonHovered(btn);
                lastHoveredButtonID = btn->getTag();
                break;
            }
        }
    }
}

const std::vector<const char*> buttonLabels = {"Record", "Replay", "Render", "Save Replay", "Load Replay"};
void GatoBotMenu::buttonHovered(gd::CCMenuItemSpriteExtra* btn) {
    auto bot = GatoBot::sharedState();
    int index = btn->getTag() - 1;

    if(index < buttonLabels.size()) {
        const char* str = buttonLabels[index];

        // garbaj
        if(bot->status == Recording && index == 0) str = "Stop Recording";
        if(bot->status == Replaying && index == 1) str = "Stop Replaying";
        if(bot->status == Rendering && index == 2) str = "Stop Rendering";
        
        buttonLabel->setString(str);
    }
}

void GatoBotMenu::onOpen(CCObject*) {
    auto menu = create();
    auto dir = CCDirector::sharedDirector();
    auto scene = dir->getRunningScene();

    if(scene->getChildByTag(0xAE) == nullptr) {
        scene->addChild(menu, 1000);

        menu->registerWithTouchDispatcher();
        dir->getTouchDispatcher()->incrementForcePrio(2);
    }
}

void GatoBotMenu::FLAlert_Clicked(gd::FLAlertLayer* alert, bool btn2) {
    if(alert->getTag() == 1 && !btn2) {
        // load replay
        GatoBot::sharedState()->loadNewReplay();
    }

    if(alert->getTag() == 2 && !btn2) {
        // open record menu
        RecordPopup::open(this);
    }

    if(alert->getTag() == 3 && !btn2) {
        // toggle render
        GatoBot::sharedState()->toggleRender();
    }
    if(alert->getTag() == 4 && !btn2) {
        // download ffmpeg
        CCApplication::sharedApplication()->openURL("https://www.gyan.dev/ffmpeg/builds/");
    }
}

void GatoBotMenu::onRecord(CCObject*) {
    auto bot = GatoBot::sharedState();

    if(bot->status == Rendering) {
        auto layer = gd::FLAlertLayer::create(this, "Warning", "Yes", "No", 400, "Are you sure you want to stop rendering?");
        layer->setTag(3);
        layer->m_pTargetLayer = this;
        layer->show();

        return;
    }
    if(bot->levelFrames.size() > 0 && bot->status != Recording) {
        auto alert = gd::FLAlertLayer::create(this, "Warning", "Yes", "No", 400, "Are you sure you want to record a new replay?\n<cy>The current replay will be overwritten</c>.");
        alert->setTag(2);
        alert->m_pTargetLayer = this;
        alert->show();
    }
    else {
        if(bot->status != Recording) RecordPopup::open(this);
        else bot->toggleRecord();
    }
}

void GatoBotMenu::onReplay(CCObject*) {
    auto bot = GatoBot::sharedState();

    if(bot->status == Rendering) {
        auto layer = gd::FLAlertLayer::create(this, "Warning", "Yes", "No", 400, "Are you sure you want to stop rendering?");
        layer->setTag(3);
        layer->m_pTargetLayer = this;
        layer->show();

        return;
    }

    if(bot->levelFrames.size() == 0) {
        auto layer = gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "There are no frames to replay!");
        layer->m_pTargetLayer = this;
        layer->show();
    }
    else {
        if(bot->status != Replaying) RecordPopup::open(this, true);
        else bot->toggleReplay();
    }
}

void GatoBotMenu::onRender(CCObject*) {
    auto bot = GatoBot::sharedState();

    if(bot->status != Rendering) {
        if(!bot->FFmpegInstalled()) {
            auto alert = gd::FLAlertLayer::create(this, "Alert", "Download", "Discard", 300, "FFmpeg not found in the game directory!\nDownload?");
            alert->setTag(4);
            alert->m_pTargetLayer = this;
            alert->show();
            return;
        }

        if(bot->levelFrames.size() > 0) {
            auto dir = CCDirector::sharedDirector();
            auto scene = dir->getRunningScene();

            if(scene->getChildByTag(0xA) == nullptr) {
                auto popup = SettingsPopup::create();
                popup->m_pTargetLayer = this;
                popup->registerWithTouchDispatcher();
                dir->getTouchDispatcher()->incrementForcePrio(2);

                toggleButtons(false);

                popup->show();
            }
        }
        else {
            auto alert = gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "There are no frames to render!");
            alert->m_pTargetLayer = this;
            alert->show();
        }
    }
    else {
        bot->toggleRender();
    }
}

void GatoBotMenu::onSaveReplay(CCObject*) {
    auto bot = GatoBot::sharedState();

    if(bot->levelFrames.size() > 0) 
        bot->saveCurrentReplay();

    else {
        auto alert = gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "There are no frames to save!");
        alert->m_pTargetLayer = this;        
        alert->show();
    }
}

void GatoBotMenu::onLoadReplay(CCObject*) {
    auto bot = GatoBot::sharedState();

    if(bot->levelFrames.size() > 0) {
        auto alert = gd::FLAlertLayer::create(this, "Warning", "Yes", "No", 400, "Are you sure you want to load a new replay?\n<cy>The current replay will be overwritten</c>.");
        alert->setTag(1);
        alert->m_pTargetLayer = this;
        alert->show();
    }
    else {
        bot->loadNewReplay();
    }
}

void GatoBotMenu::toggleButtons(bool enabled) {
    for(size_t i = 0; i < m_pButtonMenu->getChildrenCount(); i++) {
        auto btn = (gd::CCMenuItemSpriteExtra*)m_pButtonMenu->getChildren()->objectAtIndex(i);
        btn->setEnabled(enabled);
    }
}