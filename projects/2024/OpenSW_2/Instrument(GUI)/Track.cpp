#include "Track.h"
#include "Scale.h"
#include "CInstrumentDlg.h" // CInstrumentDlg 클래스 선언 포함
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <ctime>
#include "pch.h"
#include <fmod.hpp>
#include <algorithm>


using namespace std;


Track::Track(int bpm, const string& key, int timeSignature)
    : bpm(bpm), key(key), timeSignature(timeSignature), isPlaying(false), outputFunc(nullptr) {}


void Track::setOutputFunction(function<void(const CString&)> func) {
    outputFunc = func;
}


void Track::setBPM(int newBPM) {
    lock_guard<mutex> lock(mtx);
    bpm = newBPM;
}


void Track::setTimeSignature(int newTimeSignature) {
    lock_guard<mutex> lock(mtx);
    timeSignature = newTimeSignature;
}

void Track::setScale(const Scale& scale) {
    key = scale.getRoot();
    backingTrackChords = scale.getNotes();
}


void Track::generateMainTrackNotes(const Scale& scale) {
    vector<string> scaleNotes = scale.getNotes();

    if (scaleNotes.empty()) {
        throw std::runtime_error("스케일이 설정되지 않았습니다. 메인 트랙을 생성할 수 없습니다.");
    }

    mainTrackNotes.clear();
    srand(static_cast<unsigned int>(time(0)));

    int scaleSize = scaleNotes.size();
    int currentNoteIndex = rand() % scaleSize; 
    mainTrackNotes.push_back(scaleNotes[currentNoteIndex]);

   
    for (int i = 1; i < 8; ++i) {
        int nextNoteIndex;

        
        do {
            int step = (rand() % 3) - 1; 
            nextNoteIndex = currentNoteIndex + step;

            
            if (nextNoteIndex < 0) nextNoteIndex += scaleSize;
            if (nextNoteIndex >= scaleSize) nextNoteIndex -= scaleSize;
        } while (nextNoteIndex == currentNoteIndex); 

        mainTrackNotes.push_back(scaleNotes[nextNoteIndex]);
        currentNoteIndex = nextNoteIndex;
    }

  
    cerr << " 메인 트랙 단음 진행: ";
    for (const auto& note : mainTrackNotes) {
        cerr << note << " ";
    }
    cerr << endl;
}







void Track::generateBackingTrack(const Scale& scale, int comboIndex) {
    vector<string> scaleNotes = scale.getNotes();

    if (scaleNotes.empty()) {
        throw std::runtime_error("스케일이 설정되지 않았습니다. 백킹 트랙을 생성할 수 없습니다.");
    }

    
    vector<vector<int>> noteProgressions;

    if (comboIndex == 0 || comboIndex == 2) { // Major 계열
        noteProgressions = {
            {0, 2, 4, 5},  
            {0, 4, 5, 7}, 
            {0, 2, 5, 4},  
            {0, 4, 2, 0}  
        };
    }
    else if (comboIndex == 1 || comboIndex == 3) { // Minor 계열
        noteProgressions = {
            {0, 3, 5, 7},  
            {0, 3, 5, 4},  
            {0, 5, 3, 7},  
            {0, 3, 5, 0}   
        };
    }

    
    int patternIndex = rand() % noteProgressions.size();
    vector<int> selectedProgression = noteProgressions[patternIndex];

    backingTrackChords.clear();

    
    for (int repeat = 0; repeat < 4; ++repeat) {
        for (int noteIndex : selectedProgression) {
            int idx = noteIndex % scaleNotes.size();
            if (idx >= 0 && idx < scaleNotes.size()) {
                backingTrackChords.push_back(scaleNotes[idx]);
            }
        }
    }

    
    cerr << "디버깅: 생성된 백킹 트랙 노트 진행: ";
    for (const auto& note : backingTrackChords) {
        cerr << note << " ";
    }
    cerr << endl;
}









