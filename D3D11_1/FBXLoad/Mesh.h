#pragma once
#include <wrl/client.h> // comptr
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <d3d11_1.h>
#include <directxtk/SimpleMath.h> // directXmath 대신 사용
#include "../Common/Helper.h"

using namespace std;
using namespace DirectX::SimpleMath;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct Vertex
{
    Vector3 position;
    Vector2 texture;

    Vector3 tenget;
    Vector3 bitenget;
    Vector3 normal;
};

struct Texture
{
	string type;
	string path;

	ComPtr<ID3D11ShaderResourceView> pTexture = nullptr;
};

class Mesh
{
public:
	vector<Vertex> vertices;
	vector<UINT> indices;
	vector<Texture> textures;
	ComPtr<ID3D11Device> m_pDevice;

	Mesh(ComPtr<ID3D11Device>& dev, const std::vector<Vertex>& vertices, const std::vector<UINT>& indices, const std::vector<Texture>& textures) :
		vertices(vertices),
		indices(indices),
		textures(textures),
		m_pDevice(dev),
		m_pVertexBuffer(nullptr),
		m_pIndexBuffer(nullptr) {
		this->setupMesh(this->m_pDevice);
	}

    void Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext)
    {
        UINT stride = sizeof(Vertex);
        UINT offset = 0;

        pDeviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
        pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

        pDeviceContext->PSSetShaderResources(0, 1, textures[0].pTexture.GetAddressOf());

        pDeviceContext->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
    }

private:
	ComPtr<ID3D11Buffer> m_pVertexBuffer;
	ComPtr<ID3D11Buffer> m_pIndexBuffer;

    // Functions
    // Initializes all the buffer objects/arrays
    void setupMesh(ComPtr<ID3D11Device>& dev)
    {
        HRESULT hr;

        D3D11_BUFFER_DESC vbd;
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        vbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.CPUAccessFlags = 0;
        vbd.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = &vertices[0];

        HR_T(dev->CreateBuffer(&vbd, &initData, m_pVertexBuffer.GetAddressOf()));

        D3D11_BUFFER_DESC ibd;
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        ibd.ByteWidth = static_cast<UINT>(sizeof(UINT) * indices.size());
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.CPUAccessFlags = 0;
        ibd.MiscFlags = 0;

        initData.pSysMem = &indices[0];

        HR_T(dev->CreateBuffer(&ibd, &initData, m_pIndexBuffer.GetAddressOf()));
    }
};