#include "renderer.h"
#include "PointLight.h"

#include "tiny_obj_loader.h"
#include "DDSTextureLoader.h"

#include "scene.vs.hlsl.h"
#include "scene.ps.hlsl.h"

#include "skybox.vs.hlsl.h"
#include "skybox.ps.hlsl.h"

#include "water.vs.hlsl.h"
#include "water.ps.hlsl.h"

#include "fog.vs.hlsl.h"
#include "fog.gs.hlsl.h"
#include "fog.ps.hlsl.h"

#include <iostream>
#include <algorithm>

namespace SceneBufferBindings
{
    enum
    {
        PositionOnlyBuffer,
        NormalBuffer,
        PerInstanceBuffer,
        Count
    };
}

namespace SceneVSConstantBufferSlots
{
    enum
    {
        CameraCBV
    };
}

namespace ScenePSConstantBufferSlots
{
    enum
    {
        LightCBV,
        MaterialCBV
    };
}

namespace SkyboxVSConstantBufferSlots
{
    enum
    {
        CameraCBV
    };
}

namespace SkyboxPSShaderResourceSlots
{
    enum
    {
        SkyboxTextureSRV,
    };
}

namespace SkyboxPSConstantBufferSlots
{
	enum
	{
		SkyboxLightCBV
    };
}

namespace SkyboxPSSamplerSlots
{
    enum
    {
        SkyboxSMP
    };
}

namespace WaterVSConstantBufferSlots
{
    enum
    {
        CameraCBV
    };
}

namespace WaterPSConstantBufferSlots
{
    enum
    {
        TimeCBV,
		LightningCBV
    };
}

namespace WaterPSShaderResourceSlots
{
    enum
    {
        WaterDepthSRV
    };
}

namespace WaterPSSamplerSlots
{
    enum
    {
        WaterDepthSMP
    };
}

namespace FogBufferBindingSlots
{
    enum
    {
        PerParticleData,
        Count
    };
}

namespace FogGSConstantBufferSlots
{
    enum
    {
        CameraCBV
    };
}

namespace FogPSShaderResourceSlots
{
    enum
    {
        FogTextureSRV
    };
}

namespace FogPSSamplerSlots
{
    enum
    {
        FogSMP
    };
}

__declspec(align(16))
struct CameraData
{
    DirectX::XMFLOAT4X4 WorldViewProjection;
    DirectX::XMFLOAT4X4 WorldView;
    DirectX::XMFLOAT4X4 ViewProjection;
    DirectX::XMFLOAT4 EyePosition;
};

__declspec(align(16))
struct TimeData
{
    float TimeSinceStart_sec;
};

Renderer::Renderer(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext)
    : mpDevice(pDevice)
    , mpDeviceContext(pDeviceContext)
    , mFogRNG(std::random_device()())
{
    mTotalFogParticlesMade = 0;
    mTimeSinceStart_sec = 0.0;

    float t = (float)mTimeSinceStart_sec;
    float lightIntensity = (sin(t) + 1.f) * (sin(t) / 1.5f) + (2.f / 3.f);
    for (PointLight& pL : mLightVector) {
        pL.SetIntensity(lightIntensity);
    }

    mSkullPosition = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    mSkullLookDirection = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
}

