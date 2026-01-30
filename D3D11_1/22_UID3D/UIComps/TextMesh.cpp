#include "TextMesh.h"

void TextMesh::Init(ComPtr<ID3D11Device>& dev)
{

    // 상수 버퍼 갱신
    D3D11_BUFFER_DESC bd{};
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.ByteWidth = (UINT)((sizeof(ImageCBData) + 15) / 16 * 16); // 16바이트 정렬
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    mvp = Matrix::Identity;
}

void TextMesh::Render(ComPtr<ID3D11DeviceContext>& context)
{
}