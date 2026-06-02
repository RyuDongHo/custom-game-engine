#pragma once

/*
 * AuthorMap.h
 * __FILE__ 경로 → 작성자 이름 매핑.
 *
 * Logger가 LOG_* 매크로로 호출될 때 호출처 파일을 lookup해 author 필드를 채운다.
 * git log의 first-commit author 기준으로 매핑 (unknown/RyuDongHo는 RDH로 통합).
 *
 * 새 파일 추가 시 default = "RDH".
 */

#include <cstdio>
#include <cstring>

namespace AuthorMap {

// 파일 경로 끝이 suffix와 일치하는지. __FILE__는 보통 절대경로라 끝부분만 비교.
inline bool EndsWith(const char* path, const char* suffix) {
    if (path == nullptr || suffix == nullptr) return false;
    const size_t lp = std::strlen(path);
    const size_t ls = std::strlen(suffix);
    if (ls > lp) return false;
    return std::strcmp(path + (lp - ls), suffix) == 0;
}

// 정규화된 separator로 비교 (__FILE__ on MSVC는 '\\' 포함).
// 비교 simplification: 양쪽 '\\'를 '/'로 본다고 가정. EndsWith는 단순 strcmp 기반이라
// "/SpriteAnimator.cpp"와 "\\SpriteAnimator.cpp"를 모두 매칭하려면 두 케이스 다 등록한다.
inline const char* Lookup(const char* file) {
    if (file == nullptr) return "RDH";

    struct Entry { const char* suffix; const char* author; };
    static const Entry kTable[] = {
        // Kimunet
        { "SpriteAnimator.cpp",        "RDH"  },
        { "SpriteAnimator.h",          "RDH"  },
        { "EnemyState.cpp",            "Kimunet"  },
        // (LevelLayout.h는 RDH가 이미 재작성했으므로 RDH default)

        // Seojin
        { "State.h",                   "Seojin"   },
        { "AttackState.cpp",           "Seojin"   },
        { "AttackState.h",             "Seojin"   },
        { "LifeState.cpp",             "Seojin"   },
        { "LifeState.h",               "Seojin"   },
        { "MovementState.cpp",         "Seojin"   },
        { "MovementState.h",           "Seojin"   },

        // zkfkel123 — TerrainStateController/EnvironmentRenderer 초기 작성 (PR #3)
        { "EnvironmentRenderer.cpp",   "zkfkel123"},
        { "EnvironmentRenderer.h",     "zkfkel123"},
        { "TerrainStateController.cpp","zkfkel123"},
        { "TerrainStateController.h",  "zkfkel123"},
        { "TerrainState.cpp",          "zkfkel123"},
        { "TerrainState.h",            "zkfkel123"},
    };

    for (const Entry& e : kTable) {
        // 양쪽 separator 둘 다 매칭하기 위해 두 prefix를 시도.
        char with_back[128];
        char with_fwd[128];
        std::snprintf(with_back, sizeof(with_back), "\\%s", e.suffix);
        std::snprintf(with_fwd,  sizeof(with_fwd),  "/%s",  e.suffix);
        if (EndsWith(file, with_back) || EndsWith(file, with_fwd)) {
            return e.author;
        }
    }
    return "RDH";
}

} // namespace AuthorMap
