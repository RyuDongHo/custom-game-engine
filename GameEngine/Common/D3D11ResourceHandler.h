#pragma once

#include "EngineTypes.h"

HRESULT compileShader(const void* pSrc, bool isFile, LPCSTR szEntry, LPCSTR szTarget, ID3DBlob** ppBlob);

class GraphicsContext {
private:
    // singleton management
    static GraphicsContext* pInstance;
    GraphicsContext();

    GraphicsContext(const GraphicsContext&) = delete;
    GraphicsContext& operator=(const GraphicsContext&) = delete;

    HWND hWnd;
    ID3D11Device* pd3dDevice;
    ID3D11DeviceContext* pImmediateContext;
    IDXGISwapChain* pSwapChain;
    ID3D11RenderTargetView* pRenderTargetView;
    ID3D11VertexShader* pVertexShader;
    ID3D11PixelShader* pPixelShader;
    ID3D11InputLayout* pVertexLayout;

public:
    static GraphicsContext* getInstance();
    static void Release();

    HWND getHWND();
    ID3D11Device* getDevice();
    ID3D11DeviceContext* getDeviceContext();
    IDXGISwapChain* getSwapChain();
    ID3D11RenderTargetView* getRTV();
    ID3D11VertexShader* getVertexShader();
    ID3D11PixelShader* getPixelShader();
    ID3D11InputLayout* getInputLayout();

    void setVertexShader(ID3D11VertexShader* vertexShader);
    void setPixelShader(ID3D11PixelShader* pixelShader);
    void setInputLayout(ID3D11InputLayout* inputLayout);

    void createDeviceAndSwapChainAndRTV(int width, int height);
    void createWindow(HINSTANCE hInstance, int nCmdShow, const wchar_t* winClassName, int width, int height);
    void RebuildVideoResource();
    void CleanUp();

    ~GraphicsContext();
};