void Renderer::Init()
{
    std::string inputfile = "Models/skull.obj";
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err = tinyobj::LoadObj(shapes, materials, inputfile.c_str(), "Models/");

    if (!err.empty()) {
        OutputDebugStringA(err.c_str());
        assert(false);
        exit(1);
    }

    for (size_t i = 0; i < shapes.size(); i++)
    {
        // so we know what mesh instances need to be moved when keyboard keys are pressed
        mSkullInstances.push_back(i);

        if (shapes[i].name == "jaw" || shapes[i].name == "lteeth")
        {
            mSkullJawInstances.push_back(i);
    }
    }

    // Create position vertex staging buffer
    {
        D3D11_BUFFER_DESC stagingBufferDesc{};
        UINT bufferSize = 0;
        for (int i = 0; i < shapes.size(); ++i) {
            bufferSize += (UINT)shapes[i].mesh.positions.size() * sizeof(float);
        }
        stagingBufferDesc.ByteWidth = bufferSize;
        stagingBufferDesc.Usage = D3D11_USAGE_STAGING;
        stagingBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ComPtr<ID3D11Buffer> stagingBuffer;
        CHECK_HR(mpDevice->CreateBuffer(&stagingBufferDesc, NULL, &stagingBuffer));

        D3D11_MAPPED_SUBRESOURCE mappedBuffer;
        CHECK_HR(mpDeviceContext->Map(stagingBuffer.Get(), 0, D3D11_MAP_WRITE, 0, &mappedBuffer));

        float* pData = (float*)mappedBuffer.pData;
        for (int i = 0; i < shapes.size(); ++i) {
            memcpy(pData, shapes[i].mesh.positions.data(), sizeof(float) * shapes[i].mesh.positions.size());
            pData += shapes[i].mesh.positions.size();
        }

        mpDeviceContext->Unmap(stagingBuffer.Get(), 0);

        // Create vertex position buffer
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = bufferSize;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, NULL, &mpScenePositionVertexBuffer));
        mpDeviceContext->CopyResource(mpScenePositionVertexBuffer.Get(), stagingBuffer.Get());
    }

    // Create vertex normal staging buffer
    {
        D3D11_BUFFER_DESC stagingBufferDesc{};
        UINT bufferSize = 0;
        for (int i = 0; i < shapes.size(); ++i) {
            bufferSize += (UINT)shapes[i].mesh.normals.size() * sizeof(float);
        }
        stagingBufferDesc.ByteWidth = bufferSize;
        stagingBufferDesc.Usage = D3D11_USAGE_STAGING;
        stagingBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ComPtr<ID3D11Buffer> stagingBuffer;
        CHECK_HR(mpDevice->CreateBuffer(&stagingBufferDesc, NULL, &stagingBuffer));

        D3D11_MAPPED_SUBRESOURCE mappedBuffer;
        CHECK_HR(mpDeviceContext->Map(stagingBuffer.Get(), 0, D3D11_MAP_WRITE, 0, &mappedBuffer));

        float* pData = (float*)mappedBuffer.pData;
        for (int i = 0; i < shapes.size(); ++i) {
            memcpy(pData, shapes[i].mesh.normals.data(), sizeof(float) * shapes[i].mesh.normals.size());
            pData += shapes[i].mesh.normals.size();
        }

        mpDeviceContext->Unmap(stagingBuffer.Get(), 0);

        // Create vertex normal buffer
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = bufferSize;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, NULL, &mpScenePositionNormalBuffer));
        mpDeviceContext->CopyResource(mpScenePositionNormalBuffer.Get(), stagingBuffer.Get());
    }

    // Create position vertex staging buffer
    {
        D3D11_BUFFER_DESC stagingBufferDesc{};
        UINT bufferSize = 0;
        for (int i = 0; i < shapes.size(); ++i) {
            bufferSize += (UINT)shapes[i].mesh.indices.size() * sizeof(uint32_t);
        }
        stagingBufferDesc.ByteWidth = bufferSize;
        stagingBufferDesc.Usage = D3D11_USAGE_STAGING;
        stagingBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ComPtr<ID3D11Buffer> stagingBuffer;
        CHECK_HR(mpDevice->CreateBuffer(&stagingBufferDesc, NULL, &stagingBuffer));

        D3D11_MAPPED_SUBRESOURCE mappedBuffer;
        CHECK_HR(mpDeviceContext->Map(stagingBuffer.Get(), 0, D3D11_MAP_WRITE, 0, &mappedBuffer));

        uint32_t* pData = (uint32_t*)mappedBuffer.pData;
        for (int i = 0; i < shapes.size(); ++i) {
            memcpy(pData, shapes[i].mesh.indices.data(), sizeof(uint32_t) * shapes[i].mesh.indices.size());
            pData += shapes[i].mesh.indices.size();
        }

        mpDeviceContext->Unmap(stagingBuffer.Get(), 0);

        // Create vertex position buffer
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = bufferSize;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, NULL, &mpSceneIndexBuffer));
        mpDeviceContext->CopyResource(mpSceneIndexBuffer.Get(), stagingBuffer.Get());
    }

    // Create instance buffer
    {
        mNumTotalMeshInstances = shapes.size();

        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = (UINT)(sizeof(PerInstanceData) * mNumTotalMeshInstances);
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // initialize instance data
        mCPUSceneInstanceBuffer.resize(mNumTotalMeshInstances);
        for (UINT i = 0; i < mNumTotalMeshInstances; ++i)
        {
            PerInstanceData& instance = mCPUSceneInstanceBuffer.at(i);
            instance.materialID = i;
        }

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = mCPUSceneInstanceBuffer.data();
        initialData.SysMemPitch = (UINT)(mNumTotalMeshInstances * sizeof(PerInstanceData));
        CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, &initialData, &mpSceneInstanceBuffer));
    }

    // Create the list of draws to render the scene
    {
        UINT startIndex = 0;
        UINT baseIndex = 0;
        for (int i = 0; i < shapes.size(); ++i) {
            D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS drawArgs{};
            drawArgs.IndexCountPerInstance = (UINT)shapes[i].mesh.indices.size();
            drawArgs.InstanceCount = 1;
            drawArgs.StartIndexLocation = startIndex;
            drawArgs.BaseVertexLocation = baseIndex;
            drawArgs.StartInstanceLocation = i;
            mSceneDrawArgs.push_back(drawArgs);
            startIndex += (UINT)shapes[i].mesh.indices.size();
            baseIndex += (UINT)shapes[i].mesh.positions.size() / 3;
        }
    }

    // Create pipeline state
    {
        CHECK_HR(mpDevice->CreateVertexShader(g_scene_vs, sizeof(g_scene_vs), NULL, &mpSceneVertexShader));
        CHECK_HR(mpDevice->CreatePixelShader(g_scene_ps, sizeof(g_scene_ps), NULL, &mpScenePixelShader));

        D3D11_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,    SceneBufferBindings::PositionOnlyBuffer, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,    SceneBufferBindings::NormalBuffer,	    0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "MODELWORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "MODELWORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "MODELWORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "MODELWORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "MATERIALID", 4, DXGI_FORMAT_R32_FLOAT,          SceneBufferBindings::PerInstanceBuffer,  64, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
        };

        CHECK_HR(mpDevice->CreateInputLayout(inputElementDescs, _countof(inputElementDescs), g_scene_vs, sizeof(g_scene_vs), &mpSceneInputLayout));

        D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.FrontCounterClockwise = TRUE;
        CHECK_HR(mpDevice->CreateRasterizerState(&rasterizerDesc, &mpSceneRasterizerState));

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
        CHECK_HR(mpDevice->CreateDepthStencilState(&depthStencilDesc, &mpSceneDepthStencilState));
    }

    // Create material data
    {
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = sizeof(Material);
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, NULL, &mpMaterialBuffer));
    }

    // Load material data into member vector
    {
        for (int i = 0; i < materials.size(); ++i) {
            Material mat;
            DirectX::XMFLOAT3 ambient(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
            DirectX::XMFLOAT3 diffuse(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
            DirectX::XMFLOAT3 specular(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
            mat.AmbientColor = ambient;
            mat.DiffuseColor = diffuse;
            mat.SpecularColor = specular;
            mat.Ns = materials[i].shininess;
            mMaterialVector.push_back(mat);
        }
    }

    {
        // Load lights into light buffer. TODO: Add functionality for importing light data from somewhere.
        PointLight tlight;

        tlight.SetColor(0.7f, 0.4f, 0.1f, 1.0f);
        tlight.SetPosition(0.f, -10.f, 1.f);
        tlight.SetIntensity(1.f);
        tlight.SetAmbientColor(0.2f, 0.0f, 1.0f, 1.0f);

        mLightVector.push_back(tlight);
    }

    // Time buffer
    {
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = sizeof(TimeData);
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, NULL, &mpTimeBuffer));
    }

    // Create camera data
    {
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = sizeof(CameraData);
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, NULL, &mpCameraBuffer));
    }

    // Load skybox texture
    {
        CHECK_HR(DirectX::CreateDDSTextureFromFileEx(
            mpDevice, mpDeviceContext,
            L"Skyboxes/grimmnight.dds",
            (size_t)0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, true,
            &mpSkyboxTexture, &mpSkyboxTextureSRV, nullptr));

        D3D11_SAMPLER_DESC skyboxSamplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
        skyboxSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
        CHECK_HR(mpDevice->CreateSamplerState(&skyboxSamplerDesc, &mpSkyboxSampler));
    }

    // Create skybox pipeline state
    {
        CHECK_HR(mpDevice->CreateVertexShader(g_skybox_vs, sizeof(g_skybox_vs), NULL, &mpSkyboxVertexShader));
        CHECK_HR(mpDevice->CreatePixelShader(g_skybox_ps, sizeof(g_skybox_ps), NULL, &mpSkyboxPixelShader));

        D3D11_RASTERIZER_DESC skyboxRasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
        skyboxRasterizerDesc.CullMode = D3D11_CULL_NONE;
        CHECK_HR(mpDevice->CreateRasterizerState(&skyboxRasterizerDesc, &mpSkyboxRasterizerState));

        D3D11_DEPTH_STENCIL_DESC skyboxDepthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
        skyboxDepthStencilDesc.DepthEnable = FALSE;
        CHECK_HR(mpDevice->CreateDepthStencilState(&skyboxDepthStencilDesc, &mpSkyboxDepthStencilState));
    }

    // Create light data
    {
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = sizeof(LightData) * (UINT)mLightVector.size();
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, NULL, &mpLightBuffer));
    }

    // Create water pipeline state
    {
        CHECK_HR(mpDevice->CreateVertexShader(g_water_vs, sizeof(g_water_vs), NULL, &mpWaterVertexShader));
        CHECK_HR(mpDevice->CreatePixelShader(g_water_ps, sizeof(g_water_ps), NULL, &mpWaterPixelShader));

        D3D11_BLEND_DESC waterBlendDesc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
        waterBlendDesc.RenderTarget[0].BlendEnable = true;
        waterBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        waterBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

        CHECK_HR(mpDevice->CreateBlendState(&waterBlendDesc, &mpWaterBlendState));
    }

    // Load water texture
    {
        CHECK_HR(DirectX::CreateDDSTextureFromFileEx(
            mpDevice, mpDeviceContext,
            L"Skyboxes/water.DDS",
            (size_t)0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, false,
            &mpWaterDepthTexture, &mpWaterDepthTextureSRV, nullptr));

        D3D11_SAMPLER_DESC waterSamplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
        waterSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        waterSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        CHECK_HR(mpDevice->CreateSamplerState(&waterSamplerDesc, &mpWaterDepthSampler));
    }

    // Fog pipeline state
    {
        CHECK_HR(mpDevice->CreateVertexShader(g_fog_vs, sizeof(g_fog_vs), NULL, &mpFogVertexShader));
        CHECK_HR(mpDevice->CreateGeometryShader(g_fog_gs, sizeof(g_fog_gs), NULL, &mpFogGeometryShader));
        CHECK_HR(mpDevice->CreatePixelShader(g_fog_ps, sizeof(g_fog_ps), NULL, &mpFogPixelShader));

        D3D11_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, FogBufferBindingSlots::PerParticleData, offsetof(FogParticleData, WorldPosition),  D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "INTENSITY", 0, DXGI_FORMAT_R32_FLOAT,       FogBufferBindingSlots::PerParticleData, offsetof(FogParticleData, Intensity),  D3D11_INPUT_PER_VERTEX_DATA,   0 },
        };

        CHECK_HR(mpDevice->CreateInputLayout(inputElementDescs, _countof(inputElementDescs), g_fog_vs, sizeof(g_fog_vs), &mpFogInputLayout));
    }

    // Load fog texture
    {
        CHECK_HR(DirectX::CreateDDSTextureFromFileEx(
            mpDevice, mpDeviceContext,
            L"Skyboxes/fog.DDS",
            (size_t)0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, true,
            &mpFogTexture, &mpFogTextureSRV, nullptr));

        D3D11_SAMPLER_DESC fogSamplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
        CHECK_HR(mpDevice->CreateSamplerState(&fogSamplerDesc, &mpFogSampler));
    }
}

