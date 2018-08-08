#include<windows.h>
#include<stdio.h>
#include<time.h>
#include<d3dx9.h>
#include<dinput.h>
#include<ctype.h>

#pragma comment(lib,"winmm.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"dinput8.lib")

#define TITLE 	TEXT("Battle Run")
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define DISPLAY_WIDTH  1920
#define DISPLAY_HEIGHT 1080
#define SAFE_RELEASE(p) {if(p){(p)->Release(); (p)=NULL;}}

#define FIELD_LEFT 0
#define FIELD_TOP  0

#define MAP_01_HEIGHT	150
#define MAP_01_WIDTH	300
#define MAP_02_HEIGHT	25
#define MAP_02_WIDTH	50

#define CELL_SIZE 32

#define MOVE_SPEED 7.f

int MapData01[MAP_01_HEIGHT][MAP_01_WIDTH];
int MapData02[MAP_02_HEIGHT][MAP_02_WIDTH];

static float MoveImage;
static float MoveImage2P;

int trampolinecount;

static int Trampoline_x[10];
static int Trampoline_y[10];

int y_prev = 0, y_temp = 0;
int y_prev2P = 0, y_temp2P = 0;

struct CUSTOMVERTEX
{
	FLOAT	x, y, z, rhw;
	DWORD	color;
	FLOAT	tu, tv;
};

struct OBJECT_STATE
{
	float x, y, scale_x, scale_y;
};

enum TEXTURE
{
	PLAYER_LEFT_TEX,
	PLAYER_RIGHT_TEX,
	PLAYER_2P_LEFT_TEX,
	PLAYER_2P_RIGHT_TEX,
	GROUNDBLOCK_TEX,
	SCAFFOLD_TEX,	// 足場という意味
	TRAMPOLINE_TEX,
	TEXMAX
};

enum BLOCKTYPE
{
	NONE,
	GROUNDBLOCK,
	SCAFFOLD,
	TRAMPOLINEBLOCK
};

enum PLAYER_MODE1P {
	RIGHT_DIRECTION1P,
	LEFT_DIRECTION1P,
};

enum PLAYER_MODE2P {
	RIGHT_DIRECTION2P,
	LEFT_DIRECTION2P
};

OBJECT_STATE g_Player = { 30.f,680.f,90.f,120.f };
OBJECT_STATE g_Player2P = { 30.f,680.f,90.f,120.f };
OBJECT_STATE g_Trampoline = { 0.f,0.f,32.f,32.f };

int PlayerMode1P = RIGHT_DIRECTION1P;
int PlayerMode2P = RIGHT_DIRECTION2P;

//Directx関係----------------------------
LPDIRECT3DTEXTURE9	  g_pTexture[TEXMAX];		//	画像の情報を入れておく為のポインタ配列
IDirect3DDevice9*	  g_pD3Device;				//	Direct3Dのデバイス
D3DPRESENT_PARAMETERS g_D3dPresentParameters;	//	パラメータ
D3DDISPLAYMODE		  g_D3DdisplayMode;
IDirect3D9*			  g_pDirect3D;				//	Direct3Dのインターフェイス
LPDIRECTINPUT8		  pDinput = NULL;
LPDIRECTINPUTDEVICE8  pKeyDevice = NULL;

void ReadMapData(const char* FileName,int* MapData,int MapWidth)
{
	FILE *fp1;
	char data[4];
	int c, i = 0, x = 0, y = 0;

	if ((fopen_s(&fp1, FileName, "r")) != 0)
	{
		exit(1);
	}

	while ((c = getc(fp1)) != EOF)
	{
		if (isdigit(c))
		{
			data[i] = (char)c;
			i++;
		}
		else
		{
			data[i] = '\0';
			*(MapData + y * MapWidth + x) = atoi(data);
			x++;
			i = 0;
			if (x == MapWidth) {
				y++;
				x = 0;
			}
		}
	}
	fclose(fp1);
}

