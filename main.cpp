//--------------------------------------------------------------------------------------
// BTH - Stefan Petersson 2014.
//--------------------------------------------------------------------------------------
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include "SimpleMath.h"
#include "bth_image.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

#define SCREEN_WIDTH 640.0f
#define SCREEN_HEIGHT 480.0f

HWND InitWindow(HINSTANCE hInstance);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

HRESULT CreateDirect3DContext(HWND wndHandle);

IDXGISwapChain* gSwapChain = nullptr;
ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gDeviceContext = nullptr;
ID3D11RenderTargetView* gBackbufferRTV = nullptr;
ID3D11DepthStencilView* gZBuffer = nullptr;
ID3D11ShaderResourceView* gTextureView = nullptr;

ID3D11Buffer* gVertexBuffer = nullptr;
ID3D11Buffer* gCBuffer = nullptr;

ID3D11InputLayout* gVertexLayout = nullptr;
ID3D11VertexShader* gVertexShader = nullptr;
ID3D11PixelShader* gPixelShader = nullptr;
ID3D11GeometryShader* gGeometryShader = nullptr;

using namespace DirectX;
using namespace DirectX::SimpleMath;



void CreateShaders()
{
	HRESULT hr;
	//create vertex shader
	ID3DBlob* pVS = nullptr, *vError;
	hr = D3DCompileFromFile(
		L"Vertex.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"VS_main",		// entry point
		"vs_4_0",		// shader model (target)
		0,				// shader compile options
		0,				// effect compile options
		&pVS,			// double pointer to ID3DBlob		
		&vError			// pointer for Error Blob messages.
		// how to use the Error blob, see here
		// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
		);
	if (vError)
		MessageBox(NULL, L"The Vertex shader failed to compile", L"Error", MB_OK);

	gDevice->CreateVertexShader(pVS->GetBufferPointer(), pVS->GetBufferSize(), nullptr, &gVertexShader);
	
	//create input layout (verified using vertex shader)
	D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	gDevice->CreateInputLayout(inputDesc, ARRAYSIZE(inputDesc), pVS->GetBufferPointer(), pVS->GetBufferSize(), &gVertexLayout);
	// we do not need anymore this COM object, so we release it.
	pVS->Release();

	//create pixel shader
	ID3DBlob* pPS = nullptr, *pError;
	hr = D3DCompileFromFile(
		L"Fragment.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"PS_main",		// entry point
		"ps_4_0",		// shader model (target)
		0,				// shader compile options
		0,				// effect compile options
		&pPS,			// double pointer to ID3DBlob		
		&pError			// pointer for Error Blob messages.
		// how to use the Error blob, see here
		// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
		);

	if (pError)
		MessageBox(NULL, L"The Pixel shader failed to compile", L"Error", MB_OK);

	gDevice->CreatePixelShader(pPS->GetBufferPointer(), pPS->GetBufferSize(), nullptr, &gPixelShader);
	// we do not need anymore this COM object, so we release it.
	pPS->Release();

	ID3DBlob* pGS = nullptr, *gError;
	hr = D3DCompileFromFile(
		L"Geometry.hlsl",
		nullptr,
		nullptr,
		"GS_main",
		"gs_4_0",
		0,
		0,
		&pGS,
		&gError
		);
		
	if (gError)
		MessageBox(NULL, L"The Geometry shader failed to compile", L"Error", MB_OK);

	gDevice->CreateGeometryShader(pGS->GetBufferPointer(), pGS->GetBufferSize(), nullptr, &gGeometryShader);
	pGS->Release();
}

