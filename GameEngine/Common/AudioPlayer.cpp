#include "AudioPlayer.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <windows.h>
#include <mmsystem.h>
#include <digitalv.h>

#pragma comment(lib, "winmm.lib")

namespace {
constexpr const wchar_t* kBackgroundMusicAlias = L"bgm_main";
std::mutex g_backgroundMusicMutex;
std::wstring g_backgroundMusicPath;

void CloseBackgroundMusic() {
    wchar_t cmd[64];
    std::swprintf(cmd, 64, L"close %s", kBackgroundMusicAlias);
    mciSendStringW(cmd, nullptr, 0, nullptr);
    g_backgroundMusicPath.clear();
}

// ── 효과음 voice pool ──────────────────────────────────────
// worker N개가 동시 재생 상한이 되고, 대기 큐는 kQueueCap으로 제한한다.
// 큐가 가득 차면 가장 오래된 요청을 버린다(최신 우선). 종료 시 worker를 join한다.
constexpr int    kVoiceCount = 8;
constexpr size_t kQueueCap   = 32;

std::mutex                g_sfxMutex;
std::condition_variable   g_sfxCv;
std::deque<std::wstring>  g_sfxQueue;
std::vector<std::thread>  g_voices;
bool                      g_sfxStarted = false;
bool                      g_sfxStop    = false;

// 한 voice(=worker alias)로 동기 재생 + close. 끝날 때까지 해당 worker thread만 block.
void PlayOnVoice(int voiceId, const std::wstring& path) {
    wchar_t alias[32];
    std::swprintf(alias, 32, L"sfx_%d", voiceId);

    wchar_t cmd[1024];
    // 직전 재생이 비정상 종료돼 alias가 남아있을 수 있으니 먼저 닫는다.
    std::swprintf(cmd, 1024, L"close %s", alias);
    mciSendStringW(cmd, nullptr, 0, nullptr);

    std::swprintf(cmd, 1024, L"open \"%s\" type mpegvideo alias %s", path.c_str(), alias);
    if (mciSendStringW(cmd, nullptr, 0, nullptr) != 0) {
        std::swprintf(cmd, 1024, L"open \"%s\" alias %s", path.c_str(), alias);
        if (mciSendStringW(cmd, nullptr, 0, nullptr) != 0) return;
    }
    std::swprintf(cmd, 1024, L"play %s wait", alias);
    mciSendStringW(cmd, nullptr, 0, nullptr);
    std::swprintf(cmd, 1024, L"close %s", alias);
    mciSendStringW(cmd, nullptr, 0, nullptr);
}

void VoiceWorker(int voiceId) {
    for (;;) {
        std::wstring path;
        {
            std::unique_lock<std::mutex> lock(g_sfxMutex);
            g_sfxCv.wait(lock, [] { return g_sfxStop || !g_sfxQueue.empty(); });
            if (g_sfxStop && g_sfxQueue.empty()) return;
            path = std::move(g_sfxQueue.front());
            g_sfxQueue.pop_front();
        }
        PlayOnVoice(voiceId, path);
    }
}

void EnsureVoicesStarted() {
    std::lock_guard<std::mutex> lock(g_sfxMutex);
    if (g_sfxStarted) return;
    g_sfxStarted = true;
    g_sfxStop = false;
    g_voices.reserve(kVoiceCount);
    for (int i = 0; i < kVoiceCount; ++i) {
        g_voices.emplace_back(VoiceWorker, i);
    }
}
} // namespace

void AudioPlayer::PlayOneShot(const wchar_t* path) {
    if (path == nullptr) return;
    EnsureVoicesStarted();
    {
        std::lock_guard<std::mutex> lock(g_sfxMutex);
        if (g_sfxStop) return;
        // 큐 포화 시 가장 오래된 요청을 버려 최신 효과음을 우선한다.
        if (g_sfxQueue.size() >= kQueueCap) {
            g_sfxQueue.pop_front();
        }
        g_sfxQueue.emplace_back(path);
    }
    g_sfxCv.notify_one();
}

void AudioPlayer::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(g_sfxMutex);
        if (!g_sfxStarted) { StopBackgroundMusic(); return; }
        g_sfxStop = true;
        g_sfxQueue.clear(); // 남은 요청은 버리고 즉시 종료(진행 중 재생만 마저 끝남).
    }
    g_sfxCv.notify_all();
    for (std::thread& t : g_voices) {
        if (t.joinable()) t.join();
    }
    g_voices.clear();
    g_sfxStarted = false;
    StopBackgroundMusic();
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
