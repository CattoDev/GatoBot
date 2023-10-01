#ifndef _FFMPEG_WRAPPER_
#define _FFMPEG_WRAPPER_

#include <sstream>
#include <string>

struct FFmpegSettings {
    int frameWidth;
    int frameHeight;
    int fps;
    int bitrate;
    int audioBitrate;

    float songOffset;
    double time;

    std::string codec;
    std::string tempPath;
    std::string path;
    std::string songPath;

    std::string extraArgs;

    std::string getCommandStr();
    std::string getSongCmdStr();
};

#endif