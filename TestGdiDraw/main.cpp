#include <Windows.h>
#include "AssaultCube12_offset.h"
#include "ProcMem.h"
#include <stdio.h>

ProcMem mem;

#define WINDOW_NAME "AssaultCube"

RECT WBounds;
HWND EspHWND;
HWND GameHWND;
HPEN BoxPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));


struct Vector3 {
	float x, y, z;
};

struct view_matrix_t {
	float matrix[16];
};

struct Vector3 WorldToScreen(const struct Vector3 pos, struct view_matrix_t matrix) {
	struct Vector3 out;

	float _x = pos.x * matrix.matrix[0] + pos.y * matrix.matrix[4] + pos.z * matrix.matrix[8] + matrix.matrix[12];
	float _y = pos.x * matrix.matrix[1] + pos.y * matrix.matrix[5] + pos.z * matrix.matrix[9] + matrix.matrix[13];
	float _z = pos.x * matrix.matrix[2] + pos.y * matrix.matrix[6] + pos.z * matrix.matrix[10] + matrix.matrix[14];
	float _w = pos.x * matrix.matrix[3] + pos.y * matrix.matrix[7] + pos.z * matrix.matrix[11] + matrix.matrix[15];

	_x = _x / _w;
	_y = _y / _w;
	_z = _z / _w;

	int width = WBounds.right - WBounds.left;
	int height = WBounds.bottom - WBounds.top;

	out.x = (width / 2 * _x) + (_x + width / 2);
	out.y = -(height / 2 * _y) + (_y + height / 2);

	if (_w < 0.1f)
		out.z = -1;
	else
		out.z = 0;
	return out;
}


void Draw(HDC Mem_hdc) {
	// Get matrix
	view_matrix_t matrix = mem.read<view_matrix_t>(view_matrix);

	GetWindowRect(GameHWND, &WBounds);
	int player_num = mem.read<int>(players_in_map);

	Player bot;
	for (int i = 1; i < player_num; i++) {
		bot = mem.read<Player>(mem.read<int>(mem.read<int>(entity_base) + i * 4));
		Vector3 screen_foot = WorldToScreen({ bot.foot_x, bot.foot_y, bot.foot_z }, matrix);
		Vector3 screen_head = WorldToScreen({ bot.head_x, bot.head_y, bot.head_z }, matrix);
		if (screen_foot.z == 0 && screen_head.z == 0) {
			float bot_height = screen_foot.y - screen_head.y;
			float bot_width = bot_height / 2;
			SelectObject(Mem_hdc, BoxPen);
			Rectangle(Mem_hdc,screen_head.x - bot_width/2,screen_head.y,screen_foot.x + bot_width/2,screen_foot.y);
		}
	}

	return;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC Memhdc;
		HDC hdc;
		HBITMAP Membitmap;
		// Make our espwindow always cover game window.	
		GetWindowRect(GameHWND, &WBounds);
		MoveWindow(EspHWND, WBounds.left, WBounds.top, WBounds.right - WBounds.left, WBounds.bottom - WBounds.top, true);
		int win_width = WBounds.right - WBounds.left;
		int win_height = WBounds.bottom - WBounds.top;		
		hdc = BeginPaint(hwnd, &ps);
		Memhdc = CreateCompatibleDC(hdc);
		Membitmap = CreateCompatibleBitmap(hdc, win_width, win_height);
		SelectObject(Memhdc, Membitmap);
		// We fill with white_brush actually clear the bg, because our layer window transprent is LWA_COLORKEY mode, it make the RGB(255, 255, 255) mask to transprent.
		RECT bg_rect{ 0,0,win_width,win_height };
		FillRect(Memhdc, &bg_rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
		// Do Real Draw
		Draw(Memhdc);
		// Directly copy memhdc to esp window's hdc.
		BitBlt(hdc, 0, 0, win_width, win_height, Memhdc, 0, 0, SRCCOPY);
		// Clear.
		DeleteObject(Membitmap);
		DeleteDC(Memhdc);
		DeleteDC(hdc);
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_ERASEBKGND:
		// We use double-buffer: directly draw in Memhdc, and bitblt whole image into esp window, so don't need default erase-bg step(and bg is transparent you can't really use invalidrect to clear).
		return 1;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

DWORD UpdateLoop() {
	while (true) {
		GameHWND = FindWindowA(0, WINDOW_NAME);
		if (GameHWND == NULL) {
			ExitProcess(0);
		}
		if (GetForegroundWindow() == GameHWND) {
			ShowWindow(EspHWND, SW_SHOW);
			//Send wm_paint message to our esp window(Our esp layed won't auto send paint).
			InvalidateRect(EspHWND, NULL, true);
		}
		else {
			ShowWindow(EspHWND, SW_HIDE);
		}
		Sleep(16);
	}
	return 0;
}



int main() {
	mem.Init(NULL, WINDOW_NAME, 0);

	GameHWND = FindWindowA(0, WINDOW_NAME);
	GetWindowRect(GameHWND, &WBounds);

	WNDCLASSEX WClass;
	MSG Msg;
	WClass.cbSize = sizeof(WNDCLASSEX);
	WClass.style = CS_HREDRAW|CS_VREDRAW;
	WClass.lpfnWndProc = WndProc;
	WClass.cbClsExtra = 0;
	WClass.cbWndExtra = 0;
	WClass.hInstance = GetModuleHandle(NULL);
	WClass.hIcon = NULL;
	WClass.hCursor = NULL;
	WClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WClass.lpszMenuName = L" ";
	WClass.lpszClassName = L"ESPClass";
	WClass.hIconSm = NULL;
	RegisterClassExW(&WClass);
	EspHWND = CreateWindowExW(WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED, L"ESPClass", L"ESPWindow", WS_POPUP|WS_VISIBLE, 
		WBounds.left, WBounds.top, WBounds.right - WBounds.left, WBounds.bottom - WBounds.top, NULL, NULL, WClass.hInstance, NULL);
	SetLayeredWindowAttributes(EspHWND, RGB(255, 255, 255), 0, LWA_COLORKEY);


	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)&UpdateLoop, NULL, NULL, NULL);
	while (GetMessageA(&Msg, NULL, NULL, NULL)) {
		TranslateMessage(&Msg);
		DispatchMessageA(&Msg);
	}
	return 0;
}
