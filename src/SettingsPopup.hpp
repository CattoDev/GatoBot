#include "GatoBot.hpp"

struct ResolutionSize {
    int width;
    int height;
    int fps;
};

class SettingsPopup : public gd::FLAlertLayer {
public:
    std::vector<gd::CCTextInputNode*> textInputs;
    std::vector<gd::CCMenuItemToggler*> toggles;

    std::vector<const char*> codecs;
    std::vector<ResolutionSize> resolutions;
    std::vector<const char*> helpTexts;

    std::string videoPath;

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

    void createTextInput(const char*, int, int, int, const char*, CCPoint, float, const char*);
    void createHelpBtn(const char*, CCPoint, float);
    void createCodecBtn(const char*, const char*, const char*, CCPoint);
    void createResBtn(const char*, const char*, CCPoint, ResolutionSize);
    void createToggle(const char*, CCPoint, const char*);
    void createResInputs(CCPoint);

    void onToggle(CCObject*);
    void onChoosePath(CCObject*);
    void onCodec(CCObject*);
    void onResolution(CCObject*);
    void onHelp(CCObject*);
    void onApply(CCObject*);
    void onCancel(CCObject*);
};