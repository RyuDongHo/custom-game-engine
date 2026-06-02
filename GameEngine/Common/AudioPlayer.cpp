#include "AudioPlayer.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include <windows.h>
#include <mmsystem.h>
#include <digitalv.h>

#pragma comment(lib, "winmm.lib")

namespace {
// alias 충돌 방지용 카운터. 매 호출마다 +1.
std::atomic<int> g_aliasCounter{ 0 };
constexpr const wchar_t* kBackgroundMusicAlias = L"bgm_main";
std::mutex g_backgroundMusicMutex;
std::wstring g_backgroundMusicPath;

void CloseBackgroundMusic() {
    wchar_t cmd[64];
    std::swprintf(cmd, 64, L"close %s", kBackgroundMusicAlias);
    mciSendStringW(cmd, nullptr, 0, nullptr);
    g_backgroundMusicPath.clear();
}
}

void AudioPlayer::PlayOneShot(const wchar_t* path) {
    if (path == nullptr) return;
    const int id = ++g_aliasCounter;
    std::wstring pathCopy(path);

    // 별도 thread에서 동기 재생 + 자동 close. 게임 thread는 즉시 반환.
    std::thread([pathCopy, id]() {
        wchar_t alias[32];
        std::swprintf(alias, 32, L"snd_%d", id);

        // open. mpegvideo = .mp3 디코더. .wav는 type 생략해도 자동 인식.
        wchar_t cmd[1024];
        std::swprintf(cmd, 1024, L"open \"%s\" type mpegvideo alias %s",
            pathCopy.c_str(), alias);
        if (mciSendStringW(cmd, nullptr, 0, nullptr) != 0) {
            // .mp3 외 (.wav 등) 폴백.
            std::swprintf(cmd, 1024, L"open \"%s\" alias %s",
                pathCopy.c_str(), alias);
            if (mciSendStringW(cmd, nullptr, 0, nullptr) != 0) return;
        }

        // play wait — 끝날 때까지 thread block.
        std::swprintf(cmd, 1024, L"play %s wait", alias);
        mciSendStringW(cmd, nullptr, 0, nullptr);

        std::swprintf(cmd, 1024, L"close %s", alias);
        mciSendStringW(cmd, nullptr, 0, nullptr);
    }).detach();
}

bool AudioPlayer::PlayBackgroundMusic(const wchar_t* path, int volume) {
    if (path == nullptr) return false;
    if (volume < 0) volume = 0;
    if (volume > 1000) volume = 1000;

    std::lock_guard<std::mutex> lock(g_backgroundMusicMutex);
    if (g_backgroundMusicPath == path) return true;

    wchar_t cmd[1024];
    CloseBackgroundMusic();
    std::swprintf(cmd, 1024, L"open \"%s\" type mpegvideo alias %s",
        path, kBackgroundMusicAlias);
    if (mciSendStringW(cmd, nullptr, 0, nullptr) != 0) {
        std::swprintf(cmd, 1024, L"open \"%s\" alias %s",
            path, kBackgroundMusicAlias);
        if (mciSendStringW(cmd, nullptr, 0, nullptr) != 0) return false;
    }

    std::swprintf(cmd, 1024, L"setaudio %s volume to %d",
        kBackgroundMusicAlias, volume);
    mciSendStringW(cmd, nullptr, 0, nullptr);

    std::swprintf(cmd, 1024, L"play %s repeat", kBackgroundMusicAlias);
    if (mciSendStringW(cmd, nullptr, 0, nullptr) != 0) {
        CloseBackgroundMusic();
        return false;
    }
    g_backgroundMusicPath = path;
    return true;
}

void AudioPlayer::StopBackgroundMusic() {
    std::lock_guard<std::mutex> lock(g_backgroundMusicMutex);
    CloseBackgroundMusic();
}
