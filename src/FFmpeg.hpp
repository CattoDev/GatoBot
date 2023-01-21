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

    std::string getCommandStr() {
        std::stringstream stream;

        stream << "ffmpeg" << " -y -f rawvideo -pix_fmt rgba";
        stream << " -s " << frameWidth << "x" << frameHeight;
        stream << " -r " << fps;

        stream << " -i -"; // pipe input

        // codec
        if(extraArgs.find("-c:v") == extraArgs.npos && !codec.empty()) stream << " -c:v " << codec;

        // bitrate
        if(extraArgs.find("-b:v") == extraArgs.npos && bitrate > 0) stream << " -b:v " << bitrate << "k";

        if(extraArgs.find("-pix_fmt") == extraArgs.npos)
            stream << " -pix_fmt yuv420p";

        // video path
        std::string toAddVf = "-vf \"vflip\"";

        if(extraArgs.find(toAddVf.c_str()) == extraArgs.npos) {
            if(extraArgs.find("-vf ") != extraArgs.npos) {
                extraArgs.replace(extraArgs.find("-vf"), 3, toAddVf);
            }
            else {
                stream << " " << toAddVf;
            }
        }
        else {
            stream << " " << toAddVf;
        }

        // extra args
        if(!extraArgs.empty()) {
            stream << " " << extraArgs;
        }
        
        stream << " -an \"" << tempPath << "\""; // output

        return stream.str();
    }

    std::string getSongCmdStr() {
        std::stringstream stream;
        stream << "ffmpeg";
        stream << " -y -ss ";
        stream << songOffset;
        stream << " -i \"" << songPath << "\"";
        stream << " -i \"" << tempPath << "\"";
        stream << " -t " << time;
        stream << " -b:a " << audioBitrate << "k"; //bitrate
        stream << " -c:v copy ";
        stream << "\"" << path << "\"";

        return stream.str();
    }
};

#endif