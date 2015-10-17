#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <comdef.h>

using Microsoft::WRL::ComPtr;

#ifdef _DEBUG
#define CHECK_HR(...) \
    do { \
        HRESULT hr = __VA_ARGS__; \
        if (FAILED(hr)) { \
            _com_error err(hr); \
            OutputDebugString(err.ErrorMessage()); \
        } \
    } while (0)
#else
#define CHECK_HR(...) __VA_ARGS__
#endif
