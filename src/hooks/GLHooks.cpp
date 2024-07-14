#include <Geode/Geode.hpp>

#include <core/Bot.hpp>

using namespace geode::prelude;

void glViewportHook(GLint x, GLint y, GLsizei width, GLsizei height) {
    auto bot = GatoBot::get();
    auto params = bot->getRenderParams();

    // something somewhere is still setting the 
    // wrong resolution and I'm too lazy to find
    // where the fuck the problem is
    if(!params->m_updateViewport) {
        if(width != height) {
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