void Track::generateDrumPattern() {
    if (timeSignature == 4) { // 4/4 박자
        drumPattern = {
            "Kick", "HiHatClosed", "Snare", "HiHatOpen",
            "Kick", "HiHatClosed", "Snare", "Crash1"
        };
    }
    else if (timeSignature == 3) { // 3/4 박자
        drumPattern = {
            "Kick", "Snare", "HiHatOpen",
            "TomLow", "TomMid", "Crash1"
        };
    }
    else {
        throw std::invalid_argument("지원되지 않는 박자입니다: " + to_string(timeSignature));
    }
}



vector<string> Track::getBackingTrackChords() const {
    return backingTrackChords;
}


vector<string> Track::getMainTrackNotes() const {
    return mainTrackNotes;
}


string Track::getBackingTrackChordsAsString() const {
    string result;
    for (const auto& chord : backingTrackChords) {
        result += chord + " ";
    }
    return result;
}


string Track::getMainTrackNotesAsString() const {
    string result;
    for (const auto& note : mainTrackNotes) {
        result += note + " ";
    }
    return result;
}


string Track::getDrumPatternAsString() const {
    string result;
    for (const auto& beat : drumPattern) {
        result += beat + " ";
    }
    return result;
}


void Track::playDrumTrack(Instrument& drumInstrument) {
    
    if (drumPlaybackThread.joinable()) {
        drumPlaybackThread.join();
    }

   
    drumPlaybackThread = std::thread(&Track::playDrumTrackThreaded, this, std::ref(drumInstrument));
}






void Track::playBackingTrack(Instrument& pianoInstrument, const Scale& scale) {
    
    if (playbackThread.joinable()) {
        playbackThread.join();
    }

    
    playbackThread = std::thread([this, &pianoInstrument, &scale]() {
        isPlaying = true; 
        int duration = 60000 / bpm; 

        try {
            for (const auto& chord : backingTrackChords) {
                if (!isPlaying) break;

                
                vector<string> chordNotes = scale.generateTriadChord(chord);
                vector<FMOD::Channel*> currentChordChannels;

                
                for (const auto& note : chordNotes) {
                    pianoInstrument.setFilePath("sounds/piano/" + note + ".mp3");
                    if (pianoInstrument.initialize()) {
                        FMOD::Channel* channel = nullptr;
                        pianoInstrument.playWithChannel(&channel);
                        channel->setVolume(0.7f); 
                        currentChordChannels.push_back(channel);
                    }
                }

                
                std::this_thread::sleep_for(std::chrono::milliseconds(duration));

                for (auto& channel : currentChordChannels) {
                    if (channel) channel->stop();
                }
            }
        }
        catch (const std::exception& e) {
            cerr << "백킹 트랙 재생 중 예외 발생: " << e.what() << endl;
        }

        isPlaying = false; 
        });
}



void Track::playTrackWithTiming(CInstrumentDlg* dlg) {
    double beatDuration = 60000.0 / bpm;

    for (size_t i = 0; i < mainTrackNotes.size(); ++i) {
        lock_guard<mutex> lock(mtx);
        if (!isPlaying) break; 

        CString output;
        output.Format(_T("메인 트랙 음: %s\r\n"), CString(mainTrackNotes[i].c_str()));
        if (outputFunc) {
            outputFunc(output);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(beatDuration)));
    }
}



int Track::getBPM() const {
    return bpm;
}

void Track::stopAllPlayback() {
    lock_guard<mutex> lock(mtx);
    isPlaying = false;

   
    for (auto& channel : drumChannels) {
        if (channel) channel->stop();
    }
    drumChannels.clear();

   
    for (auto& channel : pianoChannels) {
        if (channel) channel->stop();
    }
    pianoChannels.clear();
}


