#include <cassert>
#include <cstdio>
#include <memory>

#include "dxutil.h"
#include "renderer.h"
#include "camera.h"

#include "xaudio2.h"

#include <iostream>


#include <shellapi.h> // must be after windows.h
#include <ShellScalingApi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "ninput.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "xaudio2.lib")

#ifdef _XBOX //Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

// Constants
static const int kSwapChainBufferCount = 3;
static const DXGI_FORMAT kSwapChainFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
static const int kUpdateFrequency = 60;

// Globals
HWND ghWnd;
bool gShouldClose;
ComPtr<IDXGISwapChain> gpSwapChain;
ComPtr<ID3D11Device> gpDevice;
ComPtr<ID3D11DeviceContext> gpDeviceContext;
std::unique_ptr<Renderer> gpRenderer;
OrbitCamera gCamera;
UINT64 gPerformanceFrequency;
UINT64 gLastFrameTicks;
UINT64 gAccumulatedFrameTicks;
double gRenderScale;
int gWindowWidth = 1280;
int gWindowHeight = 720;
int gRenderWidth;
int gRenderHeight;

ComPtr<IXAudio2> gpXAudio2;
IXAudio2SourceVoice* gpSource;
XAUDIO2_BUFFER gXAudio2Buffer;
WAVEFORMATEX gWfxThunder = { 0 };


void StartXAudio2() {
	IXAudio2MasteringVoice* pMasterVoice = nullptr;
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	CHECK_HR(XAudio2Create(&gpXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR));
	CHECK_HR(gpXAudio2->CreateMasteringVoice(&pMasterVoice));
}



// Find a chunk in a RIFF file
HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());

	DWORD dwChunkType;
	DWORD dwChunkDataSize;
	DWORD dwRIFFDataSize = 0;
	DWORD dwFileType;
	DWORD bytesRead = 0;
	DWORD dwOffset = 0;

	while (hr == S_OK)
	{
		DWORD dwRead;
		if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		switch (dwChunkType)
		{
		case fourccRIFF:
			dwRIFFDataSize = dwChunkDataSize;
			dwChunkDataSize = 4;
			if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());
			break;

		default:
			if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
				return HRESULT_FROM_WIN32(GetLastError());
		}

		dwOffset += sizeof(DWORD) * 2;

		if (dwChunkType == fourcc)
		{
			dwChunkSize = dwChunkDataSize;
			dwChunkDataPosition = dwOffset;
			return S_OK;
		}

		dwOffset += dwChunkDataSize;

		if (bytesRead >= dwRIFFDataSize) return S_FALSE;

	}

	return S_OK;

}

// Read the chunk data into a buffer
HRESULT ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());
	DWORD dwRead;
	if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
		hr = HRESULT_FROM_WIN32(GetLastError());
	return hr;
}

HRESULT LoadSoundFiles() {
	HRESULT hr = S_OK;
	
	TCHAR * strFileName = "Sounds\\thunder.wav";

	// Open the file
	HANDLE hFile = CreateFile(
		strFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (INVALID_HANDLE_VALUE == hFile) {
		std::cout << "Invalid file handle value" << '\n';
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN)) {
		std::cout << "Invalid set file pointer" << '\n';
		return HRESULT_FROM_WIN32(GetLastError());
	}

	DWORD dwChunkSize;
	DWORD dwChunkPosition;
	//check the file type, should be fourccWAVE or 'XWMA'
	FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
	DWORD filetype;
	ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
	if (filetype != fourccWAVE)
		return S_FALSE;

	FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
	ReadChunkData(hFile, &gWfxThunder, dwChunkSize, dwChunkPosition);

	//fill out the audio data buffer with the contents of the fourccDATA chunk
	FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
	BYTE * pDataBuffer = new BYTE[dwChunkSize];
	ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

	gXAudio2Buffer.AudioBytes = dwChunkSize;  //buffer containing audio data
	gXAudio2Buffer.pAudioData = pDataBuffer;  //size of the audio buffer in bytes
	gXAudio2Buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

	return hr;
}

void InitApp()
{

	StartXAudio2();
	CHECK_HR(LoadSoundFiles());

	CHECK_HR(gpXAudio2->CreateSourceVoice(&gpSource, &gWfxThunder));

    LARGE_INTEGER performanceFrequency, firstFrameTicks;
    CHECK_WIN32(QueryPerformanceFrequency(&performanceFrequency));
    CHECK_WIN32(QueryPerformanceCounter(&firstFrameTicks));
    gPerformanceFrequency = performanceFrequency.QuadPart;
    gLastFrameTicks = firstFrameTicks.QuadPart;
    gAccumulatedFrameTicks = 0;

    gpRenderer = std::make_unique<Renderer>(gpDevice.Get(), gpDeviceContext.Get());
    gpRenderer->Init();

#define SIM_ORBIT_RADIUS 50.f
#define SIM_DISC_RADIUS  12.f
    auto center = DirectX::XMVectorSet(0.0f, 0.4f*SIM_DISC_RADIUS, 0.0f, 0.0f);
    auto radius = 35.0f;
    auto minRadius = SIM_ORBIT_RADIUS - 3.25f * SIM_DISC_RADIUS;
    auto maxRadius = SIM_ORBIT_RADIUS + 3.0f * SIM_DISC_RADIUS;
    auto longAngle = 1.50f;
    auto latAngle = 0.75f;
    gCamera.View(center, radius, minRadius, maxRadius, longAngle, latAngle);
}

