#pragma once

/*
 * ScoreState.h
 * 누적 점수(int) Observable State. 변경 시 (prev, next) 콜백 발화.
 *
 * 일반적으로 Player GameObject에 부착하며, 픽업/킬 등 다양한 트리거가 Add를 호출한다.
 * 콘솔/UI/사운드 등 반응은 구독자가 처리 (StateCallbacks::OnScoreChange 등).
 */

#include "State.h"

class ScoreState : public ObservableState<int>
{
public:
    ScoreState() = default;

    int  GetCurrent() const { return Get(); }
    void SetCurrent(int v) { Set(v); }
    void Add(int delta) { Set(Get() + delta); }
};
