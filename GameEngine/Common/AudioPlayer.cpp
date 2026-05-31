#include "AudioPlayer.h"

#include <atomic>
#include <string>
#include <thread>

#include <windows.h>
#include <mmsystem.h>
#include <digitalv.h>

#pragma comment(lib, "winmm.lib")

namespace {
// alias 충돌 방지용 카운터. 매 호출마다 +1.
std::atomic<int> g_aliasCounter{ 0 };
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
