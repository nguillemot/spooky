#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <cassert>
#include <cstdio>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#ifdef _DEBUG
#define CHECK_HR(...) do { HRESULT hr = __VA_ARGS__; assert(SUCCEEDED(hr)); } while (0)
#else
#define CHECK_HR(...) __VA_ARGS__
#endif

using Microsoft::WRL::ComPtr;

// Constants
static const int kSwapChainBufferCount = 3;
static const DXGI_FORMAT kSwapChainFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

// Globals
HWND ghWnd;
bool gShouldClose;
ComPtr<IDXGISwapChain> gpSwapChain;
ComPtr<ID3D11Device> gpDevice;
ComPtr<ID3D11DeviceContext> gpDeviceContext;

void InitApp()
{

}

void ResizeApp(int width, int height)
{
    CHECK_HR(gpSwapChain->ResizeBuffers(kSwapChainBufferCount, width, height, kSwapChainFormat, 0));
}

void UpdateApp()
{

}

void RenderApp()
{

}

// Event handler
LRESULT CALLBACK MyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        gShouldClose = true;
        return 0;
    case WM_SIZE:
        ResizeApp(LOWORD(lParam), HIWORD(lParam));
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

int main()
{
    // Create window
    {
        WNDCLASSEX wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = MyWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
        wc.lpszClassName = TEXT("WindowClass");
        RegisterClassEx(&wc);

        RECT wr = { 0, 0, 640, 480 };
        AdjustWindowRect(&wr, 0, FALSE);
        ghWnd = CreateWindowEx(
            0, TEXT("WindowClass"),
            TEXT("Spooky"), WS_OVERLAPPEDWINDOW,
            0, 0, wr.right - wr.left, wr.bottom - wr.top,
            0, 0, GetModuleHandle(NULL), 0);
    }

    // Create D3D11 device and swap chain
    {
        ComPtr<IDXGIFactory> pFactory;
        CHECK_HR(CreateDXGIFactory(IID_PPV_ARGS(&pFactory)));

        UINT deviceFlags = 0;
#ifdef _DEBUG
        deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        RECT clientRect;
        GetClientRect(ghWnd, &clientRect);

        DXGI_SWAP_CHAIN_DESC scd{};
        scd.BufferCount = kSwapChainBufferCount;
        scd.BufferDesc.Format = kSwapChainFormat;
        scd.BufferDesc.Width = clientRect.right - clientRect.left;
        scd.BufferDesc.Height = clientRect.bottom - clientRect.top;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = ghWnd;
        scd.Windowed = TRUE;
        scd.SampleDesc.Count = 1;
        scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        CHECK_HR(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, NULL, 0, D3D11_SDK_VERSION, &scd, &gpSwapChain, &gpDevice, NULL, &gpDeviceContext));
    }

    ShowWindow(ghWnd, SW_SHOWNORMAL);

    InitApp();

    while (!gShouldClose)
    {
        // Handle all events
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        UpdateApp();

        RenderApp();

        // Swap buffers
        CHECK_HR(gpSwapChain->Present(0, 0));
    }
}