void Track::playBackingTrackThreaded(Instrument& pianoInstrument, const Scale& scale) {
    isPlaying = true; 
    int duration = 60000 / bpm; 

    try {
        for (const auto& chord : backingTrackChords) {
            if (!isPlaying) break; 

            
            vector<string> chordNotes = scale.generateTriadChord(chord);

            
            if (outputFunc) {
                CString message;
                message.Format(_T("백킹 트랙 재생 중: 코드 %s (화음: "), CString(chord.c_str()));
                for (const auto& note : chordNotes) {
                    message += CString(note.c_str()) + _T(" ");
                }
                message += _T(")\r\n");
                outputFunc(message);
            }

            
            for (const auto& note : chordNotes) {
                if (!isPlaying) break;

                pianoInstrument.setFilePath("sounds/piano/" + note + ".mp3");
                if (pianoInstrument.initialize()) {
                    pianoInstrument.play();
                    std::this_thread::sleep_for(std::chrono::milliseconds(duration / chordNotes.size()));
                    pianoInstrument.stop();
                }
                else {
                    cerr << "디버깅: 음원 초기화 실패 - " << note << endl;
                }
            }
        }
    }
    catch (const std::exception& e) {
        cerr << "재생 중 예외 발생: " << e.what() << endl;
    }

    isPlaying = false; 
}


Track::~Track() {
    if (playbackThread.joinable()) {
        stopAllPlayback();
        playbackThread.join();
    }

    isPlaying = true;

    if (drumPlaybackThread.joinable()) {
        drumPlaybackThread.join(); 
    }
}


void Track::playDrumTrackThreaded(Instrument& drumInstrument) {
    isPlaying = true; 
    int beatDuration = 60000 / bpm; 
    std::mutex fmodMutex; 

    try {
        for (int repeat = 0; repeat < 4 && isPlaying; ++repeat) { 
            for (const auto& drumSound : drumPattern) {
                if (!isPlaying) break; 

                
                string drumFileName = "sounds/drum/" + drumSound + ".wav";
                cerr << "디버깅: 드럼 파일 경로 - " << drumFileName << endl;
                drumInstrument.setFilePath(drumFileName);

                {
                    lock_guard<mutex> lock(fmodMutex); 
                    bool initialized = drumInstrument.initialize(); 
                    if (!initialized) {
                        cerr << "디버깅: 드럼 파일 초기화 실패 - " << drumFileName << endl;
                        continue; 
                    }
                    else {
                        cerr << "디버깅: 드럼 파일 초기화 성공 - " << drumFileName << endl;
                    }

                   
                    FMOD::Channel* channel = nullptr;
                    FMOD_RESULT result = drumInstrument.playWithChannel(&channel);
                    if (result != FMOD_OK) {
                        cerr << "디버깅: 드럼 파일 재생 실패 - "
                            << drumFileName << ", 오류 코드: "
                            << static_cast<int>(result) << endl;
                        continue;
                    }
                    else {
                        cerr << "디버깅: 드럼 파일 재생 성공 - " << drumFileName << endl;
                    }

                   
                    bool isChannelPlaying = true;
                    while (isChannelPlaying) {
                        channel->isPlaying(&isChannelPlaying);
                        std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
                    }
                    cerr << "디버깅: 드럼 파일 재생 완료 - " << drumFileName << endl;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(beatDuration / drumPattern.size()));
            }
        }
    }
    catch (const std::exception& e) {
        cerr << "드럼 재생 중 예외 발생: " << e.what() << endl;
    }

    isPlaying = false; 
}

void Track::playGuitarTrack(Instrument& guitarInstrument, const Scale& scale) {
    if (mainTrackNotes.empty()) {
        cerr << "오류: 메인 트랙 노트가 비어 있습니다. 기타를 재생할 수 없습니다." << endl;
        return;
    }

    isPlaying = true; 
    int beatDuration = 60000 / bpm; 

    size_t noteIndex = 0; 

    try {
        for (int i = 0; i < mainTrackNotes.size() && isPlaying; ++i) {
            
            string note = mainTrackNotes[noteIndex];
            string guitarSound = "sounds/guitar/" + note + ".wav";

            cerr << "디버깅: 기타 파일 경로 - " << guitarSound << endl;

            
            guitarInstrument.setFilePath(guitarSound);
            if (guitarInstrument.initialize()) {
                FMOD::Channel* channel = nullptr;
                guitarInstrument.playWithChannel(&channel);

                
                std::this_thread::sleep_for(std::chrono::milliseconds(beatDuration));

                
                if (channel) channel->stop();
            }
            else {
                cerr << "디버깅: 기타 파일 초기화 실패 - " << guitarSound << endl;
            }

            
            noteIndex = (noteIndex + 1) % mainTrackNotes.size();
        }
    }
    catch (const std::exception& e) {
        cerr << "기타 재생 중 예외 발생: " << e.what() << endl;
    }

    isPlaying = false; 
}


