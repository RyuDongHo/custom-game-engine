#pragma once

/*
 * AudioPlayer.h
 * 짧은 효과음 fire-and-forget 재생.
 *
 * 내부적으로 WinMM의 mciSendString을 사용 — .mp3 / .wav 직접 재생, 외부 lib 의존 X.
 * 효과음은 고정 크기 voice pool(worker thread N개) + bounded queue로 재생한다. 호출이
 * 몰려도 thread/MCI handle이 상한을 넘지 않고, 큐가 가득 차면 가장 오래된 요청을 버린다.
 * 종료 시 Shutdown()으로 worker를 정리(join)한다. (report §5.6)
 */

class AudioPlayer {
public:
    // path: 실행 파일 기준 상대경로. 예: L"assets\\sword_attack.mp3"
    // 즉시 반환 (재생은 voice pool worker가 처리). 큐 포화 시 조용히 drop.
    static void PlayOneShot(const wchar_t* path);

    // 하나의 BGM을 낮은 볼륨으로 반복 재생한다. volume 범위는 WinMM 기준 0~1000.
    static bool PlayBackgroundMusic(const wchar_t* path, int volume = 150);
    static void StopBackgroundMusic();

    // 효과음 voice pool worker를 정지하고 join한다. BGM도 닫는다.
    // main 종료 정리에서 호출. 멱등.
    static void Shutdown();
};