void ResizeApp(int width, int height)
{
    CHECK_HR(gpSwapChain->ResizeBuffers(kSwapChainBufferCount, width, height, kSwapChainFormat, 0));

    gpRenderer->Resize(width, height);

    float aspect = (float)gRenderWidth / gRenderHeight;
    gCamera.Projection(DirectX::XM_PIDIV2 * 0.8f * 3 / 2, aspect);
}

void UpdateApp()
{
    gCamera.ProcessInertia();

    LARGE_INTEGER currFrameTicks;
    CHECK_WIN32(QueryPerformanceCounter(&currFrameTicks));

    UINT64 deltaTicks = currFrameTicks.QuadPart - gLastFrameTicks;
    gAccumulatedFrameTicks += deltaTicks;

    const UINT64 kMillisecondsPerUpdate = 1000 / kUpdateFrequency;
    const UINT64 kTicksPerMillisecond = gPerformanceFrequency / 1000;
    const UINT64 kTicksPerUpdate = kMillisecondsPerUpdate * kTicksPerMillisecond;
    
    while (gAccumulatedFrameTicks >= kTicksPerUpdate)
    {
        gpRenderer->Update(kMillisecondsPerUpdate);
        gAccumulatedFrameTicks -= kTicksPerUpdate;
    }

    gLastFrameTicks = currFrameTicks.QuadPart;
}

void RenderApp()
{
    ComPtr<ID3D11Texture2D> pBackBuffer;
    CHECK_HR(gpSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)));

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    ComPtr<ID3D11RenderTargetView> pBackBufferRTV;
    CHECK_HR(gpDevice->CreateRenderTargetView(pBackBuffer.Get(), &rtvDesc, &pBackBufferRTV));
 
    gpRenderer->RenderFrame(pBackBufferRTV.Get(), gCamera);
}

// Event handler
LRESULT CALLBACK MyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    gpRenderer->HandleEvent(message, wParam, lParam);

    switch (message)
    {
    case WM_CLOSE:
        gShouldClose = true;
        return 0;
    case WM_SIZE: {
        UINT ww = LOWORD(lParam);
        UINT wh = HIWORD(lParam);

        // Ignore resizing to minimized
        if (ww == 0 || wh == 0) return 0;

        gWindowWidth = (int)ww;
        gWindowHeight = (int)wh;
        gRenderWidth = (UINT)(double(gWindowWidth)  * gRenderScale);
        gRenderHeight = (UINT)(double(gWindowHeight) * gRenderScale);

        // Update camera projection
        float aspect = (float)gRenderWidth / (float)gRenderHeight;
        gCamera.Projection(DirectX::XM_PIDIV2 * 0.8f * 3 / 2, aspect);
        
        ResizeApp(gWindowWidth, gWindowHeight);

        return 0;
    }
    case WM_MOUSEWHEEL: {
        auto delta = GET_WHEEL_DELTA_WPARAM(wParam);
        gCamera.ZoomRadius(-0.07f * delta);
        return 0;
    }
    case WM_POINTERDOWN:
    case WM_POINTERUPDATE:
    case WM_POINTERUP: {
        auto pointerId = GET_POINTERID_WPARAM(wParam);
        POINTER_INFO pointerInfo;
        if (GetPointerInfo(pointerId, &pointerInfo)) {
            if (message == WM_POINTERDOWN) {
                
                // Compute pointer position in render units
                POINT p = pointerInfo.ptPixelLocation;
                ScreenToClient(hWnd, &p);
                
                RECT clientRect;
                GetClientRect(hWnd, &clientRect);
                p.x = p.x * gRenderWidth / (clientRect.right - clientRect.left);
                p.y = p.y * gRenderHeight / (clientRect.bottom - clientRect.top);

                gCamera.AddPointer(pointerId);
            }

            // Otherwise send it to the camera controls
            gCamera.ProcessPointerFrames(pointerId, &pointerInfo);
            if (message == WM_POINTERUP) gCamera.RemovePointer(pointerId);
        }
        return 0;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

UINT SetupDPI()
{
    // Just do system DPI awareness for now for simplicity... scale the 3D content
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

    UINT dpiX = 0, dpiY;
    POINT pt = { 1, 1 };
    auto hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
        return dpiX;
    }
    else {
        return 96; // default
    }
}

int main()
{
    UINT dpi = SetupDPI();

    gRenderScale = 96.0 / double(dpi);

    // Scale default window size based on dpi
    gWindowWidth *= dpi / 96;
    gWindowHeight *= dpi / 96;

    // Create window
    {
        WNDCLASSEX wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = MyWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
        wc.lpszClassName = TEXT("WindowClass");
        CHECK_WIN32(RegisterClassEx(&wc));

        RECT wr = { 0, 0, gWindowWidth, gWindowHeight };
        CHECK_WIN32(AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE));
        ghWnd = CHECK_WIN32(CreateWindowEx(
            0, TEXT("WindowClass"),
            TEXT("Spooky"), WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
            0, 0, GetModuleHandle(NULL), 0));
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

    InitApp();

    ShowWindow(ghWnd, SW_SHOWNORMAL);

    EnableMouseInPointer(TRUE);

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
	CoUninitialize();
    CHECK_HR(gpSwapChain->SetFullscreenState(FALSE, NULL));
}