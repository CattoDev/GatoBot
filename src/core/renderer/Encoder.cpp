#include "Encoder.hpp"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
}

Encoder::Encoder(RenderParams params) {
    m_renderParams = params;

    if(!(m_codec = avcodec_find_encoder_by_name(params.m_codec))) {
        m_result = geode::Err("Failed to find codec: {}", params.m_codec);
        return;
    }

    if(!(m_codecContext = avcodec_alloc_context3(m_codec))) {
        m_result = geode::Err("Unable to allocate codec context!");
        return;
    }

    if(!(m_packet = av_packet_alloc())) {
        m_result = geode::Err("Unable to allocate packet!");
        return;
    }

    // settings
    m_codecContext->bit_rate = 40000;
    m_codecContext->width = m_renderParams.m_width;
    m_codecContext->height = m_renderParams.m_height;

    // FPS
    m_codecContext->time_base = AVRational {1, m_renderParams.m_fps};
    m_codecContext->framerate = AVRational {m_renderParams.m_fps, 1};

    // idk yet
    m_codecContext->gop_size = 10;
    m_codecContext->max_b_frames = 1;
    m_codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    if(m_codec->id == AV_CODEC_ID_H264) {
        av_opt_set(m_codecContext->priv_data, "preset", "slow", 0);
    }

    // create file
    if (int ret = avcodec_open2(m_codecContext, m_codec, NULL) < 0) {
        m_result = geode::Err("Could not open codec!");
        return;
    }
 
    auto file = fopen(params.m_outputPath.c_str(), "wb");
    if(!file) {
        m_result = geode::Err("Could not open output file: {}", params.m_outputPath);
        return;
    }
 
    auto frame = av_frame_alloc();
    if (!frame) {
        m_result = geode::Err("Could not allocate video frame!");
        return;
    }
    frame->format = m_codecContext->pix_fmt;
    frame->width  = m_codecContext->width;
    frame->height = m_codecContext->height;
 
    if(int ret = av_frame_get_buffer(frame, 0) < 0) {
        m_result = geode::Err("Could not allocate the video frame data!");
        return;
    }

    geode::log::debug("Encoder created.");
}

Encoder::~Encoder() {
    CC_SAFE_DELETE(m_codec);
    CC_SAFE_DELETE(m_codecContext);
    CC_SAFE_DELETE(m_packet);
}

geode::Result<> Encoder::getLastResult() {
    return m_result;
}

void Encoder::encodeFrame() {

}

void Encoder::processFrameData() {

}