void Track::playSynchronizedTrack(Instrument& pianoInstrument, Instrument& drumInstrument, Instrument& guitarInstrument, const Scale& scale) {
    if (backingTrackChords.empty()) {
        cerr << "오류: 백킹 트랙 코드가 비어 있습니다." << endl;
        return;
    }

    isPlaying = true;
    int beatDuration = 60000 / bpm; 
    size_t drumIndex = 0;
    size_t drumSize = drumPattern.size();
    size_t guitarNoteIndex = 0;

    try {
        for (int measure = 0; measure < 4; ++measure) { 
            for (int beat = 0; beat < timeSignature; ++beat) { 
                if (!isPlaying) break; 

               
                string drumSound = drumPattern[drumIndex];
                drumInstrument.setFilePath("sounds/drum/" + drumSound + ".wav");
                if (drumInstrument.initialize()) {
                    FMOD::Channel* drumChannel = nullptr;
                    drumInstrument.playWithChannel(&drumChannel);
                }
                drumIndex = (drumIndex + 1) % drumSize;

              
                if (timeSignature == 4) { 
                    string chordRoot = backingTrackChords[(measure * timeSignature + beat) % backingTrackChords.size()];
                    pianoInstrument.setFilePath("sounds/piano/" + chordRoot + ".mp3");
                    if (pianoInstrument.initialize()) {
                        FMOD::Channel* pianoChannel = nullptr;
                        pianoInstrument.playWithChannel(&pianoChannel);
                    }
                }
                else if (timeSignature == 3) { 
                    if (beat < 3) { 
                        string chordRoot = backingTrackChords[(measure * timeSignature + beat) % backingTrackChords.size()];
                        pianoInstrument.setFilePath("sounds/piano/" + chordRoot + ".mp3");
                        if (pianoInstrument.initialize()) {
                            FMOD::Channel* pianoChannel = nullptr;
                            pianoInstrument.playWithChannel(&pianoChannel);
                        }
                    }
                }

                if (timeSignature == 4 && beat % 2 == 0) { 
                    string guitarSound = "sounds/guitar/" + mainTrackNotes[guitarNoteIndex] + ".wav";
                    guitarInstrument.setFilePath(guitarSound);
                    if (guitarInstrument.initialize()) {
                        FMOD::Channel* guitarChannel = nullptr;
                        guitarInstrument.playWithChannel(&guitarChannel);
                    }
                    guitarNoteIndex = (guitarNoteIndex + 1) % mainTrackNotes.size();
                }
                else if (timeSignature == 3 && beat == 1) { 
                    string guitarSound = "sounds/guitar/" + mainTrackNotes[guitarNoteIndex] + ".wav";
                    guitarInstrument.setFilePath(guitarSound);
                    if (guitarInstrument.initialize()) {
                        FMOD::Channel* guitarChannel = nullptr;
                        guitarInstrument.playWithChannel(&guitarChannel);
                    }
                    guitarNoteIndex = (guitarNoteIndex + 1) % mainTrackNotes.size();
                }

               
                std::this_thread::sleep_for(std::chrono::milliseconds(beatDuration));
            }
        }
    }
    catch (const std::exception& e) {
        cerr << "재생 중 예외 발생: " << e.what() << endl;
    }

    
    stopAllPlayback();
    isPlaying = false;
}