void PlayerOperation(void) {

	static int prevKey[256];
	static int framecount;
	static int framecount2P;

	static bool JFlag = false;
	static bool JFlag2P = false;
	static int Jcount = 0;
	static int Jcount2P = 0;

	if (JFlag) {
		y_temp = g_Player.y;
		g_Player.y += (g_Player.y - y_prev) + 1;
		y_prev = y_temp;
		if (g_Player.y > 675)
		{
			g_Player.y = 675;
			JFlag = false;
			Jcount = 0;
		}
	}
	
	if (JFlag2P) {
		y_temp2P = g_Player2P.y;
		g_Player2P.y += (g_Player2P.y - y_prev2P) + 1;
		y_prev2P = y_temp2P;
		if (g_Player2P.y > 675)
		{
			g_Player2P.y = 675;
			JFlag2P = false;
			Jcount2P = 0;
		}
	}

	HRESULT hr = pKeyDevice->Acquire();
	if ((hr == DI_OK) || (hr == S_FALSE))
	{
		BYTE diks[256];
		pKeyDevice->GetDeviceState(sizeof(diks), &diks);

		if (diks[DIK_W] & 0x80 && !prevKey[DIK_W]) {
			Jcount++;
			if (Jcount < 3)
			{
				JFlag = true;
				y_prev = g_Player.y;
				g_Player.y = g_Player.y - 20;
			}
		}

		if (diks[DIK_UP] & 0x80 && !prevKey[DIK_UP])
		{
			Jcount2P++;
			if (Jcount2P < 3)
			{
				JFlag2P = true;
				y_prev2P = g_Player2P.y;
				g_Player2P.y = g_Player2P.y - 20;
			}
		}

		if (diks[DIK_S] & 0x80)
		{
			g_Player.y += MOVE_SPEED;
		}

		if (diks[DIK_DOWN] & 0x80)
		{
			g_Player2P.y += MOVE_SPEED;
		}

		if (diks[DIK_A] & 0x80)
		{
			g_Player.x -= MOVE_SPEED;
			PlayerMode1P = LEFT_DIRECTION1P;

			if (prevKey[DIK_A]) {

				framecount++;
				if (framecount == 3) {

					MoveImage -= 140;
					framecount = 0;
				}
			}
		}

		if (diks[DIK_LEFT] & 0x80)
		{
			g_Player2P.x -= MOVE_SPEED;
			PlayerMode2P = LEFT_DIRECTION2P;

			if (prevKey[DIK_LEFT]) {

				framecount2P++;
				if (framecount2P == 3) {

					MoveImage2P -= 140;
					framecount2P = 0;
				}
			}
		}

		if (diks[DIK_D] & 0x80)
		{
			g_Player.x += MOVE_SPEED;
			PlayerMode1P = RIGHT_DIRECTION1P;

			if (prevKey[DIK_D]) {

				framecount++;
				if (framecount == 3) {

					MoveImage += 140;
					framecount = 0;
				}
			}
		}

		if (diks[DIK_RIGHT] & 0x80)
		{
			g_Player2P.x += MOVE_SPEED;
			PlayerMode2P = RIGHT_DIRECTION2P;

			if (prevKey[DIK_RIGHT]) {

				framecount2P++;
				if (framecount2P == 3) {

					MoveImage2P += 140;
					framecount2P = 0;
				}
			}
		}

		if (diks[DIK_A] && diks[DIK_D]) {
			MoveImage = 0;
		}

		if (!diks[DIK_A] && !diks[DIK_D]) {
			MoveImage = 0;
		}

		if (diks[DIK_LEFT] && diks[DIK_RIGHT]) {
			MoveImage2P = 0;
		}

		if (!diks[DIK_LEFT] && !diks[DIK_RIGHT]) {
			MoveImage2P = 0;
		}

		prevKey[DIK_RIGHT] = diks[DIK_RIGHT];
		prevKey[DIK_LEFT] = diks[DIK_LEFT];
		prevKey[DIK_UP] = diks[DIK_UP];
		prevKey[DIK_A] = diks[DIK_A];
		prevKey[DIK_D] = diks[DIK_D];
		prevKey[DIK_W] = diks[DIK_W];
	}
}

void PlayerDecision(void) {
	
	for (int i = 0; i < trampolinecount; i++) {

		if ((Trampoline_x[i] < g_Player.x + g_Player.scale_x) &&
				(Trampoline_x[i] + g_Trampoline.scale_x > g_Player.x) &&
				(Trampoline_y[i] < g_Player.y + g_Player.scale_y) &&
				(Trampoline_y[i] + g_Trampoline.scale_y > g_Player.y)) {
				g_Player.y -= 300;
		}
	}
	trampolinecount = 0;
}

void GameMainControl(void) {
	
	PlayerOperation();
	PlayerDecision();
}

