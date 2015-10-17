#include "renderer.h"

#include "tiny_obj_loader.h"
#include <iostream>

#include "scene.vs.h"
#include "scene.ps.h"

namespace SceneBufferBindings
{
    enum
    {
        PositionOnlyBuffer,
        PerInstanceBuffer,
        Count
    };
}

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
        assert(false);
        std::cerr << err << std::endl;
        exit(1);
    }

    {
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = (UINT) shapes[0].mesh.positions.size() * sizeof(float);
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = shapes[0].mesh.positions.data();
        initialData.SysMemPitch = (UINT) shapes[0].mesh.positions.size() * sizeof(float);
        CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, &initialData, &mpScenePositionVertexBuffer));
    }

    {
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = (UINT) shapes[0].mesh.indices.size() * sizeof(uint32_t);
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        static_assert(std::is_same<uint32_t, unsigned int>::value, "assuming unsigned int is uint32_t");

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = shapes[0].mesh.indices.data();
        initialData.SysMemPitch = (UINT) shapes[0].mesh.indices.size() * sizeof(uint32_t);
        CHECK_HR(mpDevice->CreateBuffer(&bufferDesc, &initialData, &mpSceneIndexBuffer));
    }

    {
        D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS drawArgs{};
        drawArgs.IndexCountPerInstance = (UINT) shapes[0].mesh.indices.size();
        drawArgs.InstanceCount = 1;
        mSceneDrawArgs.push_back(drawArgs);
    }

    CHECK_HR(mpDevice->CreatePixelShader(g_scene_ps, sizeof(g_scene_ps), NULL, &mpScenePixelShader));
    CHECK_HR(mpDevice->CreateVertexShader(g_scene_vs, sizeof(g_scene_vs), NULL, &mpSceneVertexShader));

    D3D11_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,    SceneBufferBindings::PositionOnlyBuffer, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "MODELWORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "MODELWORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "MODELWORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "MODELWORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, SceneBufferBindings::PerInstanceBuffer,  48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };

    CHECK_HR(mpDevice->CreateInputLayout(inputElementDescs, _countof(inputElementDescs), g_scene_vs, sizeof(g_scene_vs), &mpSceneInputLayout));
}

void Renderer::RenderFrame(ID3D11RenderTargetView* pRTV, int width, int height)
{
    mpDeviceContext->OMSetRenderTargets(1, &pRTV, NULL);
    
    D3D11_VIEWPORT viewport{};
    viewport.Width = (FLOAT) width;
    viewport.Height = (FLOAT) height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    mpDeviceContext->RSSetViewports(1, &viewport);

    ID3D11Buffer* pSceneVertexBuffers[SceneBufferBindings::Count]{};
    UINT sceneStrides[SceneBufferBindings::Count]{};
    UINT sceneOffsets[SceneBufferBindings::Count]{};
    
    pSceneVertexBuffers[SceneBufferBindings::PositionOnlyBuffer] = mpScenePositionVertexBuffer.Get();
    sceneStrides[SceneBufferBindings::PositionOnlyBuffer] = sizeof(float) * 3;

    // TODO: hook up instance buffer
    
    mpDeviceContext->IASetVertexBuffers(0, _countof(pSceneVertexBuffers), pSceneVertexBuffers, sceneStrides, sceneOffsets);
    mpDeviceContext->IASetIndexBuffer(mpSceneIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    mpDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mpDeviceContext->VSSetShader(mpSceneVertexShader.Get(), NULL, 0);
    mpDeviceContext->IASetInputLayout(mpSceneInputLayout.Get());

    mpDeviceContext->PSSetShader(mpScenePixelShader.Get(), NULL, 0);

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