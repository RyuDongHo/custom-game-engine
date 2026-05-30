#include "MovementState.h"

#include "Logger.h"

MovementState::MovementState()
{
    // ObservableState의 current는 enum의 0번째 값(StandRight)으로 영초기화된다.
    // 의미 있는 초기 방향(StandDown)으로 직접 지정한다. 구독자가 아직 없어 콜백 발화는 발생하지 않는다.
    current = MovementStateType::StandDown;
    LOG_INFO("MovementState created. state=%s", GetStateName());
}

void MovementState::SetFromDirectionInput(bool moveUp, bool moveDown, bool moveLeft, bool moveRight)
{
    if (moveRight) {
        Set(MovementStateType::WalkRight);
        return;
    }
    if (moveLeft) {
        Set(MovementStateType::WalkLeft);
        return;
    }
    if (moveUp) {
        Set(MovementStateType::WalkUp);
        return;
    }
    if (moveDown) {
        Set(MovementStateType::WalkDown);
        return;
    }

    Set(GetStandStateForCurrentDirection());
}

const char* MovementState::GetStateName() const
{
    return ToString(Get());
}

const char* MovementState::GetDirectionName() const
{
    switch (Get()) {
    case MovementStateType::WalkRight:
    case MovementStateType::StandRight:
        return "right";
    case MovementStateType::WalkLeft:
    case MovementStateType::StandLeft:
        return "left";
    case MovementStateType::WalkUp:
    case MovementStateType::StandUp:
        return "up";
    case MovementStateType::WalkDown:
    case MovementStateType::StandDown:
        return "down";
    default:
        return "down";
    }
}

MovementStateType MovementState::GetStandStateForCurrentDirection() const
{
    switch (Get()) {
    case MovementStateType::WalkRight:
    case MovementStateType::StandRight:
        return MovementStateType::StandRight;
    case MovementStateType::WalkLeft:
    case MovementStateType::StandLeft:
        return MovementStateType::StandLeft;
    case MovementStateType::WalkUp:
    case MovementStateType::StandUp:
        return MovementStateType::StandUp;
    case MovementStateType::WalkDown:
    case MovementStateType::StandDown:
        return MovementStateType::StandDown;
    default:
        return MovementStateType::StandDown;
    }
}

const char* MovementState::ToString(MovementStateType state)
{
    switch (state) {
    case MovementStateType::StandRight: return "stand_right";
    case MovementStateType::StandLeft:  return "stand_left";
    case MovementStateType::StandUp:    return "stand_up";
    case MovementStateType::StandDown:  return "stand_down";
    case MovementStateType::WalkRight:  return "walk_right";
    case MovementStateType::WalkLeft:   return "walk_left";
    case MovementStateType::WalkUp:     return "walk_up";
    case MovementStateType::WalkDown:   return "walk_down";
    default:                            return "unknown";
    }
}
