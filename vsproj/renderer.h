#pragma once

#include "dxutil.h"
#include "camera.h"
#include "PointLight.h"
#include "Lightning.h"
#include <DirectXMath.h>

#include <vector>
#include <random>

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

    __declspec(align(16))
    struct PerInstanceData
    {
        DirectX::XMFLOAT4X4 ModelWorld;
        UINT materialID;
    };

    struct FogParticleData
    {
        DirectX::XMFLOAT3 WorldPosition;
        float Intensity;
    };

    ID3D11Device* mpDevice;
    ID3D11DeviceContext* mpDeviceContext;

    ComPtr<ID3D11Buffer> mpScenePositionVertexBuffer;
    ComPtr<ID3D11Buffer> mpScenePositionNormalBuffer;
    ComPtr<ID3D11Buffer> mpSceneIndexBuffer;
    ComPtr<ID3D11Buffer> mpSceneInstanceBuffer;
    std::vector<PerInstanceData> mCPUSceneInstanceBuffer;
    size_t mNumTotalMeshInstances;
    std::vector<D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS> mSceneDrawArgs;
    
    std::vector<size_t> mSkullInstances;
    std::vector<size_t> mSkullJawInstances;

    ComPtr<ID3D11Buffer> mpCameraBuffer;
    ComPtr<ID3D11Buffer> mpLightBuffer;
    ComPtr<ID3D11Buffer> mpMaterialBuffer;
    ComPtr<ID3D11Buffer> mpTimeBuffer;

    double mTimeSinceStart_sec;

    std::vector<Material> mMaterialVector;
	std::vector<PointLight> mLightVector;

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

    ComPtr<ID3D11VertexShader> mpWaterVertexShader;
    ComPtr<ID3D11PixelShader> mpWaterPixelShader;
    ComPtr<ID3D11BlendState> mpWaterBlendState;

    ComPtr<ID3D11Resource> mpWaterDepthTexture;
    ComPtr<ID3D11ShaderResourceView> mpWaterDepthTextureSRV;
    ComPtr<ID3D11SamplerState> mpWaterDepthSampler;

    ComPtr<ID3D11VertexShader> mpFogVertexShader;
    ComPtr<ID3D11GeometryShader> mpFogGeometryShader;
    ComPtr<ID3D11PixelShader> mpFogPixelShader;
    ComPtr<ID3D11InputLayout> mpFogInputLayout;

    ComPtr<ID3D11Resource> mpFogTexture;
    ComPtr<ID3D11ShaderResourceView> mpFogTextureSRV;
    ComPtr<ID3D11SamplerState> mpFogSampler;

    std::vector<FogParticleData> mFogCPUParticles;
    ComPtr<ID3D11Buffer> mpFogGPUParticles;

    std::vector<FogParticleData> mCollectableCPUParticles;
    ComPtr<ID3D11Buffer> mpCollectableGPUParticles;

    ComPtr<ID3D11Resource> mpCollectableTexture;
    ComPtr<ID3D11ShaderResourceView> mpCollectableTextureSRV;

    std::mt19937 mFogRNG;
    std::normal_distribution<float> mFogDistribution;
    size_t mTotalFogParticlesMade;

	Lightning mLightning;

    int mClientWidth;
    int mClientHeight;

    bool mForwardHeld;
    bool mBackwardHeld;
    bool mRotateLeftHeld;
    bool mRotateRightHeld;

    DirectX::XMVECTOR mSkullPosition;
    float mSkullSpeed;
    DirectX::XMVECTOR mSkullLookDirection;

    std::vector<DirectX::XMVECTOR> mSkullTailPositions;
    std::vector<DirectX::XMVECTOR> mSkullTailLookDirections;

    int mNumCollectablesCOllected = 0;

public:
    Renderer(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext);

    void Init();

    void Resize(int width, int height);

    void HandleEvent(UINT message, WPARAM wParam, LPARAM lParam);

    void Update(int deltaTime_ms);

    void RenderFrame(ID3D11RenderTargetView* pRTV, const OrbitCamera& camera);
};