#pragma once

#include <Geode/utils/Result.hpp>
#include <Geode/cocos/platform/win32/CCGL.h>
#include <core/Types.hpp>

#include "AudioNode.hpp"

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

    const AVCodec* m_videoCodec;
    const AVCodec* m_audioCodec;
    AVCodecContext* m_videoCodecContext;
    AVCodecContext* m_audioCodecContext;
    AVPacket* m_packet;
    AVFrame* m_RGBframe;
    AVFrame* m_YUVframe;
    AVFrame* m_audioFrameBuffer;
    AVFormatContext* m_formatContext;
    AVStream* m_videoStream;
    AVStream* m_audioStream;
    SwsContext* m_swsContext;

    double m_videoTime = 0;
    double m_audioTime = 0;
    int m_videoIdx = 1;
    int m_audioIdx = 1;
    
    geode::Result<> m_result;
    std::vector<GLubyte> m_frameData;
    cocos2d::CCTexture2D* m_renderTexture;
    int m_currentFrame = 0;
    std::vector<AudioNode*> m_audioNodes;

    GLint m_oldFBO;
    GLuint m_FBO;
    GLint m_oldRBO;

private:
    AVFrame* allocateAVFrame(AVPixelFormat pixFmt, int width, int height);
    void setupEncoder(const RenderParams& params);
    void setupAudioEncoder(const RenderParams& params);
    void setupAudioDecoder(const RenderParams& params);
    void sendFrame(AVFrame* frame, AVStream* stream, AVCodecContext* codecCtx, bool video);

public:
    Encoder(RenderParams params);
    ~Encoder();
    
    geode::Result<> getLastResult();
    std::vector<GLubyte>* getFrameData();
    void processFrameData();
    void processAudio();
    void captureCurrentFrame();
    void encodingFinished();
};