void CreateTriangleData()
{
	struct TriangleVertex
	{
		float x, y, z;
		float u, v;
	};

	TriangleVertex triangleVertices[] =
	{
		-0.5f, 0.5f, 0.0f,	//v0 pos
		0.0f, 0.0f,			//v0 uv

		0.5f, -0.5f, 0.0f,	//v1
		1.0f, 1.0f,			//v1 uv

		-0.5f, -0.5f, 0.0f, //v2
		0.0f, 1.0f,			//v2 uv

		-0.5f, 0.5f, 0.0f,	//v3 pos
		0.0f, 0.0f,			//v3 uv

		0.5f, 0.5f, 0.0f,	//v4
		1.0f, 0.0f,			//v4 uv

		0.5f, -0.5f, 0.0f,	//v5
		1.0f, 1.0f			//v5 uv
	};
	HRESULT hr;

	D3D11_BUFFER_DESC bufferDesc; //struct w/ buffer properties
	memset(&bufferDesc, 0, sizeof(bufferDesc));
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(triangleVertices);

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = triangleVertices;
	gDevice->CreateBuffer(&bufferDesc, &data, &gVertexBuffer);

	D3D11_BUFFER_DESC cBufferDesc;
	memset(&cBufferDesc, 0, sizeof(cBufferDesc));
	cBufferDesc.ByteWidth = sizeof(Matrix) * 2;
	cBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	gDevice->CreateBuffer(&cBufferDesc, NULL, &gCBuffer);
	gDeviceContext->GSSetConstantBuffers(0, 1, &gCBuffer);

	D3D11_TEXTURE2D_DESC texDesc;
	memset(&texDesc, 0, sizeof(texDesc));

	texDesc.Width = 640;
	texDesc.Height = 480;
	texDesc.ArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D *pDepthBuffer;
	gDevice->CreateTexture2D(&texDesc, NULL, &pDepthBuffer);

	gDevice->CreateDepthStencilView(pDepthBuffer, NULL, &gZBuffer);
	pDepthBuffer->Release();

	gDeviceContext->OMSetRenderTargets(1, &gBackbufferRTV, gZBuffer);

	D3D11_TEXTURE2D_DESC bthLogoDesc;
	memset(&bthLogoDesc, 0, sizeof(bthLogoDesc));
	bthLogoDesc.Width = BTH_IMAGE_WIDTH;
	bthLogoDesc.Height = BTH_IMAGE_HEIGHT;
	bthLogoDesc.ArraySize = 1;
	bthLogoDesc.MipLevels = 1;
	bthLogoDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bthLogoDesc.SampleDesc.Count = 1;	//default
	bthLogoDesc.SampleDesc.Quality = 0;	//default
	bthLogoDesc.Usage = D3D11_USAGE_DEFAULT;
	bthLogoDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bthLogoDesc.CPUAccessFlags = 0;		//no CPU access needed
	bthLogoDesc.MiscFlags = 0;	//not needed

	ID3D11Texture2D* bthLogo;
	D3D11_SUBRESOURCE_DATA bthLogoData;
	memset(&bthLogoData, 0, sizeof(bthLogoData));
	bthLogoData.pSysMem = (void*)BTH_IMAGE_DATA;
	bthLogoData.SysMemPitch = BTH_IMAGE_WIDTH * 4; //size of char omitted because it's 1
	hr = gDevice->CreateTexture2D(&bthLogoDesc, &bthLogoData, &bthLogo);
	if (FAILED(hr))
		MessageBox(NULL, L"Failed to create texture", L"Error", MB_OK);

	D3D11_SHADER_RESOURCE_VIEW_DESC logoViewDesc;
	memset(&logoViewDesc, 0, sizeof(logoViewDesc));
	logoViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //same as logoDesc
	logoViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	logoViewDesc.Texture2D.MipLevels = 1; //same as logoDesc
	logoViewDesc.Texture2D.MostDetailedMip = 0;
	hr = gDevice->CreateShaderResourceView(bthLogo, &logoViewDesc, &gTextureView);
	if (FAILED(hr))
		MessageBox(NULL, L"Failed to create resource view", L"Error", MB_OK);
	bthLogo->Release();
}

void SetViewport()
{
	D3D11_VIEWPORT vp;
	vp.Width = SCREEN_WIDTH;
	vp.Height = SCREEN_HEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gDeviceContext->RSSetViewports(1, &vp);
}

