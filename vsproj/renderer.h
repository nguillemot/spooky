#pragma once

#include "dxutil.h"
#include "camera.h"
#include <DirectXMath.h>

#include <vector>

class Renderer
{
    __declspec(align(16))
    struct Material
    {
        DirectX::XMFLOAT3 AmbientColor;
        DirectX::XMFLOAT3 DiffuseColor;
        DirectX::XMFLOAT3 SpecularColor;
        float Ns;
    };

    ID3D11Device* mpDevice;
    ID3D11DeviceContext* mpDeviceContext;

    ComPtr<ID3D11Buffer> mpScenePositionVertexBuffer;
    ComPtr<ID3D11Buffer> mpScenePositionNormalBuffer;
    ComPtr<ID3D11Buffer> mpSceneIndexBuffer;
    ComPtr<ID3D11Buffer> mpSceneInstanceBuffer;
    std::vector<D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS> mSceneDrawArgs;

    ComPtr<ID3D11Buffer> mpCameraBuffer;
    ComPtr<ID3D11Buffer> mpLightBuffer;
    ComPtr<ID3D11Buffer> mpMaterialBuffer;

    std::vector<Material> mMaterialVector;

    ComPtr<ID3D11Texture2D> mpSceneDepthBuffer;
    ComPtr<ID3D11DepthStencilView> mpSceneDSV;

    ComPtr<ID3D11VertexShader> mpSceneVertexShader;
    ComPtr<ID3D11PixelShader> mpScenePixelShader;
    ComPtr<ID3D11InputLayout> mpSceneInputLayout;
    ComPtr<ID3D11RasterizerState> mpSceneRasterizerState;
    ComPtr<ID3D11DepthStencilState> mpSceneDepthStencilState;

    ComPtr<ID3D11Resource> mpSkyboxTexture;
    ComPtr<ID3D11ShaderResourceView> mpSkyboxTextureSRV;
    ComPtr<ID3D11SamplerState> mpSkyboxSampler;

    ComPtr<ID3D11VertexShader> mpSkyboxVertexShader;
    ComPtr<ID3D11PixelShader> mpSkyboxPixelShader;
    ComPtr<ID3D11RasterizerState> mpSkyboxRasterizerState;
    ComPtr<ID3D11DepthStencilState> mpSkyboxDepthStencilState;

    int mClientWidth;
    int mClientHeight;

public:
    Renderer(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext);

    void Init();

    void Resize(int width, int height);

    void Update(int deltaTime_ms);

    void RenderFrame(ID3D11RenderTargetView* pRTV, const OrbitCamera& camera);
};