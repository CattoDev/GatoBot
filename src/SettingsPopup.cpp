#include "SettingsPopup.hpp"
#include "GatoBotMenu.hpp"

#include <nfd.h>

extension::CCScale9Sprite* SettingsPopup::createTextInput(const char* caption, int maxchars, int width, int height, const char* charFilter = nullptr, CCPoint pos = CCPointZero, float scale = .4f, const char* font = "bigFont.fnt") {
    // bitrate input
    auto inputBG = extension::CCScale9Sprite::create("square02_001.png", {0, 0, 80, 80});
    inputBG->setScale(scale);
    inputBG->setContentSize(CCSize(width / scale + 50, height / scale));
    inputBG->setOpacity(100);
    inputBG->setPosition(pos);

    m_pLayer->addChild(inputBG, 1);

    auto input = gd::CCTextInputNode::create(caption, this, font, width, height);
    input->setPosition(m_pButtonMenu->convertToNodeSpace(m_pLayer->convertToWorldSpace(pos)));
    input->setLabelPlaceholderColor({120, 170, 240});

    if(charFilter != nullptr)
        input->m_sFilter = std::string(charFilter);
    
    input->setMaxLabelLength(maxchars);

    m_pButtonMenu->addChild(input, 1);

    textInputs.push_back(input);

    return inputBG;
}

gd::CCMenuItemSpriteExtra* SettingsPopup::createHelpBtn(const char* helpText, CCPoint pos, float scale) {
    auto helpBtnSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    helpBtnSpr->setScale(scale);
    auto helpBtn = gd::CCMenuItemSpriteExtra::create(helpBtnSpr, this, menu_selector(SettingsPopup::onHelp));
    helpBtn->setPosition(pos);

    helpBtn->setTag(helpTexts.size());
    helpTexts.push_back(helpText);

    m_pButtonMenu->addChild(helpBtn, 3);

    return helpBtn;
}

void SettingsPopup::createToggle(const char* text, const char* helpText = nullptr) {
    // positioning
    auto togglePos = CCPoint(-210, 230);

    int differenceY = 30 * toggles.size();
    togglePos = togglePos + CCPoint(0, -differenceY);

    // checkbox
    auto toggle = gd::CCMenuItemToggler::createWithStandardSprites(this, menu_selector(SettingsPopup::onToggle), .8);
    toggle->setPosition(togglePos);
    
    toggle->setTag(toggles.size());
    toggles.push_back(toggle);

    // cell
    auto cell = GBOptionCell::create(toggle, text);

    optScrollLayer->cells->addChild(cell);

    // help button
    if(helpText != nullptr) {
        auto helpBtn = createHelpBtn(helpText, togglePos + CCPoint(-20, 10), .35);

        helpBtn->removeFromParentAndCleanup(false);
        cell->addHelpButton(helpBtn);

        helpBtn->setTag(helpTexts.size());
        helpTexts.push_back(helpText);
    }
}

void SettingsPopup::createCodecBtn(const char* caption, const char* bg, const char* codec, CCPoint pos) {
    auto spr = gd::ButtonSprite::create(caption, 60, true, "bigFont.fnt", bg, 40, .8);
    spr->setScale(.4);
    auto btn = gd::CCMenuItemSpriteExtra::create(spr, this, menu_selector(SettingsPopup::onCodec));
    btn->setPosition(pos);

    btn->setTag(codecs.size());
    codecs.push_back(codec);

    m_pButtonMenu->addChild(btn, 1);
}

void SettingsPopup::createResBtn(const char* caption, const char* bg, ResolutionSize res) {
    auto spr = gd::ButtonSprite::create(caption, 60, true, "bigFont.fnt", bg, 40, .8);
    spr->setScale(.4);
    auto btn = gd::CCMenuItemSpriteExtra::create(spr, this, menu_selector(SettingsPopup::onResolution));

    btn->setTag(resolutions.size());
    resolutions.push_back(res);

    resScrollLayer->cells->addChild(btn);
}

