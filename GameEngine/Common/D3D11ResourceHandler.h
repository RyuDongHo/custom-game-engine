#pragma once

#include "EngineTypes.h"

void createDeviceAndSwapChainAndRTV(GameContext* ctx, int width, int height);
HRESULT compileShader(const void* pSrc, bool isFile, LPCSTR szEntry, LPCSTR szTarget, ID3DBlob** ppBlob);
void RebuildVideoResource(GameContext* ctx);