#include "RecordPopup.hpp"
#include "GatoBotMenu.hpp"

bool RecordPopup::init(bool isReplayScreen) {
    if(!initWithColor(ccc4(0, 0, 0, 105))) return false;

    isReplay = isReplayScreen;

    setTag(0x92);

    auto bot = GatoBot::sharedState();
    auto dir = CCDirector::sharedDirector();
    auto winSize = dir->getWinSize();

    m_pLayer = CCLayer::create();
    addChild(m_pLayer);

    m_pButtonMenu = CCMenu::create();
    m_pLayer->addChild(m_pButtonMenu, 10);

    // bg
    auto bg = extension::CCScale9Sprite::create("GJ_square01.png", {0, 0, 80, 80});
    bg->setContentSize(CCSize(300, 180));
    bg->setPosition(winSize / 2);

    m_pLayer->addChild(bg);

    // title label
    const char* titleLabelText = isReplayScreen ? "Replay" : "Record";    

    auto titleLabel = CCLabelBMFont::create(titleLabelText, "goldFont.fnt");
    titleLabel->setPosition(winSize / 2 + CCPoint(0, bg->getContentSize().height / 2 - 20));
    titleLabel->limitLabelWidth(200, 1, .4);

    m_pLayer->addChild(titleLabel, 1);

    // buttons
    auto recBtnSpr = gd::ButtonSprite::create("Start", 0, false, "goldFont.fnt", "GJ_button_01.png", 0, 1);
    auto recBtn = gd::CCMenuItemSpriteExtra::create(recBtnSpr, this, menu_selector(RecordPopup::onStart));

    m_pButtonMenu->addChild(recBtn);

    auto cancelBtnSpr = gd::ButtonSprite::create("Cancel", 0, false, "goldFont.fnt", "GJ_button_06.png", 0, 1);
    auto cancelBtn = gd::CCMenuItemSpriteExtra::create(cancelBtnSpr, this, menu_selector(RecordPopup::onCancel));

    m_pButtonMenu->addChild(cancelBtn);

    m_pButtonMenu->alignItemsHorizontallyWithPadding(5);

    recBtn->setPositionY(-bg->getContentSize().height / 2 + 25);
    cancelBtn->setPositionY(-bg->getContentSize().height / 2 + 25);

    // fps
    auto fpsLabel = CCLabelBMFont::create("FPS:", "bigFont.fnt");
    fpsLabel->setPosition(winSize / 2 + CCPoint(-50, 30));

    m_pLayer->addChild(fpsLabel, 1);

    auto fpsInputBG = extension::CCScale9Sprite::create("square02_001.png", {0, 0, 80, 80});
    fpsInputBG->setContentSize(CCSize(280, 80));
    fpsInputBG->setScale(.4);
    fpsInputBG->setOpacity(100);
    fpsInputBG->setPosition(winSize / 2 + CCPoint(60, 30));
    m_pLayer->addChild(fpsInputBG, 1);

    fpsInput = gd::CCTextInputNode::create("FPS", this, "bigFont.fnt", 100, 40);
    fpsInput->setString(std::to_string(bot->getCurrentFPS()).c_str());
    fpsInput->setPosition(CCPoint(60, 30));
    fpsInput->setMaxLabelLength(4);
    fpsInput->setAllowedChars("0123456789");
    fpsInput->setLabelPlaceholderColor({120, 170, 240});

    m_pButtonMenu->addChild(fpsInput, 2);

    // speed
    auto speedLabel = CCLabelBMFont::create("Speed:", "bigFont.fnt");
    speedLabel->setPosition(winSize / 2 + CCPoint(-50, -10));

    m_pLayer->addChild(speedLabel, 1);

    auto speedLabelBG = extension::CCScale9Sprite::create("square02_001.png", {0, 0, 80, 80});
    speedLabelBG->setContentSize(CCSize(280, 80));
    speedLabelBG->setScale(.4);
    speedLabelBG->setOpacity(100);
    speedLabelBG->setPosition(winSize / 2 + CCPoint(60, -10));
    m_pLayer->addChild(speedLabelBG, 1);

    speedInput = gd::CCTextInputNode::create("Speedhack", this, "bigFont.fnt", 100, 40);
    speedInput->setString(std::to_string(dir->getScheduler()->getTimeScale()).c_str());
    speedInput->setPosition(CCPoint(60, -10));
    speedInput->setMaxLabelLength(10);
    speedInput->setAllowedChars(".0123456789");
    speedInput->setLabelPlaceholderColor({120, 170, 240});

    m_pButtonMenu->addChild(speedInput, 2);

    setTouchEnabled(true);
    setKeypadEnabled(true);
    
    return true;
}

void RecordPopup::open(GatoBotMenu* parent, bool isReplay) {
    auto bot = GatoBot::sharedState();
    auto dir = CCDirector::sharedDirector();
    auto scene = dir->getRunningScene();

    if(scene->getChildByTag(0x92) == nullptr) {
        auto layer = create(parent, isReplay);
        layer->registerWithTouchDispatcher();
        dir->getTouchDispatcher()->incrementForcePrio(2);
        layer->m_pTargetLayer = parent;

        parent->toggleButtons(false);

        layer->show();
    }
}

void RecordPopup::onStart(CCObject*) {
    auto bot = GatoBot::sharedState();
    auto dir = CCDirector::sharedDirector();

    int fpsVal = std::stoi(fpsInput->getString());
    float speedVal = std::stof(speedInput->getString());

    if(isReplay) bot->toggleReplay(1.f / (fpsVal * speedVal), speedVal);
    else bot->toggleRecord(1.f / (fpsVal * speedVal), speedVal);

    onCancel(nullptr);
    parentMenu->keyBackClicked();
    
    bot->retryLevel();
}

void RecordPopup::onCancel(CCObject*) {
    keyBackClicked();
}

void RecordPopup::keyBackClicked() {
    parentMenu->toggleButtons(true);

    FLAlertLayer::keyBackClicked();
}

