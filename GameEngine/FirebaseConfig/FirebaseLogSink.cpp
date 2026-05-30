#include "FirebaseLogSink.h"

#include <chrono>
#include <cstdio>
#include <cstring>

#include "FirebaseConfig.h"   // gitignored — apiKey, databaseUrl
#include "HttpsClient.h"

namespace {

// epoch ms.
long long NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

const char* LevelToStr(LogLevel l) {
    switch (l) {
    case LogLevel::Debug:   return "DEBUG";
    case LogLevel::Info:    return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error:   return "ERROR";
    default:                return "UNKNOWN";
    }
}

// "https://logger-75afc-default-rtdb.firebaseio.com" → ("logger-...firebaseio.com")
std::wstring HostFromUrl(const char* url) {
    const char* p = std::strstr(url, "://");
    p = (p ? p + 3 : url);
    std::wstring out;
    while (*p && *p != '/' && *p != '?') {
        out.push_back(static_cast<wchar_t>(*p));
        ++p;
    }
    return out;
}

// signUp/signInWithPassword 응답 JSON에서 "field":"value" 추출.
// 공백/줄바꿈을 허용하는 단순 파서 — Firebase 응답이 pretty-print되어 있어도 OK.
std::string ExtractJsonField(const std::string& json, const char* field) {
    std::string key = "\"";
    key += field;
    key += "\"";
    auto pos = json.find(key);
    if (pos == std::string::npos) return "";
    pos += key.size();
    auto skipWs = [&]() {
        while (pos < json.size() &&
               (json[pos] == ' ' || json[pos] == '\t' ||
                json[pos] == '\r' || json[pos] == '\n')) ++pos;
    };
    skipWs();
    if (pos >= json.size() || json[pos] != ':') return "";
    ++pos;
    skipWs();
    if (pos >= json.size() || json[pos] != '"') return "";
    ++pos;
    const auto start = pos;
    const auto end   = json.find('"', start);
    if (end == std::string::npos) return "";
    return json.substr(start, end - start);
}
} // anonymous

FirebaseLogSink::FirebaseLogSink() = default;

FirebaseLogSink::~FirebaseLogSink() {
    Stop();
}

bool FirebaseLogSink::Start() {
    if (running.load()) return true;
    if (!SignInAnonymous()) {
        std::fprintf(stdout, "[FirebaseLogSink] SignInAnonymous failed\n");
        return false;
    }
    running.store(true);
    workerThread = std::thread(&FirebaseLogSink::WorkerLoop, this);
    return true;
}

void FirebaseLogSink::Stop() {
    if (!running.load()) return;
    running.store(false);
    queueCv.notify_all();
    if (workerThread.joinable()) workerThread.join();
}

void FirebaseLogSink::Write(const LogEntry& entry) {
    // Auth 실패 등으로 worker가 안 돌면 그냥 drop.
    if (!running.load()) return;
    QueuedEntry q{ BuildJson(entry) };
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        // 큐가 너무 길면 head를 drop (메모리 보호).
        if (queue.size() >= 1024) queue.pop_front();
        queue.push_back(std::move(q));
    }
    queueCv.notify_one();
}

void FirebaseLogSink::WorkerLoop() {
    // 추천 정책: 큐가 10개 도달하면 즉시 flush, 그 외엔 100ms 주기로 flush.
    constexpr size_t kBatchTrigger = 10;
    constexpr auto   kFlushInterval = std::chrono::milliseconds(100);

    for (;;) {
        std::deque<QueuedEntry> drain;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCv.wait_for(lock, kFlushInterval, [&] {
                return !running.load() || queue.size() >= kBatchTrigger;
            });
            // 종료 + 큐 빔 판정은 반드시 lock 안에서 수행 (Write()와의 race 방지).
            if (!running.load() && queue.empty()) {
                break;
            }
            drain.swap(queue);
        }
        if (drain.empty()) continue;
        // 토큰 만료 가드 (1시간 만료 5분 전 갱신).
        if (NowMs() + 5 * 60 * 1000 >= tokenExpiresAtMs) {
            RefreshIdToken();
        }
        // 진짜 batch — N개를 한 번의 PATCH multi-update로 보낸다.
        // body 형태: {"<unique-key-1>": <entry1>, "<unique-key-2>": <entry2>, ...}
        // unique key: timestamp(ms) + counter (sort 순 timestamp 보존).
        std::string body;
        body.reserve(drain.size() * 256 + 4);
        body.push_back('{');
        long long ts = NowMs();
        size_t counter = 0;
        for (auto& e : drain) {
            if (counter > 0) body.push_back(',');
            char key[40];
            std::snprintf(key, sizeof(key), "\"k%lld_%04zu\":", ts, counter);
            body += key;
            body += e.jsonBody;
            ++counter;
        }
        body.push_back('}');

        std::string path = "/logs.json?auth=" + idToken;
        PostJson("PATCH", path, body);
    }
}

