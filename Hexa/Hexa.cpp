#include <Windows.h>
#include <mmsystem.h>
#include "resource.h"

#pragma comment(lib,"winmm.lib")

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PauseChildProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
LPCTSTR lpszClass = TEXT("Hexa");
HWND hWndMain, hPauseChild;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	HACCEL hAccel;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(COLOR_WINDOW + 1);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	WndClass.lpfnWndProc = PauseChildProc;
	WndClass.lpszClassName = TEXT("PauseChild");
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_SAVEBITS;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	while (GetMessage(&Message, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccel, &Message))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}
	return (int)Message.wParam;
}

#define random(n) (rand()%n)
#define TS 24
#define BW 10
#define BH 20

enum{ EMPTY, B1, B2, B3, B4, B5, B6, B7, B8, B9, B10, WALL };
int board[22][34];
int nx, ny;
int brick[3];
int nbrick[3];
int score;
int bricknum;
int level;
typedef enum tag_Status{ GAMEOVER, RUNNING, PAUSE }Status;
Status GameStatus;
int interval;
HBITMAP hBit[12];
BOOL bShowSpace = TRUE;
BOOL bQuiet = FALSE;
int flag;

void DrawScreen(HDC hdc);
void MakeNewBrick();
int GetAround(int x, int y);
BOOL MoveDown();
void TestFull();
void PrintTile(HDC hdc, int x, int y, int c);
void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit);
void PlayEffectSound(UINT Sound);
void AdjustMainWindow();
void UpdateBoard();
BOOL IsMovingBrick(int x, int y);

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	int i;
	//RECT crt;
	int t;
	HDC hdc;
	PAINTSTRUCT ps;
	int x, y;
	switch (iMessage)
	{
	case WM_CREATE:
		hWndMain = hWnd;
		hPauseChild = CreateWindow(TEXT("PauseChild"), NULL, WS_CHILD | WS_BORDER, 0, 0, 0, 0, hWnd, (HMENU)0, g_hInst, NULL);
		AdjustMainWindow();
		GameStatus = GAMEOVER;
		srand(GetTickCount());
		for (i = 0; i < 12; i++)
		{
			hBit[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1 + i));
		}
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_GAMESTART:
			if (GameStatus != GAMEOVER)
			{
				break;
			}
			for (x = 0; x < BW + 2; x++)
			{
				for (y = 0; y < BH + 2; y++)
				{
					board[x][y] = (y == 0 || y == BH + 1 || x == 0 || x == BW + 1) ? WALL : EMPTY;
				}
			}
			score = 0;
			bricknum = 0;
			level = 6;
			GameStatus = RUNNING;
			do
			{
				for (i = 0; i < 3; i++)
				{
					nbrick[i] = random(level) + 1;
				}
			} while (nbrick[0] == nbrick[1] && nbrick[1] == nbrick[2]);
			MakeNewBrick();
			interval = 1000;
			SetTimer(hWnd, 1, interval, NULL);
			break;
		case IDM_GAMEPAUSE:
			if (GameStatus == RUNNING)
			{
				GameStatus = PAUSE;
				SetTimer(hWnd, 1, 1000, NULL);
				ShowWindow(hPauseChild, SW_SHOW);
			}
			else if (GameStatus == PAUSE)
			{
				GameStatus = RUNNING;
				SetTimer(hWnd, 1, interval, NULL);
				ShowWindow(hPauseChild, SW_HIDE);
			}
			break;
		case IDM_VIEWSPACE:
			bShowSpace = !bShowSpace;
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_QUIET:
			bQuiet = !bQuiet;
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		}
		return 0;
	case WM_TIMER:
		if (GameStatus == RUNNING)
		{
			if (MoveDown() == TRUE)
			{
				MakeNewBrick();
			}
		}
		else
		{
			if (IsWindowVisible(hPauseChild))
			{
				ShowWindow(hPauseChild, SW_HIDE);
			}
			else
			{
				ShowWindow(hPauseChild, SW_SHOW);
			}
		}
		return 0;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			if (GetAround(nx - 1, ny) == EMPTY)
			{
				if ((lParam & 0x40000000) == 0)
				{
					PlayEffectSound(IDR_WAVE2);
				}
				nx--;
				UpdateBoard();
			}
			break;
		case VK_RIGHT:
			if (GetAround(nx + 1, ny) == EMPTY)
			{
				if ((lParam & 0x40000000) == 0)
				{
					PlayEffectSound(IDR_WAVE2);
				}
				nx++;
				UpdateBoard();
			}
			break;
		case VK_UP:
			PlayEffectSound(IDR_WAVE4);
			t = brick[0];
			brick[0] = brick[1];
			brick[1] = brick[2];
			brick[2] = t;
			UpdateBoard();
			break;
		case VK_DOWN:
			if (MoveDown() == TRUE)
			{
				MakeNewBrick();
			}
			break;
		case VK_SPACE:
			PlayEffectSound(IDR_WAVE1);
			while (MoveDown() == FALSE){ ; }
			MakeNewBrick();
			break;
		case 'U':
			if (level < 10)
			{
				level++;
				InvalidateRect(hWnd, NULL, FALSE);
			}
			break;
		case 'D':
			if (level > 2)
			{
				level--;
				InvalidateRect(hWnd, NULL, FALSE);
			}
			break;
		}
		return 0;
	case WM_INITMENU:
		CheckMenuItem((HMENU)wParam, IDM_VIEWSPACE, MF_BYCOMMAND | (bShowSpace ? MF_CHECKED : MF_UNCHECKED));
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawScreen(hdc);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		KillTimer(hWnd, 1);
		for (i = 0; i < 12; i++)
		{
			DeleteObject(hBit[i]);
		}
		PostQuitMessage(0);
		return 0;
	}
	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}

