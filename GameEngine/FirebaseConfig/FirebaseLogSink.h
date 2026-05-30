#pragma once

/*
 * FirebaseLogSink.h
 * Logger ILogSink 구현 — 비동기 큐 + worker thread + Firebase Realtime DB POST.
 *
 * 시작 시 익명 sign-in으로 idToken 한 번 발급 후 메모리 보관.
 * Logger::Write가 호출되면 큐에 push하고 즉시 반환 (게임 프레임 안 막음).
 * 별도 worker thread가 100ms 주기 또는 큐 10개 도달 시 Firebase로 batch POST.
 *
 * 시그니처는 ILogSink::Write(const LogEntry&)에 맞추고, 큐에는 entry 복사본을 보관.
 */

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#include "Logger.h"

class FirebaseLogSink : public ILogSink {
public:
    FirebaseLogSink();
    ~FirebaseLogSink() override;

    // ILogSink — 큐에 push만. 실제 전송은 worker가 처리.
    void Write(const LogEntry& entry) override;

    // main에서 1회 호출. 익명 인증 후 worker thread 시작. 실패 시 false (sink는 noop으로 동작).
    bool Start();
    // main 종료 시 호출. 남은 큐를 flush하고 worker 종료.
    void Stop();

private:
    // 직렬화된 한 entry. worker가 POST body에 직접 사용.
    struct QueuedEntry {
        std::string jsonBody;
    };

    void WorkerLoop();
    bool SignInAnonymous();   // /v1/accounts:signUp 호출, idToken/refreshToken 저장.
    bool RefreshIdToken();    // 만료 시 갱신.
    bool PostOne(const std::string& jsonBody);

    static std::string JsonEscape(const std::string& s);
    static std::string BuildJson(const LogEntry& entry);

    std::string idToken;
    std::string refreshToken;
    long long   tokenExpiresAtMs = 0;  // epoch ms.

    std::deque<QueuedEntry> queue;
    std::mutex              queueMutex;
    std::condition_variable queueCv;
    std::atomic<bool>       running{ false };
    std::thread             workerThread;
};
