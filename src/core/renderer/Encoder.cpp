#include "Encoder.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// https://ffmpeg.org/doxygen/trunk/encode_video_8c-example.html
// https://github.com/apc-llc/moviemaker-cpp/blob/master/src/writer.cpp
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

Encoder::Encoder(RenderParams params) {
    m_renderParams = params;

    // setup encoder internals
    this->setupEncoder(params);

    // check for errors
    if(m_result.isErr()) {
        geode::log::error("{}", m_result.unwrapErr());
        return;
    }

    // render at higher resolution
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &m_oldFBO);

    auto data = malloc(m_renderParams.m_width * m_renderParams.m_height * 3);
    memset(data, 0, m_renderParams.m_width * m_renderParams.m_height * 3);

    m_renderTexture = new CCTexture2D();
    m_renderTexture->initWithData(data, kCCTexture2DPixelFormat_RGB888, m_renderParams.m_width, m_renderParams.m_height, CCSize(static_cast<float>(m_renderParams.m_width), static_cast<float>(m_renderParams.m_height)));

    free(data);

    // allocate frame buffer
    m_frameData.resize(m_renderParams.m_width * m_renderParams.m_height * 3);

    glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &m_oldRBO);
    glGenFramebuffersEXT(1, &m_FBO);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_FBO);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_renderTexture->getName(), NULL);

    m_renderTexture->setAliasTexParameters();
    m_renderTexture->autorelease();
        
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_oldFBO);
    glBindFramebufferEXT(GL_RENDERBUFFER_EXT, m_oldRBO);

    // setup done
    geode::log::debug("Encoder created.");
}

Encoder::~Encoder() {
    av_frame_free(&m_RGBframe);
    av_frame_free(&m_YUVframe);

    avcodec_free_context(&m_codecContext);
    av_packet_free(&m_packet);
    avformat_close_input(&m_formatContext);
    sws_freeContext(m_swsContext);

    CC_SAFE_RELEASE(m_renderTexture);
}

geode::Result<> Encoder::getLastResult() {
    return m_result;
}

std::vector<GLubyte>* Encoder::getFrameData() {
    return &m_frameData;
}

AVFrame* Encoder::allocateAVFrame(AVPixelFormat pixFmt, int width, int height) {
    auto frame = av_frame_alloc();
    frame->format = pixFmt;
	frame->width = width;
	frame->height = height;
    frame->pts = AV_NOPTS_VALUE;
	av_frame_get_buffer(frame, 0);

    return frame;
}