LRESULT CALLBACK PauseChildProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT crt;
	TEXTMETRIC tm;
	switch (iMessage)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, &crt);
		SetTextAlign(hdc, TA_CENTER);
		GetTextMetrics(hdc, &tm);
		TextOut(hdc, crt.right / 2, crt.bottom / 2 - tm.tmHeight / 2, TEXT("Pause"), 5);
		EndPaint(hWnd, &ps);
		return 0;
	}
	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}

void DrawScreen(HDC hdc)
{
	int x, y, i;
	int index = 0;
	TCHAR str[128];
	for (x = 0; x < BW + 2; x++)
	{
		PrintTile(hdc, x, 0, WALL);
		PrintTile(hdc, x, BH + 1, WALL);
	}
	for (y = 0; y < BH + 2; y++)
	{
		PrintTile(hdc, 0, y, WALL);
		PrintTile(hdc, BW + 1, y, WALL);
	}
	for (x = 1; x < BW + 1; x++)
	{
		for (y = 1; y < BH + 1; y++)
		{
			if (IsMovingBrick(x, y))
			{
				PrintTile(hdc, x, y, brick[index]);
				index++;
			}
			else
			{
				PrintTile(hdc, x, y, board[x][y]);
			}
		}
	}
	if (GameStatus != GAMEOVER && flag != -1)
	{
		for (i = 0; i < 3; i++)
		{
			PrintTile(hdc, nx, ny + i, brick[i]);
		}
	}
	for (x = BW + 3; x <= BW + 11; x++)
	{
		for (y = BH - 5; y <= BH + 1; y++)
		{
			if (x == BW + 3 || x == BW + 11 || y == BH - 5 || y == BH + 1)
			{
				PrintTile(hdc, x, y, WALL);
			}
			else
			{
				PrintTile(hdc, x, y, EMPTY);
			}
		}
	}
	if (GameStatus != GAMEOVER)
	{
		for (i = 0; i < 3; i++)
		{
			PrintTile(hdc, BW + 7, BH - 2 + i, nbrick[i]);
		}
	}
	lstrcpy(str, TEXT("Hexa"));
	TextOut(hdc, (BW + 4)*TS, 30, str, lstrlen(str));
	wsprintf(str, TEXT("점수 : %d     "), score);
	TextOut(hdc, (BW + 4)*TS, 60, str, lstrlen(str));
	wsprintf(str, TEXT("벽돌 : %d 개     "), bricknum);
	TextOut(hdc, (BW + 4)*TS, 80, str, lstrlen(str));
	wsprintf(str, TEXT("레벨(Up: U/Down: D) : %d   "), level);
	TextOut(hdc, (BW + 4)*TS, 100, str, lstrlen(str));
}

void MakeNewBrick()
{
	bricknum++;
	flag = 0;
	memcpy(brick, nbrick, sizeof(brick));
	nx = BW / 2;
	ny = 3;
	int i;
	do
	{
		for (i = 0; i < 3; i++)
		{
			nbrick[i] = random(level) + 1;
		}
	} while (nbrick[0] == nbrick[1] && nbrick[1] == nbrick[2]);
	InvalidateRect(hWndMain, NULL, FALSE);
	if (GetAround(nx, ny) != EMPTY)
	{
		KillTimer(hWndMain, 1);
		GameStatus = GAMEOVER;
		MessageBox(hWndMain, TEXT("게임이 끝났습니다. 다시 시작하려면 게임/시작 항목(S)를 선택해 주십시오."), TEXT("알림"), MB_OK);
	}
}

int GetAround(int x, int y)
{
	int i, k = EMPTY;
	for (i = 0; i < 3; i++)
	{
		k = max(k, board[x][y + i]);
	}
	return k;
}

BOOL MoveDown()
{
	if (GetAround(nx, ny + 1) != EMPTY)
	{
		TestFull();
		return TRUE;
	}
	ny++;
	UpdateBoard();
	UpdateWindow(hWndMain);
	return FALSE;
}

