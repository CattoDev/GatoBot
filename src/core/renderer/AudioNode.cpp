#include "AudioNode.hpp"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

using namespace geode::prelude;

AudioNode::AudioNode(const std::string& path) {
    // get format context
    m_formatContext = avformat_alloc_context();

    if(avformat_open_input(&m_formatContext, path.c_str(), NULL, NULL) < 0) {
        m_result = geode::Err("Could not open input file: {}", path);
        return;
    }

    avformat_find_stream_info(m_formatContext, NULL);

    auto codecParams = m_formatContext->streams[0]->codecpar;

    // find codec
    if(!(m_codec = avcodec_find_decoder(codecParams->codec_id))) {
        m_result = geode::Err("Could not find decoder: {}", (int)codecParams->codec_id);
        return;
    }

    // get codec context
    if(!(m_codecContext = avcodec_alloc_context3(m_codec))) {
        m_result = geode::Err("Could not allocate codec context for audio: {}", path);
        return;
    }

    // copy input codec params
    if (avcodec_parameters_to_context(m_codecContext, codecParams) < 0) {
        m_result = geode::Err("Failed to copy codec parameters!");
        return;
    }

    // open the codec
    if (avcodec_open2(m_codecContext, m_codec, NULL) < 0) {
        m_result = geode::Err("Could not open codec!");
        return;
    }

    // set the packet timebase for the decoder
    m_codecContext->pkt_timebase = m_formatContext->streams[0]->time_base;

    // create packet
    m_packet = av_packet_alloc();

    // create frame
    m_frame = av_frame_alloc();
}

AudioNode::~AudioNode() {
    avformat_close_input(&m_formatContext);
    avcodec_free_context(&m_codecContext);
    av_packet_free(&m_packet);
    av_frame_free(&m_frame);
}

geode::Result<> AudioNode::getLastResult() {
    return m_result;
}

AVFrame* AudioNode::getFrame() {
    av_init_packet(m_packet);
    m_packet->data = NULL;
    m_packet->size = 0;

    int ret = 0;
    if((ret = av_read_frame(m_formatContext, m_packet)) < 0) {
        if(ret != AVERROR_EOF) {
            log::error("Could not read frame!");
        }
    }

    // decode frame (send the packet with the compressed data to the decoder)
    if (avcodec_send_packet(m_codecContext, m_packet) < 0) {
        m_result = geode::Err("Error submitting the packet to the decoder!");
        return nullptr;
    }

    // read all the output frames (in general there may be any number of them
    ret = avcodec_receive_frame(m_codecContext, m_frame);

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return nullptr;
    }
    else if (ret < 0) {
        m_result = geode::Err("Error during decoding!");
        return nullptr;
    }
    
    int data_size = av_get_bytes_per_sample(m_codecContext->sample_fmt);
    if (data_size < 0) {
        // This should not occur, checking just for paranoia
        m_result = geode::Err("Failed to calculate data size!");
        return nullptr;
    }

    return m_frame;
}