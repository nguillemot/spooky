#pragma once

#include "dxutil.h"

#include <vector>

class Renderer
{
    ID3D11Device* mpDevice;
    ID3D11DeviceContext* mpDeviceContext;

    ComPtr<ID3D11Buffer> mpScenePositionVertexBuffer;
    ComPtr<ID3D11Buffer> mpSceneIndexBuffer;
    std::vector<D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS> mSceneDrawArgs;

    ComPtr<ID3D11VertexShader> mpSceneVertexShader;
    ComPtr<ID3D11PixelShader> mpScenePixelShader;
    ComPtr<ID3D11InputLayout> mpSceneInputLayout;

    ComPtr<ID3D11Buffer> mpCameraBuffer;

public:
    Renderer(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext);

    void LoadScene();
    void RenderFrame(ID3D11RenderTargetView* pRTV, int width, int height);
};