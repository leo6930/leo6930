#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include <string>
#include "fmod.hpp"

class Instrument {
private:
    FMOD::System* fmodSystem;
    FMOD::Sound* sound;
    FMOD::Channel* channel;
    std::string soundFilePath;
    bool fileExists(const std::string& path);

public:
    Instrument(const std::string& filePath);
    ~Instrument();

    bool initialize();
    void play();
    void stop();
    void update();

    // 파일 경로 설정 함수 추가
    void setFilePath(const std::string& filePath);


    FMOD_RESULT playWithChannel(FMOD::Channel** channel);
};

#endif // INSTRUMENT_H
