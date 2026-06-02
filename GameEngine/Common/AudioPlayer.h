#pragma once

/*
 * AudioPlayer.h
 * 짧은 효과음 fire-and-forget 재생.
 *
 * 내부적으로 WinMM의 mciSendString을 사용 — .mp3 / .wav 직접 재생, 외부 lib 의존 X.
 * 매 호출마다 별도 thread + unique alias로 동시 재생 가능 (Player 공격과 적 사망이 겹쳐도 OK).
 * 게임 종료 시 detached thread는 자동 정리됨 (짧은 효과음 가정).
 */

class AudioPlayer {
public:
    // path: 실행 파일 기준 상대경로. 예: L"assets\\sword_attack.mp3"
    // 즉시 반환 (재생은 백그라운드 thread).
    static void PlayOneShot(const wchar_t* path);

    // 하나의 BGM을 낮은 볼륨으로 반복 재생한다. volume 범위는 WinMM 기준 0~1000.
    static bool PlayBackgroundMusic(const wchar_t* path, int volume = 150);
    static void StopBackgroundMusic();
};
