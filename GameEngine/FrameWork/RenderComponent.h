#pragma once
#include "EngineTypes.h"

class GameObject;

class RenderComponent
{
public:
	GameContext* p_gCtx;
	GameObject* pOwner = nullptr;
	bool isRenderReady = false;

	virtual void StartRenderComponent(GameContext* p_gCtx) = 0;
	virtual void Render() = 0;
	virtual ~RenderComponent() = default;
};