void Render()
{
	// clear the back buffer to a deep blue
	float clearColor[] = { 0, 0, 0, 1 };
	gDeviceContext->ClearRenderTargetView(gBackbufferRTV, clearColor);

	//clears z buffer
	gDeviceContext->ClearDepthStencilView(gZBuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	static float frameUpdate = 0.0f;
	frameUpdate += 0.01f;
	float radians = frameUpdate * (XM_PI / 180.0f);

	XMMATRIX World = DirectX::XMMatrixRotationY(radians);
	World = XMMATRIX(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1) * World;

	Vector3 camPos(0, 0, -2);
	Vector3 target(0, 0, 0);
	Vector3 upDir(0, 1, 0);
	XMMATRIX View = DirectX::XMMatrixLookAtLH(camPos, target, upDir);

	XMMATRIX Projection = DirectX::XMMatrixPerspectiveFovLH(XM_PI * 0.45f, SCREEN_WIDTH / SCREEN_HEIGHT, 0.5f, 20.0f);

	XMMATRIX Transformation = XMMatrixTranspose(World * View * Projection);

	struct ConstantBuffer
	{
		XMMATRIX World;
		XMMATRIX Transformation;
	};

	ConstantBuffer cBuffer;
	cBuffer.World = World;
	cBuffer.Transformation = Transformation;

	gDeviceContext->UpdateSubresource(gCBuffer, 0, NULL, &cBuffer, 0, 0);

	gDeviceContext->VSSetShader(gVertexShader, nullptr, 0);
	gDeviceContext->HSSetShader(nullptr, nullptr, 0);
	gDeviceContext->DSSetShader(nullptr, nullptr, 0);
	gDeviceContext->GSSetShader(gGeometryShader, nullptr, 0);
	gDeviceContext->PSSetShader(gPixelShader, nullptr, 0);

	UINT32 vertexSize = sizeof(float) * 5; //xyz+uv
	UINT32 offset = 0;
	gDeviceContext->IASetVertexBuffers(0, 1, &gVertexBuffer, &vertexSize, &offset);
	gDeviceContext->GSSetConstantBuffers(0, 1, &gCBuffer);
	gDeviceContext->PSSetShaderResources(0, 1, &gTextureView);

	gDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gDeviceContext->IASetInputLayout(gVertexLayout);


	gDeviceContext->Draw(6, 0); //no of vertices, vertex start point
}

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	MSG msg = { 0 };
	HWND wndHandle = InitWindow(hInstance); //1. Skapa fönster
	
	if (wndHandle)
	{
		CreateDirect3DContext(wndHandle); //2. Skapa och koppla SwapChain, Device och Device Context

		SetViewport(); //3. Sätt viewport

		CreateShaders(); //4. Skapa vertex- och pixel-shaders

		CreateTriangleData(); //5. Definiera triangelvertiser, 6. Skapa vertex buffer, 7. Skapa input layout

		ShowWindow(wndHandle, nCmdShow);

		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				Render(); //8. Rendera

				gSwapChain->Present(0, 0); //9. Växla front- och back-buffer
			}
		}

		gVertexBuffer->Release();

		gVertexLayout->Release();
		gVertexShader->Release();
		gPixelShader->Release();

		gBackbufferRTV->Release();
		gSwapChain->Release();
		gDevice->Release();
		gDeviceContext->Release();
		DestroyWindow(wndHandle);
	}

	return (int) msg.wParam;
}

HWND InitWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.hInstance      = hInstance;
	wcex.lpszClassName = L"BTH_D3D_DEMO";
	if (!RegisterClassEx(&wcex))
		return false;

	RECT rc = { 0, 0, 640, 480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	HWND handle = CreateWindow(
		L"BTH_D3D_DEMO",
		L"BTH Direct3D Demo",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	return handle;
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch (message) 
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;		
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

HRESULT CreateDirect3DContext(HWND wndHandle)
{
	// create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = wndHandle;                           // the window to be used
	scd.SampleDesc.Count = 1;                               // how many multisamples
	scd.Windowed = TRUE;                                    // windowed/full-screen mode

	// create a device, device context and swap chain using the information in the scd struct
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&scd,
		&gSwapChain,
		&gDevice,
		NULL,
		&gDeviceContext);

	if (SUCCEEDED(hr))
	{
		// get the address of the back buffer
		ID3D11Texture2D* pBackBuffer = nullptr;
		gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

		// use the back buffer address to create the render target
		gDevice->CreateRenderTargetView(pBackBuffer, NULL, &gBackbufferRTV);
		pBackBuffer->Release();

		// set the render target as the back buffer
		//gDeviceContext->OMSetRenderTargets(1, &gBackbufferRTV, NULL);
	}
	return hr;
}