void Renderer::Resize(int width, int height)
{
    mClientWidth = width;
    mClientHeight = height;

    // Create depth buffer
    {
        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_D32_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        CHECK_HR(mpDevice->CreateTexture2D(&textureDesc, NULL, &mpSceneDepthBuffer));

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        CHECK_HR(mpDevice->CreateDepthStencilView(mpSceneDepthBuffer.Get(), &dsvDesc, &mpSceneDSV));
    }
}

void Renderer::HandleEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
        if (wParam == 'W') {
            mForwardHeld = true;
        }
        if (wParam == 'S') {
            mBackwardHeld = true;
        }
        if (wParam == 'A') {
            mRotateLeftHeld = true;
        }
        if (wParam == 'D') {
            mRotateRightHeld = true;
        }
        break;
    case WM_KEYUP:
        if (wParam == 'W') {
            mForwardHeld = false;
}
        if (wParam == 'S') {
            mBackwardHeld = false;
        }
        if (wParam == 'A') {
            mRotateLeftHeld = false;
        }
        if (wParam == 'D') {
            mRotateRightHeld = false;
        }
        break;
    }
}

void Renderer::Update(int deltaTime_ms)
{
    float deltaTime_sec = deltaTime_ms * 0.001f;
    mTimeSinceStart_sec += deltaTime_sec;

    static const size_t kMaxParticlesToMake = 1000;
    static const float kAvgTimeToDeath = 10.0f;

    while (mFogCPUParticles.size() < kMaxParticlesToMake)
    {
        FogParticleData particle{};
        particle.Intensity = std::max(0.8f, std::abs(mFogDistribution(mFogRNG) + 1));
        float x = mFogDistribution(mFogRNG) * 25.0f;
        float y = mFogDistribution(mFogRNG);
        float z = mFogDistribution(mFogRNG) * 25.0f;
        DirectX::XMStoreFloat3(&particle.WorldPosition, DirectX::XMVectorSet(x, y, z, 1.0f));
        mFogCPUParticles.push_back(particle);
    }

    // Update particles
    for (size_t i = 0; i < mFogCPUParticles.size(); i++)
    {
        mFogCPUParticles[i].Intensity -= deltaTime_sec / kAvgTimeToDeath;
        mFogCPUParticles[i].WorldPosition.y = std::sin(mFogCPUParticles[i].Intensity * 2);
    }

    mFogCPUParticles.erase(std::remove_if(begin(mFogCPUParticles), end(mFogCPUParticles),
        [](const FogParticleData& part) { return part.Intensity <= 0.0f; }), mFogCPUParticles.end());


    // Update skull position/direction
    {
        static const float kForwardSpeed = 5.0f;
        
        float forwardMovementAmount = (mForwardHeld - mBackwardHeld) * kForwardSpeed;
        DirectX::XMVECTOR forwardMovement = DirectX::XMVectorScale(mSkullLookDirection, forwardMovementAmount * deltaTime_sec);
        mSkullPosition = DirectX::XMVectorAdd(mSkullPosition, forwardMovement);

        static const float kRotateSpeed = 1.0f;

        float rotationAmount = -(mRotateLeftHeld - mRotateRightHeld) * kRotateSpeed * deltaTime_sec;
        DirectX::XMVECTOR rotationQuat = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationAmount);
        mSkullLookDirection = DirectX::XMVector3Normalize(DirectX::XMVector3Rotate(mSkullLookDirection, rotationQuat));
    }

	// Deal with lightning
	if (mLightning.IsFlashing()) {
		mLightning.doFlash(deltaTime_ms);
	}
	else {
		mLightning.GenerateFlash();
	}

	// Update lights
	for (UINT i = 0; i < (UINT)mLightVector.size(); ++i) {
		float t = (float)mTimeSinceStart_sec;
		float lightIntensity = (sin(t) + 1.f) * (sin(t) / 1.5f) + (2.f / 3.f);
		mLightVector.at(i).SetIntensity(lightIntensity);
	}
}

