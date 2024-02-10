#pragma once

#include <Geode/utils/Result.hpp>
#include <core/Types.hpp>

class AVCodec;
class AVCodecContext;
class AVPacket;

class Encoder {
private:
    RenderParams m_renderParams;
    const AVCodec* m_codec;
    AVCodecContext* m_codecContext;
    AVPacket* m_packet;
    geode::Result<> m_result;

public:
    Encoder(RenderParams params);
    ~Encoder();
    
    geode::Result<> getLastResult();
    void encodeFrame();
    void processFrameData();
};