bool SettingsPopup::init() {
    if(!initWithColor(ccc4(0, 0, 0, 105))) return false;

    auto bot = GatoBot::sharedState();
    auto dir = CCDirector::sharedDirector();
    auto winSize = dir->getWinSize();

    setTag(0xA);

    m_pLayer = CCLayer::create();
    addChild(m_pLayer);

    m_pButtonMenu = CCMenu::create();
    m_pButtonMenu->setPositionY(m_pButtonMenu->getPositionY() - 115);
    m_pLayer->addChild(m_pButtonMenu, 10);

    // bg
    auto bg = extension::CCScale9Sprite::create("GJ_square02.png", {0, 0, 80, 80});
    bg->setContentSize(CCSize(480, 240));
    bg->setPosition(winSize / 2);

    m_pLayer->addChild(bg);

    // title
    auto titleLabel = CCLabelBMFont::create("Render", "goldFont.fnt");
    titleLabel->setPosition(winSize / 2 + CCPoint(0, 105));

    m_pLayer->addChild(titleLabel, 1);

    // ok btn
    auto okBtnSpr = gd::ButtonSprite::create("START", 0, false, "goldFont.fnt", "GJ_button_01.png", 0, 1);
    auto okBtn = gd::CCMenuItemSpriteExtra::create(okBtnSpr, this, menu_selector(SettingsPopup::onApply));

    m_pButtonMenu->addChild(okBtn);

    // cancel btn
    auto cancelBtnSpr = gd::ButtonSprite::create("CANCEL", 0, false, "goldFont.fnt", "GJ_button_06.png", 0, 1);
    auto cancelBtn = gd::CCMenuItemSpriteExtra::create(cancelBtnSpr, this, menu_selector(SettingsPopup::onCancel));

    m_pButtonMenu->addChild(cancelBtn);

    m_pButtonMenu->alignItemsHorizontallyWithPadding(10);

    okBtn->setPositionY(20);
    cancelBtn->setPositionY(20);

    // bitrate label
    auto bitrateLabel = CCLabelBMFont::create("Video Bitrate (kbps):", "bigFont.fnt");
    bitrateLabel->setScale(.3);
    bitrateLabel->setPosition(winSize.width / 2 + 160, winSize.height / 2 + 105);

    m_pLayer->addChild(bitrateLabel, 1);

    // video bitrate input
    createTextInput("bitrate", 6, 100, 30, "0123456789", winSize / 2 + CCPoint(160, 80));
    textInputs[0]->setString(std::to_string(bot->settings.bitrate).c_str());

    // codec label
    auto codecLabel = CCLabelBMFont::create("Codec:", "bigFont.fnt");
    codecLabel->setScale(.5);
    codecLabel->setPosition(winSize.width / 2 + 160, winSize.height / 2 - 5);

    m_pLayer->addChild(codecLabel, 1);

    // codec input
    createTextInput("codec", 20, 100, 30, nullptr, winSize / 2 + CCPoint(160, -30));
    textInputs[1]->setString(bot->settings.codec.c_str());

    // codec help
    createHelpBtn("Which codec to use when encoding the video\n<cb>Intel (CPU) - uses the default h264 codec to encode</c>\n<cg>NVIDIA (GPU) - uses the h264 NVENC codec to encode</c>\n<cr>AMD (GPU) - uses the h264 AMF codec to encode</c>\n<cy>Using NVENC or AMF is usually faster</c>\nYou can also use other codecs, like HEVC (<cy>for 8K videos</c>)", textInputs[1]->getPosition() + CCPoint(50, 25), .6);

    // codec buttons
    createCodecBtn("Intel", "GJ_button_02.png", "h264", {120, 55});
    createCodecBtn("NVIDIA", "GJ_button_01.png", "h264_nvenc", {160, 55});
    createCodecBtn("AMD", "GJ_button_06.png", "h264_amf", {200, 55});

    // settings scrolling
    optScrollLayer = SettingsPopupScrollLayer::create({140, 180});
    optScrollLayer->setPosition(winSize / 2 + CCPoint(-160, 20));

    m_pLayer->addChild(optScrollLayer, 10);

    // settings
    createToggle("Capture MHv7\nLabels", "Capture MHv7 labels, like run info, cheat indicator...");
    createToggle("Capture MHv7\nDraw Nodes", "Capture Mega Hack v7 draw nodes, such as hitboxes and trajectory lines.");
    createToggle("Capture %", "Capture the percentage label.");
    createToggle("Include Level\nSong", "Output the video with the level song.");
    createToggle("Show Console", "Show the FFmpeg console window.");
    createToggle("Preview Frame", "Show the frame that's currently being rendered.\n<cy>Disabling this option may improve performance on Low-End PCs.</c>");
    createToggle("Ending Delay", "Delay finishing a render for a specific amount of time.\nCan be used to show level endscreens and such.");

    optScrollLayer->setupList();

    // preset toggle
    toggles[3]->toggle(true);
    toggles[4]->toggle(bot->settings.showConsoleWindow);
    toggles[5]->toggle(true);

    videoPath = bot->settings.videoPath;

    // resolution input
    auto resLabel = CCLabelBMFont::create("Resolution", "bigFont.fnt");
    resLabel->setScale(.45);
    resLabel->setPosition(winSize.width / 2, winSize.height / 2 + 85);

    m_pLayer->addChild(resLabel, 8);

    createResInputs(winSize / 2 + CCPoint(0, 67.5));

    // resolution presets
    auto resScrollLayerBG = extension::CCScale9Sprite::create("square02_001.png", {0, 0, 80, 80});
    resScrollLayerBG->setOpacity(100);
    resScrollLayerBG->setScale(.3);
    resScrollLayerBG->setContentSize({440, 80});

    resScrollLayer = SettingsPopupScrollLayer::create({130, 60});
    resScrollLayer->horizontal = true;
    resScrollLayer->useCells = false;
    resScrollLayer->itemSize = 35;

    createResBtn("360p", "GJ_button_04.png", {640, 360, 2000});
    createResBtn("480p", "GJ_button_04.png", {854, 480, 4000});
    createResBtn("720p", "GJ_button_04.png", {1280, 720, 6000});
    createResBtn("1080p", "GJ_button_04.png", {1920, 1080, 10000});
    createResBtn("2K", "GJ_button_04.png", {2560, 1440, 25000});
    createResBtn("4K", "GJ_button_04.png", {3840, 2160, 50000});
    createResBtn("8K", "GJ_button_04.png", {7680, 4320, 100000});

    resScrollLayer->setPosition(winSize / 2 + CCPoint(0, 45));
    m_pLayer->addChild(resScrollLayer, 10);
    resScrollLayer->setupList();

    resScrollLayerBG->setPosition(winSize / 2 + CCPoint(0, 45));
    m_pLayer->addChild(resScrollLayerBG, 5);

    // file path
    createTextInput("output path", MAX_PATH, 110, 60, nullptr, m_pLayer->convertToNodeSpace(m_pButtonMenu->convertToWorldSpace(CCPoint(-15, 80))), .4, "chatFont.fnt");
    auto filePathInput = textInputs[5];
    filePathInput->setDelegate(this);
    filePathInput->setVisible(false);

    filePathArea = gd::TextArea::create("chatFont.fnt", true, "file path", 1, 160, 15, CCPoint(.5, .5));
    filePathArea->setPosition(filePathInput->getPosition());
    filePathArea->setScale(.75);
    m_pButtonMenu->addChild(filePathArea, 1);

    auto filePathBtnSpr = CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png");
    auto filePathBtn = gd::CCMenuItemSpriteExtra::create(filePathBtnSpr, this, menu_selector(SettingsPopup::onChoosePath));
    filePathBtn->setPosition(70, filePathInput->getPositionY());

    m_pButtonMenu->addChild(filePathBtn);

    // audio bitrate input
    auto audioBitrateLabel = CCLabelBMFont::create("Audio Bitrate (kbps):", "bigFont.fnt");
    audioBitrateLabel->setScale(.3);
    audioBitrateLabel->setPosition(winSize.width / 2 + 160, winSize.height / 2 + 50);

    m_pLayer->addChild(audioBitrateLabel, 1);

    createTextInput("bitrate", 6, 100, 30, "0123456789", winSize / 2 + CCPoint(160, 25));
    textInputs.back()->setString(std::to_string(bot->settings.audioBitrate).c_str());

    // extra arguments
    //createTextInput("extra args (optional)", 200, 140, 30, nullptr, winSize / 2 + CCPoint(0, -15), .4, "chatFont.fnt");
    //textInputs[7]->setString(bot->settings.extraArguments.c_str());

    // game fps
    createTextInput("FPS", 4, 60, 30, "0123456789", winSize / 2 + CCPoint(40, 17.5), .35);
    textInputs.back()->setString(std::to_string(bot->getCurrentFPS()).c_str());

    auto fpsLabel = CCLabelBMFont::create("Game FPS:", "bigFont.fnt");
    fpsLabel->setPosition(winSize / 2 + CCPoint(-40, 17.5));
    fpsLabel->limitLabelWidth(80, 1, .1);
    m_pLayer->addChild(fpsLabel, 2);

    // delay input
    delayInputBG = createTextInput("end delay (s)", 8, 50, 30, "012345679.", winSize / 2 + CCPoint(-200, -90), .3);
    delayInput = textInputs.back();

    delayInputBG->setVisible(false);
    delayInput->setVisible(false);

    // other
    setTouchEnabled(true);
    setKeypadEnabled(true);
    setMouseEnabled(true);
    
    return true;
}

