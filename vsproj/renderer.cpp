#include "renderer.h"

#include <DirectXMath.h>

#include "tiny_obj_loader.h"
#include "DDSTextureLoader.h"

#include "scene.vs.hlsl.h"
#include "scene.ps.hlsl.h"

#include "skybox.vs.hlsl.h"
#include "skybox.ps.hlsl.h"

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
        LightCBV
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
        SkyboxTextureSRV
    };
}

namespace SkyboxPSSamplerSlots
{
    enum
    {
        SkyboxSMP
    };
}

struct Material
{
    DirectX::XMFLOAT3 AmbientColor;
    DirectX::XMFLOAT3 DiffuseColor;
    DirectX::XMFLOAT3 SpecularColor;
    float Ns;
};

struct PerInstanceData
{
    DirectX::XMFLOAT4X4 ModelWorld;
    Material InstanceMaterial;
};

__declspec(align(16))
struct CameraData
{
    DirectX::XMFLOAT4X4 WorldViewProjection;
    DirectX::XMFLOAT4X4 WorldView;
    DirectX::XMFLOAT4 EyePosition;
};

__declspec(align(16))
struct LightData
{
    DirectX::XMFLOAT4 LightColor;
    DirectX::XMFLOAT4 LightPosition;
    float LightIntensity;
};

Renderer::Renderer(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext)
    : mpDevice(pDevice)
    , mpDeviceContext(pDeviceContext)
{ }

void Renderer::LoadScene()
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
        UINT totalNumInstances = (UINT)shapes.size();

        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = sizeof(PerInstanceData) * totalNumInstances;
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        std::vector<PerInstanceData> initialPerInstanceData(totalNumInstances);
        for (PerInstanceData& instance : initialPerInstanceData)
        {
            DirectX::XMStoreFloat4x4(&instance.ModelWorld, DirectX::XMMatrixIdentity());
        }

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = initialPerInstanceData.data();
        initialData.SysMemPitch = totalNumInstances * sizeof(PerInstanceData);
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
        CHECK_HR(mpDevice->CreatePixelShader(g_scene_ps, sizeof(g_scene_ps), NULL, &mpScenePixelShader));
        CHECK_HR(mpDevice->CreateVertexShader(g_scene_vs, sizeof(g_scene_vs), NULL, &mpSceneVertexShader));

        D3D11_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,    SceneBufferBindings::PositionOnlyBuffer, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,    SceneBufferBindings::NormalBuffer,	    0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "MODELWORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "MODELWORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "MODELWORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "MODELWORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };

        CHECK_HR(mpDevice->CreateInputLayout(inputElementDescs, _countof(inputElementDescs), g_scene_vs, sizeof(g_scene_vs), &mpSceneInputLayout));

        D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.FrontCounterClockwise = TRUE;
        CHECK_HR(mpDevice->CreateRasterizerState(&rasterizerDesc, &mpSceneRasterizerState));

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
        CHECK_HR(mpDevice->CreateDepthStencilState(&depthStencilDesc, &mpSceneDepthStencilState));
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
        CHECK_HR(mpDevice->CreateVertexShader(g_skybox_vs, sizeof(g_skybox_vs), NULL, &mpSkyboxVertexShader));
        CHECK_HR(mpDevice->CreatePixelShader(g_skybox_ps, sizeof(g_skybox_ps), NULL, &mpSkyboxPixelShader));

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
        bufferDesc.ByteWidth = sizeof(LightData);
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, NULL, &mpLightBuffer));
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

void Renderer::RenderFrame(ID3D11RenderTargetView* pRTV)
{
    // Update camera
    {
        D3D11_MAPPED_SUBRESOURCE mappedCamera;
        CHECK_HR(mpDeviceContext->Map(mpCameraBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedCamera));

        CameraData* pCamera = (CameraData*)mappedCamera.pData;

        static float x = 0.0f;
        x += 0.005f;
        DirectX::XMVECTOR eye = DirectX::XMVectorSet(50.0f * cos(x), 0.0f, 50.0f * sin(x), 1.0f);
        DirectX::XMVECTOR center = DirectX::XMVectorSet(0.0f, 5.0f, 0.0f, 1.0f);
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        DirectX::XMMATRIX worldView = DirectX::XMMatrixLookAtLH(eye, center, up);
        DirectX::XMMATRIX viewProjection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(70.0f), (float)mClientWidth / mClientHeight, 0.01f, 1000.0f);
        DirectX::XMMATRIX worldViewProjection = worldView * viewProjection;

        DirectX::XMStoreFloat4x4(&pCamera->WorldViewProjection, DirectX::XMMatrixTranspose(worldViewProjection));
        DirectX::XMStoreFloat4x4(&pCamera->WorldView, DirectX::XMMatrixTranspose(worldView));
        DirectX::XMStoreFloat4(&pCamera->EyePosition, eye);

        mpDeviceContext->Unmap(mpCameraBuffer.Get(), 0);
    }

    // Update light
    {
        D3D11_MAPPED_SUBRESOURCE mappedLight;
        CHECK_HR(mpDeviceContext->Map(mpLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedLight));

        LightData* pLight = (LightData*)mappedLight.pData;

        DirectX::XMFLOAT4 lightColor(0.7f, 0.4f, 0.1f, 1.0f);
        DirectX::XMFLOAT4 lightPosition(0.f, -10.f, 0.f, 1.f);
        static float x = 0.0f;
        x += 0.01f;
        float lightIntensity = (sin(x) + 1.f) * (sin(x) / 1.5f) + (2.f / 3.f);

        pLight->LightColor = lightColor;
        pLight->LightPosition = lightPosition;
        pLight->LightIntensity = lightIntensity;

        mpDeviceContext->Unmap(mpLightBuffer.Get(), 0);
    }

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
        mpDeviceContext->OMSetDepthStencilState(mpSkyboxDepthStencilState.Get(), 0);
        mpDeviceContext->VSSetConstantBuffers(SkyboxVSConstantBufferSlots::CameraCBV, 1, mpCameraBuffer.GetAddressOf());
        mpDeviceContext->PSSetShaderResources(SkyboxPSShaderResourceSlots::SkyboxTextureSRV, 1, mpSkyboxTextureSRV.GetAddressOf());
        mpDeviceContext->PSSetSamplers(SkyboxPSSamplerSlots::SkyboxSMP, 1, mpSkyboxSampler.GetAddressOf());
        mpDeviceContext->Draw(36, 0);
    }

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
    mpDeviceContext->OMSetDepthStencilState(mpSceneDepthStencilState.Get(), 0);

    mpDeviceContext->VSSetConstantBuffers(SceneVSConstantBufferSlots::CameraCBV, 1, mpCameraBuffer.GetAddressOf());
    mpDeviceContext->PSSetConstantBuffers(ScenePSConstantBufferSlots::LightCBV, 1, mpLightBuffer.GetAddressOf());

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