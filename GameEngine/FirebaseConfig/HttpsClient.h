#pragma once

/*
 * HttpsClient.h
 * WinHTTP 기반 단순 동기 HTTPS 요청 래퍼.
 *
 * 외부 의존성 없이 winhttp.lib 한 개만 링크.
 * Firebase Auth REST + Realtime DB REST 호출에 사용.
 *
 * 비동기 처리는 호출자(worker thread)가 담당한다 — 본 클래스는 한 번 호출 = 한 번 통신.
 */

#include <string>

namespace HttpsClient {

// host: 예 "logger-75afc-default-rtdb.firebaseio.com"
// path: 예 "/logs.json?auth=xxx"
// method: "POST", "PUT", "PATCH", "GET"
// body: 요청 본문 (UTF-8 JSON 등). GET이면 빈 문자열.
// 응답 본문을 response에 채워 반환한다.
// 반환값: HTTP status code (성공 200~299). 0이면 transport 실패.
int Request(const wchar_t* host, const wchar_t* path,
            const char* method, const std::string& body,
            std::string& response);

} // namespace HttpsClient
