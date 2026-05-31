#pragma once

/*
 * FirebaseConfigDummy.h  (git tracked)
 * FirebaseConfig.h가 없을 때 사용되는 빈 자격 증명.
 *
 * 본 파일이 활성화되면 (= 실제 FirebaseConfig.h 부재):
 *  - 빌드는 통과
 *  - FirebaseLogSink::Start() 호출 시 kIsDummy 검사 → 즉시 false 반환
 *  - 콘솔 로그 sink는 그대로 동작, Firebase 전송만 비활성
 *
 * 본인 Firebase 프로젝트를 연결하려면 FirebaseConfig.h를 같은 폴더에
 * 직접 만들고 apiKey / databaseUrl을 채우면 된다. (.gitignore가 보호)
 */

namespace FirebaseSecrets {
    constexpr bool kIsDummy = true;
    constexpr const char* kApiKey      = "";
    constexpr const char* kDatabaseUrl = "";
}