void SettingsPopup::createResInputs(CCPoint position) {
    auto bot = GatoBot::sharedState();
    auto dir = CCDirector::sharedDirector();
    auto frameSize = dir->getOpenGLView()->getFrameSize();
    auto winSize = dir->getWinSize();

    createTextInput("width", 4, 30, 15, "0123456789", position + CCPoint(-50, 0), .2);
    createTextInput("height", 4, 30, 15, "0123456789", position, .2);
    createTextInput("FPS", 3, 30, 15, "0123456789", position + CCPoint(50, 0), .2);

    // labels
    auto xLabel = CCLabelBMFont::create("x", "bigFont.fnt");
    xLabel->setScale(.4);

    auto slashLabel = CCLabelBMFont::create("/", "bigFont.fnt");
    slashLabel->setScale(.4);

    m_pLayer->addChild(xLabel, 1);
    m_pLayer->addChild(slashLabel, 1);

    xLabel->setPosition(m_pLayer->convertToNodeSpace(m_pButtonMenu->convertToWorldSpace(textInputs[2]->getPosition() + CCPoint(25, 0))));
    slashLabel->setPosition(m_pLayer->convertToNodeSpace(m_pButtonMenu->convertToWorldSpace(textInputs[3]->getPosition() + CCPoint(25, 0))));

    textInputs[2]->setMaxLabelScale(.4);
    textInputs[3]->setMaxLabelScale(.4);
    textInputs[4]->setMaxLabelScale(.4);

    // default values
    textInputs[2]->setString(std::to_string(bot->settings.targetWidth).c_str());
    textInputs[3]->setString(std::to_string(bot->settings.targetHeight).c_str());
    textInputs[4]->setString(std::to_string(bot->settings.targetFPS).c_str());
}

