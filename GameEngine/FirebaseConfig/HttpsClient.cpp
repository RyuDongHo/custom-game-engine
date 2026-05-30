#include "HttpsClient.h"

#include <vector>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

namespace HttpsClient {

namespace {
// 응답 status code 읽기 helper.
int ReadStatus(HINTERNET hRequest) {
    DWORD status = 0;
    DWORD size = sizeof(status);
    if (!WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX)) {
        return 0;
    }
    return static_cast<int>(status);
}

// 응답 본문을 끝까지 읽음.
void ReadAllBody(HINTERNET hRequest, std::string& out) {
    DWORD available = 0;
    while (WinHttpQueryDataAvailable(hRequest, &available) && available > 0) {
        std::vector<char> chunk(available);
        DWORD read = 0;
        if (!WinHttpReadData(hRequest, chunk.data(), available, &read)) break;
        out.append(chunk.data(), read);
    }
}

// char* → wide. (Win32 헤더에서 wide 필요한 곳용.)
std::wstring Widen(const char* s) {
    if (s == nullptr) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (len <= 1) return L"";
    std::wstring w(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], len);
    return w;
}
}

int Request(const wchar_t* host, const wchar_t* path,
            const char* method, const std::string& body,
            std::string& response)
{
    response.clear();
    if (host == nullptr || path == nullptr || method == nullptr) return 0;

    int status = 0;

    HINTERNET hSession = WinHttpOpen(L"GameEngineLogger/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == nullptr) return 0;

    HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (hConnect == nullptr) { WinHttpCloseHandle(hSession); return 0; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, Widen(method).c_str(), path,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (hRequest == nullptr) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 0;
    }

    const wchar_t* headers = L"Content-Type: application/json\r\n";
    const DWORD bodyLen = static_cast<DWORD>(body.size());
    LPVOID bodyPtr = bodyLen > 0 ? const_cast<char*>(body.data()) : WINHTTP_NO_REQUEST_DATA;

    if (WinHttpSendRequest(hRequest, headers, -1L, bodyPtr, bodyLen, bodyLen, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr)) {
        status = ReadStatus(hRequest);
        ReadAllBody(hRequest, response);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return status;
}

} // namespace HttpsClient
