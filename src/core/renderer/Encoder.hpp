#pragma once

#include <Geode/utils/Result.hpp>
#include <Geode/cocos/platform/win32/CCGL.h>
#include <Geode/cocos/cocoa/CCGeometry.h>
#include <core/Types.hpp>

#include "AudioNode.hpp"

class AVCodec;
class AVCodecContext;
class AVPacket;
class AVFrame;
class AVFormatContext;
class AVStream;
class SwsContext;

class Encoder {
private:
    RenderParams* m_renderParams;

    const AVCodec* m_videoCodec;
    const AVCodec* m_audioCodec;
    AVCodecContext* m_videoCodecContext = nullptr;
    AVCodecContext* m_audioCodecContext = nullptr;
    AVPacket* m_packet = nullptr;
    AVFrame* m_RGBframe = nullptr;
    AVFrame* m_YUVframe = nullptr;
    AVFrame* m_audioFrameBuffer = nullptr;
    AVFormatContext* m_formatContext = nullptr;
    AVStream* m_videoStream = nullptr;
    AVStream* m_audioStream = nullptr;
    SwsContext* m_swsContext = nullptr;

    double m_videoTime = 0;
    double m_audioTime = 0;
    int m_videoIdx = 1;
    int m_audioIdx = 1;
    
    geode::Result<> m_result;
    std::vector<GLubyte> m_frameData;
    cocos2d::CCTexture2D* m_renderTexture = nullptr;
    int m_currentFrame = 0;
    std::vector<AudioNode*> m_audioNodes;

    GLint m_oldFBO;
    GLuint m_FBO;
    GLint m_oldRBO;

private:
    AVFrame* allocateAVFrame(int pixFmt, int width, int height);
    void setupEncoder(const RenderParams& params);
    void setupAudioEncoder(const RenderParams& params);
    void setupAudioDecoder(const RenderParams& params);
    void sendFrame(AVFrame* frame, AVStream* stream, AVCodecContext* codecCtx, bool video);

public:
    Encoder(RenderParams* params);
    ~Encoder();
    
    geode::Result<> getLastResult();
    std::vector<GLubyte>* getFrameData();
    void processFrameData();
    void processAudio();
    void captureFrame();
    void visit();
    void encodingFinished();

    static cocos2d::CCSize getDesignResolution(int width, int height);
};