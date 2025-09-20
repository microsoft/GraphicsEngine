#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <string>
#pragma comment(lib, "winmm.lib")

class AudioPlayer {
private:
    std::wstring currentFile;
    bool isPlaying;
    int soundEffectCounter;

public:
    AudioPlayer() : isPlaying(false), soundEffectCounter(0) {}

    ~AudioPlayer() {
        Stop();
    }

    bool BGM(const std::string& filename) {
        // Stop any audio
        Stop();

        // Convert string to wstring
        std::wstring wFilename(filename.begin(), filename.end());
        currentFile = wFilename;

        // Build MCI command
        std::wstring command = L"open \"" + wFilename + L"\" type mpegvideo alias bgmusic";
        if (mciSendString(command.c_str(), NULL, 0, NULL) != 0) {
            return false;
        }

        // Play with repeat
        if (mciSendString(L"play bgmusic repeat", NULL, 0, NULL) != 0) {
            mciSendString(L"close bgmusic", NULL, 0, NULL);
            return false;
        }

        isPlaying = true;
        return true;
    }

    void Stop() {
        if (isPlaying) {
            mciSendString(L"stop bgmusic", NULL, 0, NULL);
            mciSendString(L"close bgmusic", NULL, 0, NULL);
            isPlaying = false;
        }
    }

    void SetVolume(int volume) { // 0-1000
        std::wstring cmd = L"setaudio bgmusic volume to " + std::to_wstring(volume);
        mciSendString(cmd.c_str(), NULL, 0, NULL);
    }

    void Pause() {
        if (isPlaying) {
            mciSendString(L"pause bgmusic", NULL, 0, NULL);
        }
    }

    void Resume() {
        if (isPlaying) {
            mciSendString(L"resume bgmusic", NULL, 0, NULL);
        }
    }

    // Simple sound effect player - fire and forget
    bool PlaySoundEffect(const std::string& filename) {
        // Convert string to wstring
        std::wstring wFilename(filename.begin(), filename.end());

        // Create unique alias for this sound
        std::wstring alias = L"sfx" + std::to_wstring(soundEffectCounter++);

        // Open and play the sound
        std::wstring command = L"open \"" + wFilename + L"\" type waveaudio alias " + alias;
        if (mciSendString(command.c_str(), NULL, 0, NULL) != 0) {
            // Try mpegvideo for mp3
            command = L"open \"" + wFilename + L"\" type mpegvideo alias " + alias;
            if (mciSendString(command.c_str(), NULL, 0, NULL) != 0) {
                return false;
            }
        }

        // Play once and auto-close
        command = L"play " + alias + L" wait";
        mciSendString(command.c_str(), NULL, 0, NULL);

        // Close after playing
        command = L"close " + alias;
        mciSendString(command.c_str(), NULL, 0, NULL);

        return true;
    }
};