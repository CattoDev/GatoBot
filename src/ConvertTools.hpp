#ifndef _FFMPEG_WRAPPER_
#define _FFMPEG_WRAPPER_

#include <iostream>
#include <vector>

class GBConvertTools {
public:
    static int MH_HexToInt(std::vector<unsigned char> hexValArr) {
        int num = 0;
        
        for(int i = 0; i < hexValArr.size(); i++) {
            num |= hexValArr[i] << i * 8;
        }

        return num;
    }

    static float MH_HexToFloat(std::vector<char> hexValArr) {
        char tempArr[sizeof(float)];

        for(int i = 0; i < hexValArr.size(); i++) {
            tempArr[i] = hexValArr[i];
        }

        return reinterpret_cast<float&>(tempArr);
    }

    static double MH_HexToDouble(std::vector<char> hexValArr) {
        char tempArr[sizeof(double)];

        for(int i = 0; i < hexValArr.size(); i++) {
            tempArr[i] = hexValArr[i];
        }

        return reinterpret_cast<double&>(tempArr);
    }

    static std::vector<std::string> _splitString(std::string stringData, char delimiter) {
        size_t index = 0;
        size_t nextIndex = stringData.find(delimiter);
        std::string subStr;

        std::vector<std::string> dataVec;

        while(true) {
            subStr = stringData.substr(index, nextIndex - index);

            dataVec.push_back(subStr);

            if(nextIndex == stringData.npos) break;

            // continue
            index = nextIndex + 1;
            nextIndex = stringData.find(delimiter, index);
        }

        return dataVec;
    }
};

#endif