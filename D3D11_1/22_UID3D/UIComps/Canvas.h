#pragma once
#include "../../Common/pch.h"

class UIBase; // 전방 선언

struct QuadVertex // vs
{
	Vector3 pos;
	Vector2 uv;

	QuadVertex(float x, float y, float z, float u, float v) : pos(x, y, z), uv(u, v) {}
	QuadVertex(Vector3 p, Vector2 u) : pos(p), uv(u) {}
};

/// <summary>
/// 모든 UI 컴포넌트가 참조 하고 있는 UI 패널 
/// </summary>
class Canvas
{
public:
	int width;	// 넓이
	int height;	// 높이

	void Render(ComPtr<ID3D11DeviceContext>& context);		// 등록된 ui 컴포넌트들 draw호출 부 
	void GetSize(int w, int h);								// 사이즈 등록
	void AddUIComp(UIBase* comp);							// ui 추가
	void RemoveUIComp(UIBase* base);						// ui 제거
	void CreateUIEffect(ComPtr<ID3D11Device>& dev);			// ui에 사용할 버퍼, 셰이더 추가
	void CreateStats(ComPtr<ID3D11Device>& dev);			// ui에 사용할 state 객체 추가

	Matrix GetProjection();

	ComPtr<ID3D11InputLayout>			uiInputLayout{};		// 인풋 레이아웃

	ComPtr<ID3D11Buffer>				uiVertexBuffer{};		// 버텍스 버퍼 -> 쿼드
	ComPtr<ID3D11Buffer>				uiIndexBuffer{};		// 픽셀 버퍼   -> 쿼드

	ComPtr<ID3D11ShaderResourceView>	uiSRV{};				// SRV

	ComPtr<ID3D11VertexShader>			uiVertexShader{};		// 버텍스 셰이더
	ComPtr<ID3D11PixelShader>			uiPixelShader{};		// 픽셀 셰이더

	ComPtr<ID3D11BlendState>			uiOverlayBS{};			// 블랜드 상태 
	ComPtr<ID3D11RasterizerState>		cwCullModeRS{};			// 컬링 모드
	ComPtr<ID3D11SamplerState>			SamplerState{};			// 샘플 상태
	ComPtr<ID3D11DepthStencilState>		uiDSS{};				// 뎊스 스탠실 상태

private:
	std::vector<UIBase*> uiComps;

	Matrix proj{};

	int vbStride;		// 크기
	int vbOffset;		// ofset
	int quadIndecies;	// 인덱스 좌표 개수
};