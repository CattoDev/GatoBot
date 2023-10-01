#include "GatoBot.hpp"
#include "ScrollNode.hpp"

struct ResolutionSize {
    int width;
    int height;
    int videoBitrate;
    int audioBitrate;
};

class SettingsPopup : public gd::FLAlertLayer, public gd::TextInputDelegate {
public:
    std::vector<gd::CCTextInputNode*> textInputs;
    std::vector<gd::CCMenuItemToggler*> toggles;

    std::vector<const char*> codecs;
    std::vector<ResolutionSize> resolutions;
    std::vector<const char*> helpTexts;

    gd::TextArea* filePathArea;
    gd::CCTextInputNode* delayInput;
    extension::CCScale9Sprite* delayInputBG;

    std::string videoPath;

    // settings
    SettingsPopupScrollLayer* optScrollLayer; // options
    SettingsPopupScrollLayer* resScrollLayer; // resolutions

public:
    static SettingsPopup* create() {
        auto pRet = new SettingsPopup();

        if(pRet && pRet->init()) {
            pRet->autorelease();
            return pRet;
        }
        CC_SAFE_DELETE(pRet);
        return nullptr;
    }

    bool init();
    void keyBackClicked() override;

    extension::CCScale9Sprite* createTextInput(const char*, int, int, int, const char*, CCPoint, float, const char*);
    gd::CCMenuItemSpriteExtra* createHelpBtn(const char*, CCPoint, float);
    void createCodecBtn(const char*, const char*, const char*, CCPoint);
    void createResBtn(const char*, const char*, ResolutionSize);
    void createToggle(const char*, const char*);
    void createResInputs(CCPoint);
    void presetResolution();

    void onToggle(CCObject*);
    void onChoosePath(CCObject*);
    void onCodec(CCObject*);
    void onResolution(CCObject*);
    void onHelp(CCObject*);
    void onApply(CCObject*);
    void onCancel(CCObject*);

    void textChanged(gd::CCTextInputNode*) override;
    void scrollWheel(float, float) override;
};