void SettingsPopup::onHelp(CCObject* pSender) {
    auto btn = (gd::CCMenuItemSpriteExtra*)pSender;

    auto alert = gd::FLAlertLayer::create(nullptr, "Info", "OK", nullptr, 400, std::string(helpTexts[btn->getTag()]));
    alert->m_pTargetLayer = this;
    alert->show();
}

void SettingsPopup::onCodec(CCObject* pSender) {
    auto btn = (gd::CCMenuItemSpriteExtra*)pSender;

    textInputs[1]->setString(codecs[btn->getTag()]);
}

void SettingsPopup::onResolution(CCObject* pSender) {
    auto btn = (gd::CCMenuItemSpriteExtra*)pSender;
    int tag = btn->getTag();

    auto res = resolutions[tag];

    textInputs[2]->setString(std::to_string(res.width).c_str());
    textInputs[3]->setString(std::to_string(res.height).c_str());
    textInputs[4]->setString("60");
    textInputs[0]->setString(std::to_string(res.bitrate).c_str());
}

void SettingsPopup::onChoosePath(CCObject*) {
    // get video path
    nfdchar_t *outPath = NULL;
    nfdresult_t res = NFD_SaveDialog({"mp4"}, nullptr, &outPath);

    if(res != NFD_OKAY) {
        free(outPath);

        auto alert = gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, 400, "Something went wrong when choosing path.");
        alert->m_pTargetLayer = this;
        alert->show();
        return;
    } 

    videoPath = std::string(outPath);
    if(videoPath.find(".mp4") == videoPath.npos) videoPath.append(".mp4");

    free(outPath);

    textInputs[5]->setString(videoPath.c_str());
    filePathArea->setString(videoPath);
}

void SettingsPopup::onToggle(CCObject* pSender) {
    auto toggle = (gd::CCMenuItemToggler*)pSender;

    if(toggle->getTag() == toggles[4]->getTag()) {
        if(!toggles[4]->isToggled()) {
            AllocConsole();
            freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        }
        else {
            auto window = GetConsoleWindow();
            FreeConsole();
            PostMessage(window, WM_CLOSE, 0, 0);
            GatoBot::sharedState()->settings.showConsoleWindow = false;
        }
    }

    if(toggle->getTag() == toggles[6]->getTag()) {
        bool toggled = !toggle->isToggled();

        delayInputBG->setVisible(toggled);
        delayInput->setVisible(toggled);
    }
}

