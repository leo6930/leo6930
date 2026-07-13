#ifndef TRACK_H
#define TRACK_H

#include "pch.h"
#include "Instrument.h"
#include "Scale.h"
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <afxwin.h>
#include <thread>

using namespace std;

class CInstrumentDlg; // 전방 선언 (헤더 파일 간 순환 종속성 방지)

class Track {
private:
    int bpm;
    int timeSignature;  // 박자
    string key;
    bool isPlaying;
    vector<string> backingTrackChords;
    vector<string> mainTrackNotes;
    vector<string> drumPattern;

    


    function<void(const CString&)> outputFunc; // 출력 함수
    mutex mtx;


    thread playbackThread; // 재생을 위한 스레드
    thread drumPlaybackThread;
    void playBackingTrackThreaded(Instrument& pianoInstrument, const Scale& scale);
    void playDrumTrackThreaded(Instrument& drumInstrument);


    //채널관리
    vector<FMOD::Channel*> drumChannels;
    vector<FMOD::Channel*> pianoChannels;

public:
    Track(int bpm, const string& key, int timeSignature);

    void setBPM(int newBPM);
    void setTimeSignature(int newTimeSignature);
    void setScale(const Scale& scale);
    void generateBackingTrack(const Scale& scale,int comboindex);
    void generateDrumPattern();
    vector<string> getMainTrackNotes() const;
    vector<string> getBackingTrackChords() const;
    string getBackingTrackChordsAsString() const;
    string getMainTrackNotesAsString() const;
    string getDrumPatternAsString() const;
    void playTrackWithTiming(CInstrumentDlg* dlg);
    void setOutputFunction(function<void(const CString&)> func);
    void playBackingTrack(Instrument& pianoInstrument, const Scale& scale);

    void generateMainTrackNotes(const Scale& scale);

    int getBPM() const; // BPM 값을 반환하는 함수 선언
    void stopAllPlayback();
    ~Track();
    void playDrumTrack(Instrument& drumInstrument);

    //모든 악기 함께 재생!
    void playSynchronizedTrack(Instrument& pianoInstrument, Instrument& drumInstrument, Instrument& guitarInstrument, const Scale& scale);
    void playGuitarTrack(Instrument& guitarInstrument, const Scale& scale);
};

#endif // TRACK_H
