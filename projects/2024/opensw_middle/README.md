
# 🎵 Melodic Pentatonic Generator & Sequencer

## 📝 Project Description
**Melodic Pentatonic Generator & Sequencer** is an interactive, terminal-based musical tool written in C. It serves as both an educational utility and a creative companion for songwriters and improvisers. 

The application bridges music theory and algorithmic composition. It takes any of the 12 standard Western Diatonic Major scales and dynamically extracts notes to form their corresponding **Major Pentatonic scales** (the backbone of blues, rock, and jazz improvisation). 

Going beyond a passive music dictionary, the project features an intelligent algorithmic sequencer. By simulating human playing constraints, it generates a flowing, naturally phrased 16-note random melody instead of chaotic, disjointed noise. It provides an excellent starting point for generative music experimentation, phrase training, or instant songwriting inspiration.

---

## 🚀 Key Features
* **Full Chromatic Scale Mapping:** Pre-configured with all 12 standard major scales, accurately preserving musical accidental spellings (sharps `#` and flats `b`).
* **Dynamic Pentatonic Derivation:** Automatically extracts the 1st, 2nd, 3rd, 5th, and 6th intervals from the root major scale using dynamic array allocation.
* **Smart Melodic Sequencing:** The built-in phrase generator avoids raw randomness, utilizing strict interval constraints to create cohesive, organic melodies.
* **Interactive CLI Interface:** A user-friendly, loop-driven command-line interface that allows seamless navigation, testing, and continuous generation until explicitly closed.
* **Audio-Ready Architecture:** Designed with native Windows multimedia headers (`windows.h`, `mmsystem.h`) and linked libraries, paving the way for future physical audio playback and MIDI output.

---

## 🛠️ Technical Implementation & Architecture

### 1. Data Structures
The program represents musical properties using a clean, pointer-based `Scale` structure to maximize memory efficiency:
```c
typedef struct {
    char* name;      // Name of the scale (e.g., "C major")
    char* notes[7];  // Array of pointers to the 7 diatonic notes
} Scale;
