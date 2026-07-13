#ifndef SCALE_H
#define SCALE_H

#include <vector>
#include <string>

using namespace std;

class Scale {
private:
    string root;
    vector<string> notes;
    
    static const vector<int> majorPattern;
    static const vector<int> minorPattern;
    static const vector<int> pentatonicMajorPattern;
    static const vector<int> pentatonicMinorPattern;

public:
    Scale(string root = "C", string type = "major");
    static const vector<string> allNotes;
    vector<string> getNotes() const;
    string getRoot() const;
    vector<string> generateTriadChord(const std::string& rootNote) const;
    static const vector<string>& getAllNotes();
};

#endif // SCALE_H
