#include "Mesh.h"

void Mesh::Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext)
{
    
    // ps ÃÊ±âÈ­ 
    ID3D11ShaderResourceView* nullSRV[4] = { nullptr };
    pDeviceContext->PSSetShaderResources(0, 4, nullSRV);
    
    UINT stride = sizeof(BoneWeightVertex);
    UINT offset = 0;
    
    pDeviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
    pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);        
    
    int textureCount = textures.size();
    for (int i = 0; i < textureCount; i++)
    {
        ProcessTextureByType(pDeviceContext, i);
    }
    
    pDeviceContext->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
    
}

void Mesh::SetMaterial(aiMaterial* pAiMaterial)
{
    aiColor4D color(0, 0, 0, 0);
    if (AI_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color)) //
    {
        material.diffuse = { color.r, color.g, color.b, color.a };
    }
}

void Mesh::setupMesh(ComPtr<ID3D11Device>& dev)
{
    // check texture
    int textureCount = textures.size();
    for (int i = 0; i < textureCount; i++)
    {
        string typeName = textures[i].type;
        if (typeName == TEXTURE_DIFFUSE)
        {
            material.hasDiffuse = true;
        }
        else if (typeName == TEXTURE_EMISSIVE)
        {
            material.hasEmissive = true;
        }
        else if (typeName == TEXTURE_NORMAL)
        {
            material.hasNormal = true;
        }
        else if (typeName == TEXTURE_SPECULAR)
        {
            material.hasSpecular = true;
        }
    }
}

void Mesh::ProcessTextureByType(ComPtr<ID3D11DeviceContext>& pDeviceContext, int index)
{
    string typeName = textures[index].type;

    if (typeName == TEXTURE_DIFFUSE)
    {
        pDeviceContext->PSSetShaderResources(0, 1, textures[index].pTexture.GetAddressOf());
    }
    else if (typeName == TEXTURE_EMISSIVE)
    {
        pDeviceContext->PSSetShaderResources(1, 1, textures[index].pTexture.GetAddressOf());
    }
    else if (typeName == TEXTURE_NORMAL)
    {
        pDeviceContext->PSSetShaderResources(2, 1, textures[index].pTexture.GetAddressOf());
    }
    else if (typeName == TEXTURE_SPECULAR)
    {
        pDeviceContext->PSSetShaderResources(3, 1, textures[index].pTexture.GetAddressOf());
    }
}

Material& Mesh::GetMaterial()
{
    return material;
}

void Mesh::CreateVertexBuffer(ComPtr<ID3D11Device>& dev)
{
    // vertex buffer
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = static_cast<UINT>(sizeof(BoneWeightVertex) * vertices.size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = &vertices[0];

    HR_T(dev->CreateBuffer(&vbd, &initData, m_pVertexBuffer.GetAddressOf()));
}

void Mesh::CreateIndexBuffer(ComPtr<ID3D11Device>& dev)
{
    // index buffer
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = static_cast<UINT>(sizeof(UINT) * indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = &indices[0];
    HR_T(dev->CreateBuffer(&ibd, &initData, m_pIndexBuffer.GetAddressOf()));
}
