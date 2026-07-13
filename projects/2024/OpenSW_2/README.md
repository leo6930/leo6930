# Pentatonic Automatic Composer (MFC)

This project is an **Automatic Composition Software** that leverages the **Pentatonic Scale** algorithm to generate musical melodies automatically. It provides a user-friendly interface built with **MFC (Microsoft Foundation Class)** to visualize, edit, and play the generated compositions.

## Key Features
- **Pentatonic Composition Algorithm:** Automatically generates melodic sequences based on pentatonic scale rules.
- **Instrument Variety:** Supports real-time playback with various high-quality samples, including Drum, Guitar, and Piano.
- **GUI Control Interface:** A Windows-based MFC interface designed for easy track management, instrument selection, and playback control.
- **Audio Engine:** Integrated with the **FMOD Studio API** for robust, low-latency audio rendering and track sequencing.

## Tech Stack
- **Language:** C++
- **GUI Framework:** MFC (Microsoft Foundation Class)
- **Audio Engine:** FMOD Studio API
- **IDE:** Visual Studio 2022

## Project Structure
```text
Instrument(GUI)/
├── res/               # Program resources (icons, images)
├── sounds/            # Instrument audio samples (drum, guitar, piano)
├── Scale.cpp/h        # Pentatonic scale generation logic
├── Track.cpp/h        # Track management and sequencer logic
├── CInstrumentDlg.cpp # MFC GUI main dialog controller
└── Instrument(GUI).sln # Visual Studio solution file