void GameMainRender(void) {

	int i, j;
	int TextureID;

	CUSTOMVERTEX  PLAYER[4]
	{
		{ g_Player.x, g_Player.y, 1.f, 1.f, 0xFFFFFFFF, MoveImage / 980.f, 0.f },
		{ g_Player.x + g_Player.scale_x, g_Player.y, 1.f, 1.f, 0xFFFFFFFF, (MoveImage + 120) / 980.f, 0.f },
		{ g_Player.x + g_Player.scale_x, g_Player.y + g_Player.scale_y, 1.f, 1.f, 0xFFFFFFFF, (MoveImage + 120) / 980.f, 140.f / 630.f },
		{ g_Player.x, g_Player.y + g_Player.scale_y, 1.f, 1.f, 0xFFFFFFFF, MoveImage / 980.f, 140.f / 630.f }
	};

	CUSTOMVERTEX  PLAYER2P[4]
	{
		{ g_Player2P.x, g_Player2P.y, 1.f, 1.f, 0xFFFFFFFF, MoveImage2P / 980.f, 0.f },
		{ g_Player2P.x + g_Player2P.scale_x, g_Player2P.y, 1.f, 1.f, 0xFFFFFFFF, (MoveImage2P + 120) / 980.f, 0.f },
		{ g_Player2P.x + g_Player2P.scale_x, g_Player2P.y + g_Player2P.scale_y, 1.f, 1.f, 0xFFFFFFFF, (MoveImage2P + 120) / 980.f, 140.f / 630.f },
		{ g_Player2P.x, g_Player2P.y + g_Player2P.scale_y, 1.f, 1.f, 0xFFFFFFFF, MoveImage2P / 980.f, 140.f / 630.f }
	};

	CUSTOMVERTEX  CELL[4]
	{
		{ 0.f, 0.f, 1.f, 1.f, 0xFFFFFFFF, 0.f, 0.f }, // ここに数値を代入するので
		{ 0.f, 0.f, 1.f, 1.f, 0xFFFFFFFF, 1.f, 0.f }, // x,yの数値は0で良い
		{ 0.f, 0.f, 1.f, 1.f, 0xFFFFFFFF, 1.f, 1.f },
		{ 0.f, 0.f, 1.f, 1.f, 0xFFFFFFFF, 0.f, 1.f }
	};

	CUSTOMVERTEX  TRAMPOLINE[4]
	{
		{ g_Trampoline.x, g_Trampoline.y, 1.f, 1.f, 0xFFFFFFFF, 0.f, 0.f },
		{ g_Trampoline.x + g_Trampoline.scale_x, g_Trampoline.y, 1.f, 1.f, 0xFFFFFFFF, 1.f, 0.f },
		{ g_Trampoline.x + g_Trampoline.scale_x, g_Trampoline.y + g_Trampoline.scale_y, 1.f, 1.f, 0xFFFFFFFF, 1.f, 1.f },
		{ g_Trampoline.x, g_Trampoline.y + g_Trampoline.scale_y , 1.f, 1.f, 0xFFFFFFFF, 0.f, 1.f }
	};

	//画面の消去
	g_pD3Device->Clear(
		0,
		NULL,
		D3DCLEAR_TARGET,
		D3DCOLOR_XRGB(0x00, 0x00, 0x00),
		1.0,
		0);

	//描画の開始
	g_pD3Device->BeginScene();

	switch (PlayerMode1P) {
	case RIGHT_DIRECTION1P:
		TextureID = PLAYER_RIGHT_TEX;
		break;
	case LEFT_DIRECTION1P:
		TextureID = PLAYER_LEFT_TEX;
		break;
	}
	g_pD3Device->SetTexture(0, g_pTexture[TextureID]);
	g_pD3Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, PLAYER, sizeof(CUSTOMVERTEX));

	switch (PlayerMode2P) {
	case RIGHT_DIRECTION2P:
		TextureID = PLAYER_2P_RIGHT_TEX;
		break;
	case LEFT_DIRECTION2P:
		TextureID = PLAYER_2P_LEFT_TEX;
		break;
	}
	g_pD3Device->SetTexture(0, g_pTexture[TextureID]);
	g_pD3Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, PLAYER2P, sizeof(CUSTOMVERTEX));

	for (j = 0; j <MAP_02_HEIGHT; j++) {
		for (i = 0; i < MAP_02_WIDTH; i++) {

			if (MapData02[j][i] == 0)
			{
				continue;
			}

			int left = FIELD_LEFT + CELL_SIZE * i;
			int top = FIELD_TOP + CELL_SIZE * j;
			CELL[0].x = left;
			CELL[0].y = top;
			CELL[1].x = left + CELL_SIZE;
			CELL[1].y = top;
			CELL[2].x = left + CELL_SIZE;
			CELL[2].y = top + CELL_SIZE;
			CELL[3].x = left;
			CELL[3].y = top + CELL_SIZE;

			switch (MapData02[j][i]) {
				
			case GROUNDBLOCK:
				TextureID = GROUNDBLOCK_TEX;
				break;
			case SCAFFOLD:
				TextureID = SCAFFOLD_TEX;
				break;
			case TRAMPOLINEBLOCK:
				TextureID = TRAMPOLINE_TEX;
				CELL[0].x = Trampoline_x[trampolinecount] = left - 32;
				CELL[0].y = Trampoline_y[trampolinecount] = top;
				CELL[1].x = left + CELL_SIZE + 32;
				CELL[1].y = top;
				CELL[2].x = left + CELL_SIZE + 32;
				CELL[2].y = top + CELL_SIZE;
				CELL[3].x = left - 32;
				CELL[3].y = top + CELL_SIZE;
				trampolinecount++;
				break;
			}
			g_pD3Device->SetTexture(0, g_pTexture[TextureID]);
			g_pD3Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, CELL, sizeof(CUSTOMVERTEX));
		}
	}

	//描画の終了
	g_pD3Device->EndScene();
	//表示
	g_pD3Device->Present(NULL, NULL, NULL, NULL);
}