void Encoder::setupEncoder(const RenderParams& params) {
    // only mp4 format
    const AVOutputFormat * outputFormat = av_guess_format("mp4", NULL, NULL);
	if(avformat_alloc_output_context2(&m_formatContext, outputFormat, NULL, params.m_outputPath.c_str()) != 0) {
        m_result = geode::Err("Failed to allocate output context!");
        return;
    }

    // find codec
    if(!(m_codec = avcodec_find_encoder_by_name(params.m_codec))) {
        m_result = geode::Err("Failed to find codec: {}", params.m_codec);
        return;
    }

    // allocate codec context
    if(!(m_codecContext = avcodec_alloc_context3(m_codec))) {
        m_result = geode::Err("Failed to allocate codec context!");
        return;
    }

    // set codec params
    m_codecContext->width = params.m_width;
    m_codecContext->height = params.m_height;
    m_codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    m_codecContext->time_base = AVRational { 1, params.m_fps };
    m_codecContext->framerate = AVRational { params.m_fps, 1 };

    // Set Bitrate
	m_codecContext->bit_rate = 5 * 1024 * 1024;
	m_codecContext->rc_buffer_size = 4 * 1000 * 1000;
	m_codecContext->rc_max_rate = 5 * 1024 * 1024;
	m_codecContext->rc_min_rate = 5 * 1024 * 1024;

    // needed for x264 to work
    m_codecContext->me_range = 16;
    m_codecContext->max_qdiff = 4;
    m_codecContext->qmin = 10;
    m_codecContext->qmax = 51;
    m_codecContext->qcompress = 0.6;
    m_codecContext->gop_size = params.m_fps * 2; // fps x 2
	m_codecContext->max_b_frames = 3;
	m_codecContext->refs = 3;

    av_opt_set(m_codecContext->priv_data, "preset", "slow", 0);
    av_opt_set(m_codecContext->priv_data, "crf", "35", 0);
	av_opt_set(m_codecContext->priv_data, "tune", "zerolatency", 0);

    // create video stream
    if(!(m_videoStream = avformat_new_stream(m_formatContext, m_codec))) {
        m_result = geode::Err("Error creating video stream!");
        return;
    }

    m_videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    m_videoStream->time_base = m_codecContext->time_base;

    if(m_formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
		m_codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

    av_dump_format(m_formatContext, 0, params.m_outputPath.c_str(), 1);

    // open codec
    if(avcodec_open2(m_codecContext, m_codec, NULL) != 0) {
        m_result = geode::Err("Error opening codec!");
        return;
    }

    // get params from codec ctx
    if(avcodec_parameters_from_context(m_videoStream->codecpar, m_codecContext) != 0) {
        m_result = geode::Err("Error getting parameters from codec context!");
        return;
    }

    // open output file
	if(avio_open2(&m_formatContext->pb, params.m_outputPath.c_str(), AVIO_FLAG_WRITE, NULL, NULL) != 0) {
        m_result = geode::Err("Failed to open output file!");
        return;
    }

    // write file header
    if(avformat_write_header(m_formatContext, NULL) != 0) {
        m_result = geode::Err("Failed to write file header!");
        return;
    }

    // allocate frames
    m_RGBframe = this->allocateAVFrame(AV_PIX_FMT_RGB24, params.m_width, params.m_height);
    m_YUVframe = this->allocateAVFrame(AV_PIX_FMT_YUV420P, params.m_width, params.m_height);

    // allocate packet
    m_packet = av_packet_alloc();

    // get sws context
    m_swsContext = sws_getContext(params.m_width, params.m_height, AV_PIX_FMT_RGB24, params.m_width, params.m_height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    // done
    geode::log::debug("Encoder configured!");
}

void Encoder::encodeFrame(AVFrame* frame) {
    if(frame) {
        geode::log::debug("Encoding frame: {}", frame->pts);

        // convert RGB24 to YUV420P
        if(frame->format == AV_PIX_FMT_RGB24) {
            sws_scale(m_swsContext, frame->data, frame->linesize, 0, frame->height, m_YUVframe->data, m_YUVframe->linesize);

            m_YUVframe->pts = frame->pts;
            frame = m_YUVframe;
        }
    }

    if(avcodec_send_frame(m_codecContext, frame) != 0) {
        m_result = geode::Err("Error sending frame!");
        return;
    }

    int ret = 0;
    while(ret >= 0) {
        ret = avcodec_receive_packet(m_codecContext, m_packet);

		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        	break;
		}

		m_packet->flags |= AV_PKT_FLAG_KEY;
		//packet->pts = packet->dts = 0;

		//packet->stream_index = frameIdx;
		//frameIdx++;
    	//packet->duration = codecCtx->time_base.den / codecCtx->time_base.num; // dec_video_avs->avg_frame_rate.num * dec_video_avs->avg_frame_rate.den;

        av_packet_rescale_ts(m_packet, m_codecContext->time_base, m_videoStream->time_base);

		ret = av_interleaved_write_frame(m_formatContext, m_packet);
    }
    av_packet_unref(m_packet);
}

void Encoder::processFrameData() {
    geode::log::debug("{} & {}", m_RGBframe->linesize[0] * m_RGBframe->height, m_frameData.size());

    // TEMP CODE I WILL CHANGE I PROMISE

    // set frame data
    for (unsigned int y = 0; y < m_RGBframe->height; y++)
	{
		for (unsigned int x = 0; x < m_RGBframe->width; x++)
		{
            const int newY = m_RGBframe->height - y - 1;

            // copy and flip image vertically
            m_RGBframe->data[0][y * m_RGBframe->linesize[0] + 3 * x + 0] = m_frameData[newY * m_RGBframe->linesize[0] + 3 * x + 0];
            m_RGBframe->data[0][y * m_RGBframe->linesize[0] + 3 * x + 1] = m_frameData[newY * m_RGBframe->linesize[0] + 3 * x + 1];
            m_RGBframe->data[0][y * m_RGBframe->linesize[0] + 3 * x + 2] = m_frameData[newY * m_RGBframe->linesize[0] + 3 * x + 2];
		}
	}

    m_RGBframe->pts = m_currentFrame++;

    geode::log::debug("Frame write PTS {}", m_RGBframe->pts);

    // encode RGB frame
    this->encodeFrame(m_RGBframe);
}

void Encoder::captureCurrentFrame() {
    // (stolen from CCRenderTexture lmao)
    // set viewport of custom res
    glViewport(0, 0, m_renderParams.m_width, m_renderParams.m_height);

    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &m_oldFBO);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_FBO);
        
    //visitPlayLayer(); // draw PlayLayer
    PlayLayer::get()->visit();

    // read pixels
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, m_renderParams.m_width, m_renderParams.m_height, GL_RGB, GL_UNSIGNED_BYTE, m_frameData.data());

    // reset viewport
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_oldFBO);
    CCDirector::sharedDirector()->setViewport();

    // process frame
    this->processFrameData();
}

void Encoder::encodingFinished() {
    // flush encoder
    this->encodeFrame(NULL);

    // write file trailer
    av_write_trailer(m_formatContext);

    log::debug("Encoder::encodingFinished");
}
