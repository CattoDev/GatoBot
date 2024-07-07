#include <Geode/Geode.hpp>

#include <core/Bot.hpp>

using namespace geode::prelude;

// I have no idea what to name this function lol
// it just replicates the behaviour of how ShaderLayer
// calculates its CCRenderTexture's width and height
// (it's the same number for W/H)
int getFunkyValue(CCSize size, int scaleFactor) {
    return static_cast<int>(sqrtf(size.width * size.width + size.height * size.height)) * scaleFactor;
}

void glViewportHook(GLint x, GLint y, GLsizei width, GLsizei height) {
    auto bot = GatoBot::get();
    auto params = bot->getRenderParams();

    if(!params->m_updateViewport) {
        auto dir = CCDirector::get();
        auto glView = CCEGLView::get();
        auto visibleSize = dir->getVisibleSize();
        auto frameSize = glView->getFrameSize();
        auto scaleFactor = static_cast<int>(dir->getContentScaleFactor());

        // calculate the mysterious ShaderLayer value
        int val = getFunkyValue(visibleSize, scaleFactor);

        if(width == val && height == val) {
            // get new design resolution
            float aspectRatio = static_cast<float>(params->m_width) / static_cast<float>(params->m_height);
            CCSize newDesignRes = { roundf(320.f * aspectRatio), 320.f };

            // calculate new funky numbers
            int newVal = getFunkyValue(newDesignRes, scaleFactor);

            width = newVal;
            height = newVal;
        }
        else {
            width = params->m_width;
            height = params->m_height;
        }
    }

    glViewport(x, y, width, height);
}

$execute {
    (void)Mod::get()->hook(
        reinterpret_cast<void*>(&glViewport),
        &glViewportHook,
        "glViewport",
        tulip::hook::TulipConvention::Cdecl
    );
}