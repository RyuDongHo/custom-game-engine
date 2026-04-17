#pragma once

#include "EngineTypes.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void createWindow(GameContext* ctx, HINSTANCE hInstance, int nCmdShow, const wchar_t* winClassName, int width, int height);