void TestFull()
{
	int i, x, y;
	int t, ty;
	static int arScoreInc[] = { 0, 1, 3, 7, 15, 30, 100, 500 };
	int count = 0;
	BOOL Mark[BW + 2][BH + 2];
	BOOL Remove;
	for (i = 0; i < 3; i++)
	{
		board[nx][ny + i] = brick[i];
	}
	memset(Mark, 0, sizeof(Mark));
	Remove = FALSE;
	flag = -1;
	for (;;)
	{
		for (y = 1; y < BH + 1; y++)
		{
			for (x = 1; x < BW + 1; x++)
			{
				t = board[x][y];
				if (t == EMPTY) continue;
				if (board[x - 1][y] == t && board[x + 1][y] == t)
				{
					for (i = -1; i <= 1; i++)
					{
						Mark[x + i][y] = TRUE;
					}
					Remove = TRUE;
				}
				if (board[x][y - 1] == t && board[x][y + 1] == t)
				{
					for (i = -1; i <= 1; i++)
					{
						Mark[x][y + i] = TRUE;
					}
					Remove = TRUE;
				}
				if (board[x - 1][y - 1] == t && board[x + 1][y + 1] == t)
				{
					for (i = -1; i <= 1; i++)
					{
						Mark[x + i][y + i] = TRUE;
					}
					Remove = TRUE;
				}
				if (board[x + 1][y - 1] == t && board[x - 1][y + 1] == t)
				{
					for (i = -1; i <= 1; i++)
					{
						Mark[x - i][y + i] = TRUE;
					}
					Remove = TRUE;
				}
			}
		}
		if (Remove == FALSE) break;
		for (i = 0; i < 6; i++)
		{
			for (y = 1; y < BH + 1; y++)
			{
				for (x = 1; x < BW + 1; x++)
				{
					if (board[x][y] != EMPTY && Mark[x][y] == TRUE)
					{
						HDC hdc = GetDC(hWndMain);
						PrintTile(hdc, x, y, i % 2 ? EMPTY : board[x][y]);
						ReleaseDC(hWndMain, hdc);
					}
				}
			}
			Sleep(150);
		}
		for (y = 1; y < BH + 1; y++)
		{
			for (x = 1; x < BW + 1; x++)
			{
				if (board[x][y] != EMPTY && Mark[x][y] == TRUE)
				{
					for (ty = y; ty > 1; ty--)
					{
						board[x][ty] = board[x][ty - 1];
					}
				}
			}
		}
		PlayEffectSound(IDR_WAVE3);
		count++;
		UpdateBoard();
		UpdateWindow(hWndMain);
		Sleep(150);
		Remove = FALSE;
		memset(Mark, 0, sizeof(Mark));
	}
	score += arScoreInc[count];
	if (bricknum % 10 == 0 && interval>200)
	{
		interval -= 50;
		SetTimer(hWndMain, 1, interval, NULL);
	}
}

void PrintTile(HDC hdc, int x, int y, int c)
{
	DrawBitmap(hdc, x*TS, y*TS, hBit[c]);
	if (c == EMPTY && bShowSpace)
	{
		Rectangle(hdc, x*TS + TS / 2 - 1, y*TS + TS / 2 - 1, x*TS + TS / 2 + 1, y*TS + TS / 2 + 1);
	}
}

void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit)
{
	HDC MemDC;
	HBITMAP OldBitmap;
	int bx, by;
	BITMAP bit;
	MemDC = CreateCompatibleDC(hdc);
	OldBitmap = (HBITMAP)SelectObject(MemDC, hBit);
	GetObject(hBit, sizeof(BITMAP), &bit);
	bx = bit.bmWidth;
	by = bit.bmHeight;
	BitBlt(hdc, x, y, bx, by, MemDC, 0, 0, SRCCOPY);
	SelectObject(MemDC, OldBitmap);
	DeleteDC(MemDC);
}

void PlayEffectSound(UINT Sound)
{
	if (!bQuiet)
	{
		PlaySound(MAKEINTRESOURCE(Sound), g_hInst, SND_RESOURCE | SND_ASYNC);
	}
}

void AdjustMainWindow()
{
	RECT crt;
	SetRect(&crt, 0, 0, (BW + 12)*TS, (BH + 2)*TS);
	AdjustWindowRect(&crt, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, TRUE);
	SetWindowPos(hWndMain, NULL, 0, 0, crt.right - crt.left, crt.bottom - crt.top, SWP_NOZORDER | SWP_NOMOVE);
	SetWindowPos(hPauseChild, NULL, (BW + 12)*TS / 2 - 100, (BH + 2)*TS / 2 - 50, 200, 100, SWP_NOZORDER);
}

void UpdateBoard()
{
	RECT rt;
	SetRect(&rt, TS, TS, (BW + 1)*TS, (BH + 1)*TS);
	InvalidateRect(hWndMain, &rt, FALSE);
}

BOOL IsMovingBrick(int x, int y)
{
	int i;
	if (GameStatus == GAMEOVER || flag == -1)
	{
		return FALSE;
	}
	for (i = 0; i < 3; i++)
	{
		if (x == nx && y == ny + i)
		{
			return TRUE;
		}
	}
	return FALSE;
}