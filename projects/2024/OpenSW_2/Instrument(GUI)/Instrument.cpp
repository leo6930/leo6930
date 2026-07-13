#include "Instrument.h"
#include "pch.h"
#include <iostream>
#include <fstream>


using namespace std;

Instrument::Instrument(const string& filePath)
    : fmodSystem(nullptr), sound(nullptr), channel(nullptr), soundFilePath(filePath) {
    // FMOD 시스템 초기화
    FMOD_RESULT result = FMOD::System_Create(&fmodSystem);
    if (result != FMOD_OK || !fmodSystem) {
        cerr << "FMOD 오류: 시스템 생성 실패! 오류 코드: " << static_cast<int>(result) << endl;
    }
    else {
        cerr << "디버깅: FMOD 시스템 생성 성공." << endl;
        result = fmodSystem->init(512, FMOD_INIT_NORMAL, 0);
        if (result != FMOD_OK) {
            cerr << "FMOD 오류: 시스템 초기화 실패! 오류 코드: " << static_cast<int>(result) << endl;
        }
        else {
            cerr << "디버깅: FMOD 시스템 초기화 성공." << endl;
        }
    }
}



Instrument::~Instrument() {
    if (fmodSystem) {
        if (sound) {
            FMOD_RESULT result = sound->release();
            if (result != FMOD_OK) {
                cerr << "FMOD sound release 실패! 오류 코드: " << result << endl;
            }
            sound = nullptr; 
        }

        if (fmodSystem) {
            FMOD_RESULT result = fmodSystem->close();
            if (result == FMOD_OK) {
                result = fmodSystem->release();
                if (result == FMOD_OK) {
                    cerr << "FMOD 시스템 해제 성공." << endl;
                }
            }
            fmodSystem = nullptr;
        }
    }
}




bool Instrument::initialize() {
    if (!fmodSystem) {
        std::cerr << "FMOD 오류: 시스템이 초기화되지 않았습니다." << std::endl;
        return false;
    }

  
    FMOD_OUTPUTTYPE outputType;
    fmodSystem->getOutput(&outputType);
    std::cerr << "디버깅: FMOD 출력 상태 - " << static_cast<int>(outputType) << std::endl;

    
    if (!fileExists(soundFilePath)) {
        std::cerr << "디버깅: 파일이 존재하지 않습니다 - " << soundFilePath << std::endl;
        return false;
    }

    std::cerr << "디버깅: 파일 로드 시도 - " << soundFilePath << std::endl;
    FMOD_RESULT result = fmodSystem->createSound(soundFilePath.c_str(), FMOD_DEFAULT, nullptr, &sound);
    if (result != FMOD_OK) {
        std::cerr << "FMOD 오류: 파일 로드 실패! 오류 코드: " << static_cast<int>(result) << std::endl;
        return false;
    }

    std::cerr << "디버깅: 파일 로드 성공 - " << soundFilePath << std::endl;
    return true;
}


// 사운드 재생 함수
void Instrument::play() {
    if (fmodSystem && sound) {
        FMOD_RESULT result = fmodSystem->playSound(sound, nullptr, false, &channel);
        if (result != FMOD_OK) {
            std::cerr << "FMOD 오류: 사운드 재생 실패! 오류 코드: " << static_cast<int>(result) << std::endl;
            return;
        }

        // 채널 재생 상태 확인
        bool isPlaying = false;
        if (channel) {
            channel->isPlaying(&isPlaying);
            std::cerr << "디버깅: 채널 재생 상태 - " << (isPlaying ? "재생 중" : "정지됨") << std::endl;
        }
    }
    else {
        std::cerr << "FMOD 오류: 시스템 또는 사운드가 초기화되지 않았습니다." << std::endl;
    }
}



void Instrument::stop() {
    if (channel) {
        channel->stop();
    }
}


void Instrument::update() {
    if (fmodSystem) {
        fmodSystem->update();
    }
}


void Instrument::setFilePath(const std::string& filePath) {
    soundFilePath = filePath;
}

FMOD_RESULT Instrument::playWithChannel(FMOD::Channel** channel) {
    if (fmodSystem && sound) {
        return fmodSystem->playSound(sound, nullptr, false, channel);
    }
    return FMOD_ERR_INVALID_HANDLE; 
}


bool Instrument::fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}