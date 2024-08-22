/*
    Helpful sources:
    https://ffmpeg.org/doxygen/trunk/encode_video_8c-example.html
    https://github.com/apc-llc/moviemaker-cpp/blob/master/src/writer.cpp
*/

#include "Encoder.hpp"

#include <Geode/Geode.hpp>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

using namespace geode::prelude;

Encoder::Encoder(RenderParams* params) {
    geode::log::debug("Encoder::Encoder()");

    m_renderParams = params;

    // setup encoder internals
    this->setupEncoder(*params);

    // check for errors
    if(m_result.isErr()) {
        geode::log::error("{}", m_result.unwrapErr());
        return;
    }

    // render at any resolution
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_oldFBO);

    auto data = malloc(m_renderParams->m_width * m_renderParams->m_height * 3);
    memset(data, 0, m_renderParams->m_width * m_renderParams->m_height * 3);

    m_renderTexture = new CCTexture2D();
    m_renderTexture->initWithData(data, kCCTexture2DPixelFormat_RGB888, m_renderParams->m_width, m_renderParams->m_height, CCSize(static_cast<float>(m_renderParams->m_width), static_cast<float>(m_renderParams->m_height)));

    free(data);

    // allocate frame buffer
    m_frameData.resize(m_renderParams->m_width * m_renderParams->m_height * 3);

    glGetIntegerv(GL_RENDERBUFFER_BINDING, &m_oldRBO);
    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_renderTexture->getName(), 0);

    m_renderTexture->setAliasTexParameters();
    m_renderTexture->autorelease();
        
    glBindFramebuffer(GL_FRAMEBUFFER, m_oldFBO);
    glBindFramebuffer(GL_RENDERBUFFER, m_oldRBO);

    // setup done
    geode::log::debug("Encoder created.");
}

Encoder::~Encoder() {
    #define SAFE_FREE(func, var) if(var) geode::log::debug("Freeing {}", GEODE_STR(var)); func(&var);

    SAFE_FREE(av_frame_free, m_RGBFrame);
    SAFE_FREE(av_frame_free, m_YUVFrame);
    SAFE_FREE(av_frame_free, m_audioFrameBuffer);

    SAFE_FREE(avcodec_free_context, m_videoCodecContext);
    SAFE_FREE(av_packet_free, m_packet);
    SAFE_FREE(avformat_close_input, m_formatContext);

    if (m_swsContext) sws_freeContext(m_swsContext);

    CC_SAFE_RELEASE(m_renderTexture);
    CC_SAFE_DELETE(m_audioCapture);

    geode::log::debug("Encoder freed");
}

geode::Result<> Encoder::getLastResult() {
    return m_result;
}

std::vector<GLubyte>* Encoder::getFrameData() {
    return &m_frameData;
}

FMODCapture* Encoder::getFMODCapture() {
    return m_audioCapture;
}

AVFrame* Encoder::allocateAVFrame(int pixFmt, int width, int height) {
    auto frame = av_frame_alloc();
    frame->format = static_cast<AVPixelFormat>(pixFmt);
	frame->width = width;
	frame->height = height;
    frame->pts = AV_NOPTS_VALUE;
	av_frame_get_buffer(frame, 0);

    return frame;
}

