#pragma once
#include "EngineTypes.h"

class GameObject;

class RenderComponent
{
public:
	GameObject* pOwner = nullptr;
	bool isRenderReady = false;

	virtual void StartRenderComponent() = 0;
	virtual void Render() = 0;
	virtual ~RenderComponent() = default;
};