bool FirebaseLogSink::SignInAnonymous() {
    // POST https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=<API_KEY>
    wchar_t path[256];
    std::swprintf(path, 256,
        L"/v1/accounts:signUp?key=%hs", FirebaseSecrets::kApiKey);

    std::string response;
    const int status = HttpsClient::Request(
        L"identitytoolkit.googleapis.com", path,
        "POST", std::string("{\"returnSecureToken\":true}"), response);
    if (status < 200 || status >= 300) {
        std::fprintf(stdout, "[FirebaseLogSink] signUp status=%d body=%s\n",
            status, response.c_str());
        return false;
    }
    idToken = ExtractJsonField(response, "idToken");
    refreshToken = ExtractJsonField(response, "refreshToken");
    // expiresIn은 초 단위.
    const std::string expiresStr = ExtractJsonField(response, "expiresIn");
    long long secs = expiresStr.empty() ? 3600 : std::atoll(expiresStr.c_str());
    if (secs <= 0) secs = 3600;
    tokenExpiresAtMs = NowMs() + secs * 1000;
    if (idToken.empty()) {
        std::fprintf(stdout, "[FirebaseLogSink] signUp OK but idToken empty. body(first 400): %.400s\n",
            response.c_str());
        std::fflush(stdout);
        return false;
    }
    std::fprintf(stdout, "[FirebaseLogSink] signUp OK idTokenLen=%zu expiresIn=%llds\n",
        idToken.size(), secs);
    std::fflush(stdout);
    return true;
}

bool FirebaseLogSink::RefreshIdToken() {
    if (refreshToken.empty()) return SignInAnonymous();
    wchar_t path[256];
    std::swprintf(path, 256, L"/v1/token?key=%hs", FirebaseSecrets::kApiKey);
    std::string body = "grant_type=refresh_token&refresh_token=" + refreshToken;
    std::string response;
    // /v1/token은 application/x-www-form-urlencoded이지만 단순화를 위해
    // 실패 시 signUp으로 fallback (새 익명 계정 발급).
    const int status = HttpsClient::Request(
        L"securetoken.googleapis.com", path,
        "POST", body, response);
    if (status < 200 || status >= 300) {
        return SignInAnonymous();
    }
    const std::string newId = ExtractJsonField(response, "id_token");
    if (!newId.empty()) idToken = newId;
    const std::string newRefresh = ExtractJsonField(response, "refresh_token");
    if (!newRefresh.empty()) refreshToken = newRefresh;
    tokenExpiresAtMs = NowMs() + 3600 * 1000;
    return !idToken.empty();
}

bool FirebaseLogSink::PostJson(const char* method, const std::string& pathUtf8, const std::string& jsonBody) {
    std::wstring host = HostFromUrl(FirebaseSecrets::kDatabaseUrl);
    // ASCII-only path/query — char→wchar 단순 변환.
    std::wstring path;
    path.reserve(pathUtf8.size());
    for (char c : pathUtf8) path.push_back(static_cast<wchar_t>(c));

    std::string response;
    const int status = HttpsClient::Request(host.c_str(), path.c_str(),
        method, jsonBody, response);
    if (status < 200 || status >= 300) {
        std::fprintf(stdout, "[FirebaseLogSink] %s /logs status=%d body=%s\n",
            method, status, response.c_str());
        return false;
    }
    return true;
}

std::string FirebaseLogSink::JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b";  break;
        case '\f': out += "\\f";  break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                out += buf;
            } else {
                out += c;
            }
        }
    }
    return out;
}

std::string FirebaseLogSink::BuildJson(const LogEntry& entry) {
    // 파일 경로는 끝부분만 (전체 경로는 PC별로 달라 의미 없음).
    const char* fileShort = entry.file ? entry.file : "";
    if (entry.file) {
        const char* slash = std::strrchr(entry.file, '\\');
        const char* fwd   = std::strrchr(entry.file, '/');
        const char* last  = (slash > fwd ? slash : fwd);
        if (last) fileShort = last + 1;
    }
    char header[512];
    std::snprintf(header, sizeof(header),
        "{\"ts\":%lld,\"level\":\"%s\",\"author\":\"%s\",\"file\":\"%s\",\"msg\":\"",
        NowMs(),
        LevelToStr(entry.level),
        entry.author ? entry.author : "?",
        fileShort);
    std::string out = header;
    out += JsonEscape(entry.message);
    out += "\"}";
    return out;
}
