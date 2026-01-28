#include "Canvas.h"
#include "../../Common/Helper.h"
#include "UIBase.h"

void Canvas::Render(ComPtr<ID3D11DeviceContext>& context)
{
	context->OMSetBlendState(uiOverlayBS.Get(), nullptr, 0xffffffff);	//

	// ia
	context->IASetInputLayout(uiInputLayout.Get());
	context->IASetIndexBuffer(uiIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	UINT stride = vbStride;
	UINT offset = vbOffset;
	context->IASetVertexBuffers(0, 1, uiVertexBuffer.GetAddressOf(), &stride, &offset);

	// vs rs
	context->PSSetSamplers(0, 1, SamplerState.GetAddressOf());
	context->RSSetState(cwCullModeRS.Get());
	context->VSSetShader(uiVertexShader.Get(), nullptr, 0);

	// NOTE : 만약 9-slice나 특정 효과를 줄려하면 캠버스 내에서 조절하기
	//		  너무 많으면 분리 고려.
	//		  일단은 Quad 그리는거에서 안벗어날 예정임

	// 이건 Render쪽으로 옮겨야함
	// context->PSSetShaderResources(10, 1, m_D2DTexture.GetAddressOf());
	// context->PSSetShader(m_d2dPixelShader.Get(), nullptr, 0);

	// Restore opaque blend state for next frame
	context->OMSetDepthStencilState(uiDSS.Get(), 1);

	// 이미지 컴포넌트 드로우 호출
	for (auto& e : uiComps)
	{
		e->Render(context);				// ps, 상수버퍼, 텍스처, 설정하기
		context->DrawIndexed(6, 0, 0);	// 쿼드 그리기
	}

	context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void Canvas::GetSize(int w, int h)
{
	width = w;
	height = h;

	proj = XMMatrixOrthographicOffCenterLH(
		0.0f, (float)width,		// left, right
		(float)height, 0.0f,	// bottom, top
		0.0f, 1.0f				// near, far
	);
}

void Canvas::AddUIComp(UIBase* comp)
{
	uiComps.push_back(comp);
	comp->canvas = this;
}

void Canvas::RemoveUIComp(UIBase* base)
{
	for (auto it = uiComps.begin(); it != uiComps.end();)
	{
		if (*it == base)
		{
			(*it)->canvas = nullptr;
			uiComps.erase(it);
			break;
		}
		else
		{
			it++;
		}
	}
}

void Canvas::CreateUIEffect(ComPtr<ID3D11Device>& dev)
{
	QuadVertex QuadVertices[] = // 좌측 상단을 기준으로한 쿼드 ( 0 - 1 )
	{
		{ Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f) }, // LT
		{ Vector3(1.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f) }, // RT
		{ Vector3(0.0f, 1.0f, 0.0f), Vector2(0.0f, 1.0f) }, // LB
		{ Vector3(1.0f, 1.0f, 0.0f), Vector2(1.0f, 1.0f) }, // RB
	};

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.ByteWidth = sizeof(QuadVertex) * ARRAYSIZE(QuadVertices);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = QuadVertices;	// 배열 데이터 할당.
	HR_T(dev->CreateBuffer(&vbDesc, &vbData, uiVertexBuffer.GetAddressOf()));
	vbStride = sizeof(QuadVertex);		// 버텍스 버퍼 정보
	vbOffset = 0;

	// InputLayout 생성 	+ VS 생성
	D3D11_INPUT_ELEMENT_DESC layout[] = // 입력 레이아웃.
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ComPtr<ID3DBlob> vsBuffer{};
	HR_T(CompileShaderFromFile(L"Shaders\\VS_2D.hlsl", "main", "vs_5_0", vsBuffer.GetAddressOf()));
	HR_T(dev->CreateInputLayout(layout, ARRAYSIZE(layout), vsBuffer->GetBufferPointer(), vsBuffer->GetBufferSize(), uiInputLayout.GetAddressOf()));
	HR_T(dev->CreateVertexShader(vsBuffer->GetBufferPointer(), vsBuffer->GetBufferSize(), NULL, uiVertexShader.GetAddressOf()));

	// 인덱스 버퍼 생성
	WORD indices[] =
	{
		0,1,2,
		1,3,2
	};
	quadIndecies = ARRAYSIZE(indices);	// 인덱스 개수 저장.
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.ByteWidth = sizeof(WORD) * ARRAYSIZE(indices);
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;
	HR_T(dev->CreateBuffer(&ibDesc, &ibData, uiIndexBuffer.GetAddressOf()));
}

void Canvas::CreateStats(ComPtr<ID3D11Device>& dev)
{
	// Blend state for overlay
	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;

	D3D11_RENDER_TARGET_BLEND_DESC rtbd{};
	rtbd.BlendEnable = TRUE;

	// Color: out.rgb = src.rgb * src.a + dst.rgb * (1 - src.a)
	rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;

	// Alpha: out.a = src.a * 1 + dst.a * (1 - src.a)  (보통 이렇게 둠)
	rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;

	rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	blendDesc.RenderTarget[0] = rtbd;

	HR_T(dev->CreateBlendState(&blendDesc, uiOverlayBS.GetAddressOf()));

	// Rasterizer
	D3D11_RASTERIZER_DESC cmdesc = {};
	cmdesc.FillMode = D3D11_FILL_SOLID;
	cmdesc.CullMode = D3D11_CULL_BACK;
	cmdesc.FrontCounterClockwise = false;
	HR_T(dev->CreateRasterizerState(&cmdesc, cwCullModeRS.GetAddressOf()));

	// sampler
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR_T(dev->CreateSamplerState(&sampDesc, SamplerState.GetAddressOf()));


	// dss
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; 
	depthStencilDesc.StencilEnable = FALSE;

	dev->CreateDepthStencilState(&depthStencilDesc, uiDSS.GetAddressOf());
}

Matrix Canvas::GetProjection()
{
	return proj;
}