void Encoder::setupEncoder(const RenderParams& params) {
    // only mp4 format
    const AVOutputFormat* outputFormat = av_guess_format("mp4", NULL, NULL);
    if(outputFormat == NULL) {
        m_result = geode::Err("Failed to get output format!");
        return;
    }

	if(avformat_alloc_output_context2(&m_formatContext, outputFormat, NULL, params.m_outputPath.c_str()) != 0) {
        m_result = geode::Err("Failed to allocate output format context! ({})", params.m_outputPath);
        return;
    }

    // find codec
    if(!(m_videoCodec = avcodec_find_encoder_by_name(params.m_codec.c_str()))) {
        m_result = geode::Err("Failed to find codec: {}", params.m_codec);
        return;
    }

    // allocate codec context
    if(!(m_videoCodecContext = avcodec_alloc_context3(m_videoCodec))) {
        m_result = geode::Err("Failed to allocate codec context!");
        return;
    }

    // set codec params
    m_videoCodecContext->width = params.m_width;
    m_videoCodecContext->height = params.m_height;
    m_videoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    m_videoCodecContext->time_base = AVRational { 1, params.m_fps };
    m_videoCodecContext->framerate = AVRational { params.m_fps, 1 };

    // set bitrate
    m_videoCodecContext->bit_rate = params.m_videoBitrate * 1000;
	m_videoCodecContext->rc_buffer_size = params.m_videoBitrate * 1000;
	m_videoCodecContext->rc_max_rate = params.m_videoBitrate * 1000;
	m_videoCodecContext->rc_min_rate = params.m_videoBitrate * 1000;

    m_videoCodecContext->thread_count = std::thread::hardware_concurrency();

    // needed for x264 to work
    m_videoCodecContext->me_range = 16;
    m_videoCodecContext->max_qdiff = 4;
    m_videoCodecContext->qmin = 10;
    m_videoCodecContext->qmax = 51;
    m_videoCodecContext->qcompress = 0.6;
    m_videoCodecContext->gop_size = params.m_fps * 2; // fps x 2
	m_videoCodecContext->max_b_frames = 3;
	m_videoCodecContext->refs = 3;

    av_opt_set(m_videoCodecContext->priv_data, "preset", "slow", 0);
    av_opt_set(m_videoCodecContext->priv_data, "crf", "10", 0);
	av_opt_set(m_videoCodecContext->priv_data, "tune", "zerolatency", 0);

    // create video stream
    if(!(m_videoStream = avformat_new_stream(m_formatContext, m_videoCodec))) {
        m_result = geode::Err("Error creating video stream!");
        return;
    }

    m_videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    m_videoStream->time_base = m_videoCodecContext->time_base;

    if(m_formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
		m_videoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

    // open codec
    if(avcodec_open2(m_videoCodecContext, m_videoCodec, NULL) != 0) {
        m_result = geode::Err("Error opening codec!");
        return;
    }

    // get params from codec ctx
    if(avcodec_parameters_from_context(m_videoStream->codecpar, m_videoCodecContext) != 0) {
        m_result = geode::Err("Error getting parameters from codec context!");
        return;
    }

    // setup audio encoder
    this->setupAudioEncoder(params);

    if(m_result.isErr()) return;

    // the
    av_dump_format(m_formatContext, 0, params.m_outputPath.c_str(), 1);

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
    m_RGBFrame = this->allocateAVFrame(2, params.m_width, params.m_height); // AV_PIX_FMT_RGB24
    m_YUVFrame = this->allocateAVFrame(0, params.m_width, params.m_height); // AV_PIX_FMT_YUV420P
    
    // allocate packet
    m_packet = av_packet_alloc();

    // get sws context
    m_swsContext = sws_getContext(params.m_width, params.m_height, AV_PIX_FMT_RGB24, params.m_width, params.m_height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    // setup FMOD capture
    this->setupFMODCapture();

    // done
    geode::log::debug("Encoder configured!");
}

static int select_channel_layout(const AVCodec *codec, AVChannelLayout *dst)
{
    const AVChannelLayout *p, *best_ch_layout;
    int best_nb_channels = 0;
 
    if (!codec->ch_layouts) {
        auto layout = AVChannelLayout AV_CHANNEL_LAYOUT_STEREO;

        return av_channel_layout_copy(dst, &layout);
    }
 
    p = codec->ch_layouts;
    while (p->nb_channels) {
        int nb_channels = p->nb_channels;
 
        if (nb_channels > best_nb_channels) {
            best_ch_layout   = p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return av_channel_layout_copy(dst, best_ch_layout);
}

static int select_sample_rate(const AVCodec *codec)
{
    const int *p;
    int best_samplerate = 0;
 
    if (!codec->supported_samplerates)
        return 44100;
 
    p = codec->supported_samplerates;
    while (*p) {
        if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
            best_samplerate = *p;
        p++;
    }
    return best_samplerate;
}

void Encoder::setupFMODCapture() {
    if(!m_renderParams->m_includeAudio) return;

    m_audioCapture = FMODCapture::get();
    m_audioCapture->setup();
}

void Encoder::setupAudioEncoder(const RenderParams& params) {
    if(!m_renderParams->m_includeAudio) return;

    if(!(m_audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC))) {
        m_result = geode::Err("Unable to open audio codec!");
        return;
    }

	if(!(m_audioCodecContext = avcodec_alloc_context3(m_audioCodec))) {
        m_result = geode::Err("Unable to allocate audio codec context!");
        return;
    }

    m_audioCodecContext->bit_rate = m_renderParams->m_audioBitrate * 1000;
    m_audioCodecContext->strict_std_compliance = -2;
    m_audioCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
 
    m_audioCodecContext->sample_fmt = AV_SAMPLE_FMT_FLTP;
    m_audioCodecContext->sample_rate = select_sample_rate(m_audioCodec);
    m_audioCodecContext->time_base = AVRational { 1, m_audioCodecContext->sample_rate };

    // select channel layout
    if(select_channel_layout(m_audioCodec, &m_audioCodecContext->ch_layout) < 0) {
        m_result = geode::Err("Failed to select audio channel layout!");
        return;
    }

    // open codec
    if(avcodec_open2(m_audioCodecContext, m_audioCodec, NULL) < 0) {
        m_result = geode::Err("Failed to open audio codec!");
        return;
    }

    if(!(m_audioStream = avformat_new_stream(m_formatContext, m_audioCodec))) {
        m_result = geode::Err("Failed to create audio stream!");
        return;
    }

    m_audioStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
	m_audioStream->time_base = m_audioCodecContext->time_base;

	if (m_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
	{
		m_audioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

    if(avcodec_open2(m_audioCodecContext, m_audioCodec, NULL) < 0) {
        m_result = geode::Err("Failed to open audio codec!");
        return;
    }

    avcodec_parameters_from_context(m_audioStream->codecpar, m_audioCodecContext);

    // setup audio frame buffer
    m_audioFrameBuffer = av_frame_alloc();

    m_audioFrameBuffer->format = m_audioCodecContext->sample_fmt;
    m_audioFrameBuffer->sample_rate = m_audioCodecContext->sample_rate;
    m_audioFrameBuffer->nb_samples = m_audioCodecContext->frame_size;
    av_channel_layout_copy(&m_audioFrameBuffer->ch_layout, &m_audioCodecContext->ch_layout);
    
    av_frame_get_buffer(m_audioFrameBuffer, 0);

    m_audioFrameBuffer->pts = 0;

    m_audioBufferSize = av_samples_get_buffer_size(NULL, m_audioCodecContext->ch_layout.nb_channels, m_audioCodecContext->frame_size, m_audioCodecContext->sample_fmt, 0) / sizeof(float);
    m_audioBuffer.resize(m_audioBufferSize);
}

void Encoder::sendFrame(AVFrame* frame, AVStream* stream, AVCodecContext* codecCtx) {
    int errCode = avcodec_send_frame(codecCtx, frame);
    if(errCode != 0) {
        char errStr[255];
        av_make_error_string(errStr, AV_ERROR_MAX_STRING_SIZE, errCode);
        m_result = geode::Err("Error sending frame! ({})", errStr);

        return;
    }

    int ret = 0;
    while(ret >= 0) {
        ret = avcodec_receive_packet(codecCtx, m_packet);

		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        	break;
		}

        if(stream) {
            m_packet->stream_index = stream->index;
        }

        av_packet_rescale_ts(m_packet, codecCtx->time_base, stream->time_base);
		ret = av_interleaved_write_frame(m_formatContext, m_packet);

        if(ret < 0) {
            m_result = geode::Err("av_interleaved_write_frame error");
            break;
        }
    }
    av_packet_unref(m_packet);
}

void Encoder::processFrameData() {
    // set frame data
    for (unsigned int y = 0; y < m_RGBFrame->height; y++) {
		for (unsigned int x = 0; x < m_RGBFrame->width; x++) {
            const int newY = m_RGBFrame->height - y - 1;

            // copy and flip image vertically
            m_RGBFrame->data[0][y * m_RGBFrame->linesize[0] + 3 * x + 0] = m_frameData[newY * m_RGBFrame->linesize[0] + 3 * x + 0];
            m_RGBFrame->data[0][y * m_RGBFrame->linesize[0] + 3 * x + 1] = m_frameData[newY * m_RGBFrame->linesize[0] + 3 * x + 1];
            m_RGBFrame->data[0][y * m_RGBFrame->linesize[0] + 3 * x + 2] = m_frameData[newY * m_RGBFrame->linesize[0] + 3 * x + 2];
		}
	}

    // convert RGB24 to YUV420P
    sws_scale(m_swsContext, m_RGBFrame->data, m_RGBFrame->linesize, 0, m_RGBFrame->height, m_YUVFrame->data, m_YUVFrame->linesize);

    m_RGBFrame->pts = m_currentFrame++;
    m_YUVFrame->pts = m_RGBFrame->pts;

    // encode RGB frame
    this->sendFrame(m_YUVFrame, m_videoStream, m_videoCodecContext);

    // set video time
    m_videoTime = (double)m_videoIdx++ * ((double)m_videoCodecContext->framerate.den / (double)m_videoCodecContext->framerate.num);
}

void Encoder::processAudio() {
    if(!m_renderParams->m_includeAudio) return;

    // huge thanks to https://stackoverflow.com/a/39693587
    while(m_audioTime < m_videoTime) {
        int loadedSize = 0;

        while(loadedSize < m_audioBufferSize) {
            auto data = m_audioCapture->capture();

            // add audio data to buffer
            const int channels = m_audioCodecContext->ch_layout.nb_channels;
            const int capturedDataSize = data.size();

            // copy data in planar order (https://stackoverflow.com/a/18889127)
            const int fullLength = m_audioBufferSize / channels;
            const int length = capturedDataSize / channels;
            for(int i = 0; i < channels; i++) {
                for(int j = 0; j < length; j++) {
                    m_audioBuffer[loadedSize / channels + i * fullLength + j] = data[j * channels + i];
                }
            }

            loadedSize += capturedDataSize;

            // allow next samples to be copied
            m_audioCapture->processed();

            // encode buffer
            if(loadedSize == m_audioBufferSize) {
                if(avcodec_fill_audio_frame(m_audioFrameBuffer, channels, m_audioCodecContext->sample_fmt, (const uint8_t*)m_audioBuffer.data(), m_audioBufferSize * channels * sizeof(float), 1) < 0) {
                    log::error("yummers");
                }

                this->sendFrame(m_audioFrameBuffer, m_audioStream, m_audioCodecContext); 

                m_audioFrameBuffer->pts += m_audioCodecContext->frame_size;

                // set audio time
                m_audioTime = (double)m_audioIdx++ * ((double)m_audioCodecContext->frame_size / (double)m_audioCodecContext->sample_rate);
            }
        }
    }
}

void Encoder::captureFrame() {
    auto dir = CCDirector::get();
    
    // set custom viewport
    glViewport(0, 0, m_renderParams->m_width, m_renderParams->m_height);

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_oldFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        
    // draw PlayLayer
    this->visit();

    // read pixels
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, m_renderParams->m_width, m_renderParams->m_height, GL_RGB, GL_UNSIGNED_BYTE, m_frameData.data());

    // reset settings
    glBindFramebuffer(GL_FRAMEBUFFER, m_oldFBO);
    
    // process video frame
    this->processFrameData();

    // process audio frame
    this->processAudio();
}

void Encoder::visit() {
    PlayLayer::get()->visit(); // TEMP

    // TODO: rewrite PlayLayer::visit to allow text on top of a rendering scene
}

void Encoder::encodingFinished() {
    log::debug("m_videoIdx: {} & m_videoTime: {}", m_videoIdx, m_videoTime);
    log::debug("m_audioIdx: {} & m_audioTime: {}", m_audioIdx, m_audioTime);

    // flush video encoder
    this->sendFrame(NULL, m_videoStream, m_videoCodecContext);

    // write file trailer
    av_write_trailer(m_formatContext);

    // stop capturing FMOD
    if(m_audioCapture) m_audioCapture->disable();

    log::debug("Encoder::encodingFinished");
}

void Encoder::levelStarted() {
    log::debug("Encoder::levelStarted");

    m_audioCapture->begin();
    //m_audioCapture->processed();
}

cocos2d::CCSize Encoder::getDesignResolution(int width, int height) {
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    return { roundf(320.f * aspectRatio), 320.f };
}