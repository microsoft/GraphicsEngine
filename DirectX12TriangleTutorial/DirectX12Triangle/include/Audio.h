#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <string>
#pragma comment(lib, "winmm.lib")

class AudioPlayer {
private:
    std::wstring currentFile;
    bool isPlaying;
    
public:
    AudioPlayer() : isPlaying(false) {}
    
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
};