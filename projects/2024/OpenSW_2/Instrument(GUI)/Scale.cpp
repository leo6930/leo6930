#include "Scale.h"
#include "pch.h"
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <fstream>
using namespace std;

const vector<string> Scale::allNotes = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
const vector<int> Scale::majorPattern = { 2, 2, 1, 2, 2, 2, 1 };
const vector<int> Scale::minorPattern = { 2, 1, 2, 2, 1, 2, 2 };
const vector<int> Scale::pentatonicMajorPattern = { 2, 2, 3, 2, 3 };
const vector<int> Scale::pentatonicMinorPattern = { 3, 2, 2, 3, 2 };

Scale::Scale(string root, string type) : root(root) {
    notes.clear();

    int startIdx = -1;
    for (size_t i = 0; i < allNotes.size(); ++i) {
        if (allNotes[i] == root) {
            startIdx = i;
            break;
        }
    }

    if (startIdx == -1) {
        throw std::invalid_argument("잘못된 루트 값: " + root);
    }

    
    transform(type.begin(), type.end(), type.begin(), ::tolower);

    const vector<int>* pattern = nullptr;
    if (type == "major") {
        pattern = &majorPattern;
    }
    else if (type == "minor") {
        pattern = &minorPattern;
    }
    else if (type == "pentatonic major") {
        pattern = &pentatonicMajorPattern;
    }
    else if (type == "pentatonic minor") {
        pattern = &pentatonicMinorPattern;
    }
    else {
        throw std::invalid_argument("잘못된 스케일 타입: " + type);
    }

    int idx = startIdx;
    notes.push_back(allNotes[idx]);
    for (const auto& step : *pattern) {
        idx = (idx + step) % allNotes.size();
        notes.push_back(allNotes[idx]);
    }
}

vector<string> Scale::getNotes() const {
    return notes;
}

string Scale::getRoot() const {
    return root;
}

const vector<string>& Scale::getAllNotes() {
    return allNotes;
}


vector<string> Scale::generateTriadChord(const string& rootNote) const {
    auto it = find(allNotes.begin(), allNotes.end(), rootNote);
    if (it == allNotes.end()) {
        throw std::invalid_argument("알 수 없는 루트 노트: " + rootNote);
    }

    int rootIndex = distance(allNotes.begin(), it);
    vector<string> chordNotes = {
        allNotes[rootIndex],
        allNotes[(rootIndex + 4) % allNotes.size()],
        allNotes[(rootIndex + 7) % allNotes.size()]
    };

    cerr << "디버깅: 코드 구성 음 (필터링 전): ";
    for (const auto& note : chordNotes) {
        cerr << note << " ";
    }
    cerr << endl;

    
    chordNotes.erase(
        remove_if(chordNotes.begin(), chordNotes.end(), [](const string& note) {
            string filePath = "sounds/piano/" + note + ".mp3";
            ifstream file(filePath);
            return !file.good();
            }),
        chordNotes.end()
    );

    cerr << "디버깅: 코드 구성 음 (필터링 후): ";
    for (const auto& note : chordNotes) {
        cerr << note << " ";
    }
    cerr << endl;

    return chordNotes;
}
