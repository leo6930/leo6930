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
```text

## HOW TO USE

[Installation & FMOD Configuration]
Installation & FMOD Configuration
To build and run this project, you need to configure the FMOD Studio API within your Visual Studio environment.

1. Install FMOD Studio API
Download the FMOD Engine (Windows version) from the official FMOD website.

Run the installer and install it to your preferred directory (e.g., C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows).

2. Configure Visual Studio
Once installed, you need to link the FMOD libraries to your project:

A. Include Directories:

Right-click your project in Solution Explorer and go to Properties.

Navigate to C/C++ > General > Additional Include Directories.

Add the api/core/inc and api/studio/inc folders from your FMOD installation directory.

B. Library Directories:

Go to Linker > General > Additional Library Directories.

Add the api/core/lib/x64 and api/studio/lib/x64 folders.

C. Linker Dependencies:

Go to Linker > Input > Additional Dependencies.

Add fmod_vc.lib and fmodstudio_vc.lib.

3. Runtime Files
Ensure that the necessary FMOD DLL files (fmod.dll, fmodstudio.dll) are placed in the same directory as your project's executable (.exe) file to ensure the program runs correctly.

Pro Tip
If you encounter "Missing DLL" errors at runtime, simply copy the .dll files from the FMOD api/core/lib/x64 folder into your project's x64/Debug or x64/Release folder where the .exe is generated.