void Renderer::RenderFrame(ID3D11RenderTargetView* pRTV, const OrbitCamera& camera)
{
    static bool DebugOutput = false;
    // Update camera
    {
        D3D11_MAPPED_SUBRESOURCE mappedCamera;
        CHECK_HR(mpDeviceContext->Map(mpCameraBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedCamera));

        CameraData* pCamera = (CameraData*)mappedCamera.pData;

        DirectX::XMVECTOR eye = camera.Eye();
        DirectX::XMVECTOR center = DirectX::XMVectorSet(0.0f, 5.0f, 0.0f, 1.0f);
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        DirectX::XMMATRIX worldView = camera.WorldView();
        DirectX::XMMATRIX viewProjection = camera.ViewProjection();
        DirectX::XMMATRIX worldViewProjection = camera.WorldViewProjection();

        DirectX::XMStoreFloat4x4(&pCamera->WorldViewProjection, DirectX::XMMatrixTranspose(worldViewProjection));
        DirectX::XMStoreFloat4x4(&pCamera->WorldView, DirectX::XMMatrixTranspose(worldView));
        DirectX::XMStoreFloat4x4(&pCamera->ViewProjection, DirectX::XMMatrixTranspose(viewProjection));
        DirectX::XMStoreFloat4(&pCamera->EyePosition, eye);

        mpDeviceContext->Unmap(mpCameraBuffer.Get(), 0);
    }

    // Update lights
    {
        D3D11_MAPPED_SUBRESOURCE mappedLight;
        CHECK_HR(mpDeviceContext->Map(mpLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedLight));

        LightData* pLight = (LightData*)mappedLight.pData;

        for (int i = 0; i < mLightVector.size(); ++i) {

			/*
			PointLight PointLightInstance = mLightVector.at(i);

			DirectX::XMFLOAT4 lightColor = PointLightInstance.GetColor();
			DirectX::XMFLOAT4 lightPosition = PointLightInstance.GetPosition();
			DirectX::XMFLOAT4 ambColor = PointLightInstance.GetAmbientColor();
			float lightIntensity = PointLightInstance.GetIntensity();

			pLight->LightColor = lightColor;
			pLight->AmbientLightColor = PointLightInstance.GetAmbientColor();
			pLight->LightPosition = PointLightInstance.GetPosition();
			pLight->LightIntensity = PointLightInstance.GetIntensity();

			*/

            DirectX::XMFLOAT4 lightColor(0.7f, 0.4f, 0.1f, 1.0f);
            DirectX::XMFLOAT4 lightPosition(0.f, -10.f, 1.f, 1.f);
            float t = (float)mTimeSinceStart_sec;
            float lightIntensity = (sin(t) + 1.f) * (sin(t) / 1.5f) + (2.f / 3.f);

            DirectX::XMFLOAT4 ambColor(0.2f, 0.0f, 1.0f, 1.0f);

            pLight->LightColor = lightColor;
            pLight->LightPosition = lightPosition;
            pLight->LightIntensity = lightIntensity;
            pLight->AmbientLightColor = ambColor;
			pLight->LightningIntensity = mLightning.GetIntensity();

            pLight += sizeof(LightData);

            if (!DebugOutput) {
                std::cout << "Light Information" << '\n';
                std::cout << "Position: " << pLight->LightPosition.x << ", " << pLight->LightPosition.y << ", " << pLight->LightPosition.z << '\n';
                std::cout << "Color: " << pLight->LightColor.x << ", " << pLight->LightColor.y << ", " << pLight->LightColor.z << '\n';
                std::cout << "Intensity: " << pLight->LightIntensity << '\n';
                std::cout << "Ambient Color: " << pLight->AmbientLightColor.x << ", " << pLight->AmbientLightColor.y << ", " << pLight->AmbientLightColor.z << '\n';
            }
        }

        mpDeviceContext->Unmap(mpLightBuffer.Get(), 0);
    }

    // Update time
    {
        D3D11_MAPPED_SUBRESOURCE mappedTime;
        CHECK_HR(mpDeviceContext->Map(mpTimeBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedTime));

        TimeData* pTime = (TimeData*)mappedTime.pData;

        pTime->TimeSinceStart_sec = (float)mTimeSinceStart_sec;

        mpDeviceContext->Unmap(mpTimeBuffer.Get(), 0);
    }

    // Do this in each draw - load material buffer with appropriate material data
    {
        D3D11_MAPPED_SUBRESOURCE mappedMaterial;
        CHECK_HR(mpDeviceContext->Map(mpMaterialBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedMaterial));

        Material* pMaterial = (Material*)mappedMaterial.pData;

        Material* mat = &mMaterialVector.at(0);

        memcpy(pMaterial, mat, sizeof(Material));

        if (!DebugOutput) {
            std::cout << "Material Information" << '\n';
            std::cout << "Ambient Color: " << pMaterial->AmbientColor.x << ", " << pMaterial->AmbientColor.y << ", " << pMaterial->AmbientColor.z << '\n';
            std::cout << "Diffuse Color: " << pMaterial->DiffuseColor.x << ", " << pMaterial->DiffuseColor.y << ", " << pMaterial->DiffuseColor.z << '\n';
            std::cout << "Specular Color: " << pMaterial->SpecularColor.x << ", " << pMaterial->SpecularColor.y << ", " << pMaterial->SpecularColor.z << '\n';
            std::cout << "Shininess: " << pMaterial->Ns << '\n';

        }

        mpDeviceContext->Unmap(mpMaterialBuffer.Get(), 0);
    }

    DebugOutput = true;

    mpDeviceContext->OMSetRenderTargets(1, &pRTV, mpSceneDSV.Get());

    float kClearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    mpDeviceContext->ClearRenderTargetView(pRTV, kClearColor);
    mpDeviceContext->ClearDepthStencilView(mpSceneDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    D3D11_VIEWPORT viewport{};
    viewport.Width = (FLOAT)mClientWidth;
    viewport.Height = (FLOAT)mClientHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    mpDeviceContext->RSSetViewports(1, &viewport);

    // Draw skybox
    {
        mpDeviceContext->VSSetShader(mpSkyboxVertexShader.Get(), NULL, 0);
        mpDeviceContext->PSSetShader(mpSkyboxPixelShader.Get(), NULL, 0);
        mpDeviceContext->IASetInputLayout(nullptr);
        mpDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        mpDeviceContext->RSSetState(mpSkyboxRasterizerState.Get());
        mpDeviceContext->OMSetBlendState(NULL, NULL, UINT_MAX);
        mpDeviceContext->OMSetDepthStencilState(mpSkyboxDepthStencilState.Get(), 0);
        mpDeviceContext->VSSetConstantBuffers(SkyboxVSConstantBufferSlots::CameraCBV, 1, mpCameraBuffer.GetAddressOf());
        mpDeviceContext->PSSetShaderResources(SkyboxPSShaderResourceSlots::SkyboxTextureSRV, 1, mpSkyboxTextureSRV.GetAddressOf());
		mpDeviceContext->PSSetConstantBuffers(SkyboxPSConstantBufferSlots::SkyboxLightCBV, 1, mpLightBuffer.GetAddressOf());
        mpDeviceContext->PSSetSamplers(SkyboxPSSamplerSlots::SkyboxSMP, 1, mpSkyboxSampler.GetAddressOf());
        mpDeviceContext->Draw(36, 0);
    }

    // Update model transforms
    {
        // update skull transforms
        for (size_t skullMeshInstanceID : mSkullInstances)
        {
            DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            DirectX::XMVECTOR across = DirectX::XMVector3Cross(up, mSkullLookDirection);
            DirectX::XMMATRIX rotation = DirectX::XMMatrixIdentity();
            rotation.r[0] = across;
            rotation.r[1] = up;
            rotation.r[2] = mSkullLookDirection;

            // skull model slightly off center
            DirectX::XMMATRIX skullRotationFixup = DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), -0.3f);

            DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(mSkullPosition);
            
            // spooky levitation
            DirectX::XMMATRIX levitation = DirectX::XMMatrixTranslation(0.0f, std::sinf((float)mTimeSinceStart_sec * 5) * 0.5f, 0.0f);

            // extra spooky levitation
            if (std::find(begin(mSkullJawInstances), end(mSkullJawInstances), skullMeshInstanceID) != end(mSkullJawInstances))
            {
                levitation = levitation * DirectX::XMMatrixTranslation(0.0f, std::abs(std::sin((float)mTimeSinceStart_sec * 4)) * -0.5f, 0.0f);
            }

            DirectX::XMMATRIX transform = skullRotationFixup * rotation * translation * levitation;
            DirectX::XMStoreFloat4x4(&mCPUSceneInstanceBuffer[skullMeshInstanceID].ModelWorld, transform);
        }

        // copy instance data
        {
            D3D11_MAPPED_SUBRESOURCE mappedInstances;
            CHECK_HR(mpDeviceContext->Map(mpSceneInstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedInstances));

            PerInstanceData* pInstances = (PerInstanceData*)mappedInstances.pData;

            for (size_t i = 0; i < mCPUSceneInstanceBuffer.size(); i++)
            {
                DirectX::XMMATRIX transform = DirectX::XMLoadFloat4x4(&mCPUSceneInstanceBuffer[i].ModelWorld);
                DirectX::XMStoreFloat4x4(&pInstances[i].ModelWorld, DirectX::XMMatrixTranspose(transform));
                pInstances[i].materialID = mCPUSceneInstanceBuffer[i].materialID;
            }

            mpDeviceContext->Unmap(mpSceneInstanceBuffer.Get(), 0);
        }
    }

    // Draw models
    {
        ID3D11Buffer* pSceneVertexBuffers[SceneBufferBindings::Count]{};
        UINT sceneStrides[SceneBufferBindings::Count]{};
        UINT sceneOffsets[SceneBufferBindings::Count]{};

        pSceneVertexBuffers[SceneBufferBindings::PositionOnlyBuffer] = mpScenePositionVertexBuffer.Get();
        sceneStrides[SceneBufferBindings::PositionOnlyBuffer] = sizeof(float) * 3;

        pSceneVertexBuffers[SceneBufferBindings::NormalBuffer] = mpScenePositionNormalBuffer.Get();
        sceneStrides[SceneBufferBindings::NormalBuffer] = sizeof(float) * 3;

        pSceneVertexBuffers[SceneBufferBindings::PerInstanceBuffer] = mpSceneInstanceBuffer.Get();
        sceneStrides[SceneBufferBindings::PerInstanceBuffer] = sizeof(PerInstanceData);

        mpDeviceContext->IASetVertexBuffers(0, _countof(pSceneVertexBuffers), pSceneVertexBuffers, sceneStrides, sceneOffsets);
        mpDeviceContext->IASetIndexBuffer(mpSceneIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        mpDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        mpDeviceContext->VSSetShader(mpSceneVertexShader.Get(), NULL, 0);
        mpDeviceContext->PSSetShader(mpScenePixelShader.Get(), NULL, 0);
        mpDeviceContext->IASetInputLayout(mpSceneInputLayout.Get());
        mpDeviceContext->RSSetState(mpSceneRasterizerState.Get());
        mpDeviceContext->OMSetBlendState(NULL, NULL, UINT_MAX);
        mpDeviceContext->OMSetDepthStencilState(mpSceneDepthStencilState.Get(), 0);

        mpDeviceContext->VSSetConstantBuffers(SceneVSConstantBufferSlots::CameraCBV, 1, mpCameraBuffer.GetAddressOf());
        mpDeviceContext->PSSetConstantBuffers(ScenePSConstantBufferSlots::LightCBV, 1, mpLightBuffer.GetAddressOf());
        mpDeviceContext->PSSetConstantBuffers(ScenePSConstantBufferSlots::MaterialCBV, 1, mpMaterialBuffer.GetAddressOf());

        for (const D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS& drawArgs : mSceneDrawArgs)
        {
            mpDeviceContext->DrawIndexedInstanced(
                drawArgs.IndexCountPerInstance,
                drawArgs.InstanceCount,
                drawArgs.StartIndexLocation,
                drawArgs.BaseVertexLocation,
                drawArgs.StartInstanceLocation);
        }
    }

    // Draw water
    {
        mpDeviceContext->VSSetShader(mpWaterVertexShader.Get(), NULL, 0);
        mpDeviceContext->PSSetShader(mpWaterPixelShader.Get(), NULL, 0);
        mpDeviceContext->IASetInputLayout(nullptr);
        mpDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        mpDeviceContext->RSSetState(nullptr);
        mpDeviceContext->OMSetBlendState(mpWaterBlendState.Get(), NULL, UINT_MAX);
        mpDeviceContext->OMSetDepthStencilState(mpSceneDepthStencilState.Get(), 0);
        mpDeviceContext->VSSetConstantBuffers(WaterVSConstantBufferSlots::CameraCBV, 1, mpCameraBuffer.GetAddressOf());
        mpDeviceContext->PSSetConstantBuffers(WaterPSConstantBufferSlots::TimeCBV, 1, mpTimeBuffer.GetAddressOf());
		mpDeviceContext->PSSetConstantBuffers(WaterPSConstantBufferSlots::LightningCBV, 1, mpLightBuffer.GetAddressOf());
        mpDeviceContext->PSSetShaderResources(WaterPSShaderResourceSlots::WaterDepthSRV, 1, mpWaterDepthTextureSRV.GetAddressOf());
        mpDeviceContext->PSSetSamplers(WaterPSSamplerSlots::WaterDepthSMP, 1, mpWaterDepthSampler.GetAddressOf());
        mpDeviceContext->Draw(6, 0);
    }

    // Draw fog
    {
        // Sort particles from back to front
        {
            std::sort(begin(mFogCPUParticles), end(mFogCPUParticles),
                [&](const FogParticleData& p1, const FogParticleData& p2)
            {
                DirectX::XMVECTOR pos1 = DirectX::XMLoadFloat3(&p1.WorldPosition);
                pos1.m128_f32[3] = 1.0f;
                DirectX::XMVECTOR pos2 = DirectX::XMLoadFloat3(&p2.WorldPosition);
                pos2.m128_f32[3] = 1.0f;

                DirectX::XMVECTOR view1 = DirectX::XMVector4Transform(pos1, camera.WorldView());
                DirectX::XMVECTOR view2 = DirectX::XMVector4Transform(pos2, camera.WorldView());

                return view1.m128_f32[2] > view2.m128_f32[2];
            });
        }

        // copy fog data in
        {
            D3D11_BUFFER_DESC bufferDesc{};
            if (mpFogGPUParticles) {
                mpFogGPUParticles->GetDesc(&bufferDesc);
            }

            if (bufferDesc.ByteWidth < mFogCPUParticles.size() * sizeof(FogParticleData))
            {
                bufferDesc.ByteWidth = (UINT)((mFogCPUParticles.size() + 100) * sizeof(FogParticleData));
                bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
                bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, NULL, &mpFogGPUParticles));
            }

            if (!mFogCPUParticles.empty())
            {
                D3D11_MAPPED_SUBRESOURCE mapped;
                CHECK_HR(mpDeviceContext->Map(mpFogGPUParticles.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));

                memcpy(mapped.pData, mFogCPUParticles.data(), mFogCPUParticles.size() * sizeof(FogParticleData));

                mpDeviceContext->Unmap(mpFogGPUParticles.Get(), 0);
            }
        }

        mpDeviceContext->VSSetShader(mpFogVertexShader.Get(), NULL, 0);
        mpDeviceContext->GSSetShader(mpFogGeometryShader.Get(), NULL, 0);
        mpDeviceContext->PSSetShader(mpFogPixelShader.Get(), NULL, 0);
        mpDeviceContext->IASetInputLayout(mpFogInputLayout.Get());
        mpDeviceContext->RSSetState(NULL);
        mpDeviceContext->OMSetBlendState(mpWaterBlendState.Get(), NULL, UINT_MAX);
        mpDeviceContext->OMSetDepthStencilState(mpSceneDepthStencilState.Get(), 0);
        mpDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        mpDeviceContext->GSSetConstantBuffers(FogGSConstantBufferSlots::CameraCBV, 1, mpCameraBuffer.GetAddressOf());
        mpDeviceContext->PSSetShaderResources(FogPSShaderResourceSlots::FogTextureSRV, 1, mpFogTextureSRV.GetAddressOf());
        mpDeviceContext->PSSetSamplers(FogPSSamplerSlots::FogSMP, 1, mpFogSampler.GetAddressOf());

        ID3D11Buffer* fogBuffers[FogBufferBindingSlots::Count]{};
        UINT fogStrides[FogBufferBindingSlots::Count]{};
        UINT fogOffsets[FogBufferBindingSlots::Count]{};

        fogBuffers[FogBufferBindingSlots::PerParticleData] = mpFogGPUParticles.Get();
        fogStrides[FogBufferBindingSlots::PerParticleData] = sizeof(FogParticleData);
        mpDeviceContext->IASetVertexBuffers(FogBufferBindingSlots::PerParticleData, FogBufferBindingSlots::Count, fogBuffers, fogStrides, fogOffsets);

        mpDeviceContext->Draw((UINT)mFogCPUParticles.size(), 0);
        mpDeviceContext->GSSetShader(NULL, NULL, 0);
    }
}