void CreateTexture(void) {

	D3DXCreateTextureFromFile(
		g_pD3Device,
		"Player_Move_Right.png",
		&g_pTexture[PLAYER_RIGHT_TEX]);

	D3DXCreateTextureFromFile(
		g_pD3Device,
		"Player_Move_Left.png",
		&g_pTexture[PLAYER_LEFT_TEX]);

	D3DXCreateTextureFromFile(
		g_pD3Device,
		"Player_Move_Right.png",
		&g_pTexture[PLAYER_2P_RIGHT_TEX]);

	D3DXCreateTextureFromFile(
		g_pD3Device,
		"Player_Move_Left.png",
		&g_pTexture[PLAYER_2P_LEFT_TEX]);

	D3DXCreateTextureFromFile(
		g_pD3Device,
		"block.png",
		&g_pTexture[SCAFFOLD_TEX]);

	D3DXCreateTextureFromFile(
		g_pD3Device,
		"groundblock.jpg",
		&g_pTexture[GROUNDBLOCK_TEX]);

	D3DXCreateTextureFromFile(
		g_pD3Device,
		"trampoline.png",
		&g_pTexture[TRAMPOLINE_TEX]);
}

HRESULT InitDinput(HWND hWnd)
{
	HRESULT hr;
	//directinputオブジェクトの作成
	if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL),
		DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&pDinput, NULL)))
	{
		return hr;
	}
	//directinputデバイスオブジェクトの作成
	if (FAILED(hr = pDinput->CreateDevice(GUID_SysKeyboard,
		&pKeyDevice, NULL)))
	{
		return hr;
	}

	//デバイスをキーボードに設定
	if (FAILED(hr = pKeyDevice->SetDataFormat(&c_dfDIKeyboard)))
	{
		return hr;
	}
	//協調レベルの設定
	if (FAILED(hr = pKeyDevice->SetCooperativeLevel(
		hWnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)))
	{
		return hr;
	}
	pKeyDevice->Acquire();
	return S_OK;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void FreeDx()
{
	for (int i = 0; i < TEXMAX; i++)
	{
		SAFE_RELEASE(g_pTexture[i]);
	}

	SAFE_RELEASE(g_pD3Device);
	SAFE_RELEASE(g_pDirect3D);
	SAFE_RELEASE(pKeyDevice);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS wc;
	MSG msg;

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;

	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = __FILE__;

	if (!RegisterClass(&wc))return 0;

	HWND hWnd = CreateWindow(
		__FILE__,
		TITLE,
		(WS_OVERLAPPEDWINDOW) | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		DISPLAY_WIDTH,
		DISPLAY_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);
	if (hWnd == NULL) return 0;

	//DirectX オブジェクトの生成
	g_pDirect3D = Direct3DCreate9(
		D3D_SDK_VERSION);
	
	InitDinput(hWnd);

	//Display Mode の設定
	g_pDirect3D->GetAdapterDisplayMode(
		D3DADAPTER_DEFAULT,
		&g_D3DdisplayMode);

	ZeroMemory(&g_D3dPresentParameters,
		sizeof(D3DPRESENT_PARAMETERS));
	g_D3dPresentParameters.BackBufferFormat = g_D3DdisplayMode.Format;
	g_D3dPresentParameters.BackBufferCount = 1;
	g_D3dPresentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_D3dPresentParameters.Windowed = TRUE;

	//デバイスを作る
	g_pDirect3D->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&g_D3dPresentParameters, &g_pD3Device);

	//描画設定
	g_pD3Device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	g_pD3Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);  //SRCの設定
	g_pD3Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	g_pD3Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pD3Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);

	g_pD3Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	g_pD3Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	//頂点に入れるデータを設定
	g_pD3Device->SetFVF(D3DFVF_CUSTOMVERTEX);

	CreateTexture();
	ReadMapData("BigField.csv", &MapData01[0][0], MAP_01_WIDTH);
	ReadMapData("Book2.csv", &MapData02[0][0], MAP_02_WIDTH);

	DWORD SyncOld = timeGetTime();
	DWORD SyncNow;

	timeBeginPeriod(1);
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		Sleep(1);
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			SyncNow = timeGetTime();
			if (SyncNow - SyncOld >= 1000 / 60)
			{
				GameMainControl();
				GameMainRender();
				SyncOld = SyncNow;
			}				
		}
	}
	timeEndPeriod(1);

	FreeDx();

	return 0;
}
