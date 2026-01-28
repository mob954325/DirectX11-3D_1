#pragma once
#include "../../Common/pch.h"

struct PerObjectCB
{
	Matrix WVP; // View Projection
};

/// <summary>
/// 모든 UI 컴포넌트가 참조 하고 있는 UI 패널 
/// </summary>
class Canvas
{
public:
	float width;	// 넓이
	float height;	// 높이

private:
	ComPtr<ID3D11ShaderResourceView>	m_UISRVB{};		// ??

	ComPtr<ID3D11Buffer>				m_D2DVertBuffer{};
	ComPtr<ID3D11Buffer>				m_D2DIndexBuffer{};

	ComPtr<ID3D11Buffer>				m_perObjCB{};

	ComPtr<ID3D11BlendState>			m_TransparencyBS{};
	ComPtr<ID3D11RasterizerState>		m_CWcullModeRS{};
	ComPtr<ID3D11SamplerState>			m_SamplerState{};

	ComPtr<ID3D11VertexShader>			m_d2dVertexShader{};
	ComPtr<ID3D11PixelShader>			m_d2dPixelShader{};

	PerObjectCB m_perObjData{};
	Matrix m_WVP{};
};