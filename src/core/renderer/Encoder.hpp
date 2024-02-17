#pragma once

#include <Geode/utils/Result.hpp>
#include <Geode/cocos/platform/win32/CCGL.h>
#include <core/Types.hpp>

class AVCodec;
class AVCodecContext;
class AVPacket;
class AVFrame;
class AVFormatContext;
enum AVPixelFormat;
class AVStream;
class SwsContext;

class Encoder {
private:
    RenderParams m_renderParams;

    const AVCodec* m_codec;
    AVCodecContext* m_codecContext;
    AVPacket* m_packet;
    AVFrame* m_RGBframe;
    AVFrame* m_YUVframe;
    AVFormatContext* m_formatContext;
    AVStream* m_videoStream;
    SwsContext* m_swsContext;

    //FILE* m_outFile;
    
    geode::Result<> m_result;
    std::vector<GLubyte> m_frameData;
    cocos2d::CCTexture2D* m_renderTexture;
    int m_currentFrame = 0;

    GLint m_oldFBO;
    GLuint m_FBO;
    GLint m_oldRBO;

private:
    AVFrame* allocateAVFrame(AVPixelFormat pixFmt, int width, int height);
    void setupEncoder(const RenderParams& params);
    void encodeFrame(AVFrame*);

public:
    Encoder(RenderParams params);
    ~Encoder();
    
    geode::Result<> getLastResult();
    std::vector<GLubyte>* getFrameData();
    void processFrameData();
    void captureCurrentFrame();
    void encodingFinished();
};