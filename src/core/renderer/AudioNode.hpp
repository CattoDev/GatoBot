#pragma once

#include <Geode/Geode.hpp>

class AVFormatContext;
class AVCodec;
class AVCodecContext;
class AVPacket;
class AVFrame;

class AudioNode {
private:
    AVFormatContext* m_formatContext;
    const AVCodec* m_codec;
    AVCodecContext* m_codecContext;
    AVPacket* m_packet;
    AVFrame* m_frame;

    geode::Result<> m_result;

public:
    AudioNode(const std::string& path);
    ~AudioNode();
    geode::Result<> getLastResult();

    AVFrame* getFrame();
};