void SettingsPopup::onApply(CCObject*) {
    auto bot = GatoBot::sharedState();

    // save settings
    int frameWidth = std::stoi(textInputs[2]->getString());
    int frameHeight = std::stoi(textInputs[3]->getString());
    int enteredFPS = std::stoi(textInputs[4]->getString());
    int enteredGameFPS = std::stoi(textInputs[7]->getString());

    // width and height have to be even numbers
    if(frameWidth % 2 != 0 || frameHeight % 2 != 0) {
        auto alert = gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, 400, "Width and Height have to be divisible by 2.");
        alert->m_pTargetLayer = this;
        alert->show();
        return;
    }

    // FPS
    if(enteredGameFPS % enteredFPS != 0) {
        auto alert = gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, 400, CCString::createWithFormat("Current FPS has to be divisible by target FPS.\n<cy>%i %% %i</c> <cr>!=</c> <cy>0</c>", enteredGameFPS, enteredFPS)->m_sString);
        alert->m_pTargetLayer = this;
        alert->show();
        
        return;
    }

    videoPath = std::string(textInputs[5]->getString());

    // path
    if(videoPath.empty()) {
        auto alert = gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, 400, "Please choose where to save the video.");
        alert->m_pTargetLayer = this;
        alert->show();
        return;
    }

    // delay
    float delay = std::strtof(delayInput->getString(), nullptr);

    if(delay > 99) delay = 99;
    if(delay < 0) delay = 0;

    auto dir = CCDirector::sharedDirector();
    bot->lastSPF = dir->getAnimationInterval();
    dir->setAnimationInterval(1.f / (float)enteredGameFPS);

    bot->settings.bitrate = std::stoi(textInputs[0]->getString());
    bot->settings.audioBitrate = std::stoi(textInputs[6]->getString());

    bot->settings.captureMegaHackLabels = toggles[0]->isToggled();
    bot->settings.captureMegaHackDrawNodes = toggles[1]->isToggled();
    bot->settings.capturePercentage = toggles[2]->isToggled();
    bot->settings.includeLevelSong = toggles[3]->isToggled();

    bot->settings.codec = std::string(textInputs[1]->getString());
    bot->settings.videoPath = videoPath;

    bot->settings.targetWidth = frameWidth;
    bot->settings.targetHeight = frameHeight;
    bot->settings.targetFPS = enteredFPS;
    bot->settings.targetGameFPS = enteredGameFPS;
    bot->settings.showConsoleWindow = toggles[4]->isToggled();
    bot->settings.fastRender = !toggles[5]->isToggled();
    bot->settings.delayEnd = toggles[6]->isToggled();

    bot->settings.renderDelay = delay;

    //bot->settings.extraArguments = std::string(textInputs[7]->getString());

    onCancel(nullptr); // just close the layer

    // exit
    auto gbmenu = CCDirector::sharedDirector()->getRunningScene()->getChildByTag(0xAE);
    if(gbmenu != nullptr) reinterpret_cast<gd::FLAlertLayer*>(gbmenu)->keyBackClicked();

    GatoBot::sharedState()->toggleRender();

    reinterpret_cast<void(__thiscall*)(gd::PauseLayer*, CCObject*)>(gd::base + 0x1E6040)(bot->currentPauseLayer, nullptr);
}

void SettingsPopup::onCancel(CCObject*) {
    keyBackClicked();
}

void SettingsPopup::keyBackClicked() {
    GatoBot::sharedState()->botMenu->toggleButtons(true);

    FLAlertLayer::keyBackClicked();
}

void SettingsPopup::textChanged(gd::CCTextInputNode* inputNode) {
    auto str = std::string(inputNode->getString());
    if(str.length() <= 120)
        filePathArea->setString(str);
}

void SettingsPopup::scrollWheel(float yDiff, float p2) {
    auto dir = CCDirector::sharedDirector();
    auto glView = CCEGLView::sharedOpenGLView();
    auto mousePos = glView->getMousePosition();

    auto winSize = dir->getWinSize();
    auto frameSize = glView->getFrameSize();

    mousePos.y = frameSize.height - mousePos.y;

    // convert to cocos
    mousePos.x *= winSize.width / frameSize.width;
    mousePos.y *= winSize.height / frameSize.height;

    // options
    if(optScrollLayer->getAreaRect().containsPoint(mousePos))
        optScrollLayer->scrollWheel(yDiff, p2);

    // resolutions
    if(resScrollLayer->getAreaRect().containsPoint(mousePos))
        resScrollLayer->scrollWheel(yDiff, p2);
}