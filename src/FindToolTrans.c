//
//  DockTransPanel.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include "WinSpy.h"

#include "resource.h"


HBITMAP LoadPNGImage(UINT id, void **bits);

#define WC_TRANSWINDOW  TEXT("TransWindow")

HBITMAP MakeDockPanelBitmap(RECT *rect)
{
	int width = GetRectWidth(rect);
	int height = GetRectHeight(rect);

	DWORD *pdwBox;

	static HBITMAP hbmBox, hbm;
	HDC     hdcBox, hdcDIB, hdcSrc;
	HANDLE  hOldBox, hOldDIB;

	// 32bpp bitmap
	BITMAPINFOHEADER bih = { sizeof(bih) };

	bih.biWidth = width;
	bih.biHeight = height;
	bih.biPlanes = 1;
	bih.biBitCount = 32;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = 0;

	hdcSrc = GetDC(0);

	if (hbmBox == 0)
	{
		hbmBox = LoadPNGImage(IDB_SELBOX, (void **)&pdwBox);
	}

	hdcBox = CreateCompatibleDC(hdcSrc);
	hOldBox = SelectObject(hdcBox, hbmBox);

	hbm = CreateDIBSection(hdcSrc, (BITMAPINFO *)&bih, DIB_RGB_COLORS, (void**)&pdwBox, 0, 0);
	hdcDIB = CreateCompatibleDC(hdcSrc);
	hOldDIB = SelectObject(hdcDIB, hbm);

    // corners
    BitBlt(hdcDIB, 0, 0, 32, 32, hdcBox, 0, 0, SRCCOPY);
    BitBlt(hdcDIB, width - 32, 0, 32, 32, hdcBox, 32, 0, SRCCOPY);
    BitBlt(hdcDIB, 0, height - 32, 32, 32, hdcBox, 0, 32, SRCCOPY);
    BitBlt(hdcDIB, width - 32, height - 32, 32, 32, hdcBox, 32, 32, SRCCOPY);

    // sides
    StretchBlt(hdcDIB, 0, 32, 32, height - 64, hdcBox, 0, 32, 32, 1, SRCCOPY);
    StretchBlt(hdcDIB, width - 32, 32, 32, height - 64, hdcBox, 32, 32, 32, 1, SRCCOPY);
    StretchBlt(hdcDIB, 32, 0, width - 64, 32, hdcBox, 32, 0, 1, 32, SRCCOPY);
    StretchBlt(hdcDIB, 32, height - 32, width - 64, 32, hdcBox, 32, 32, 1, 32, SRCCOPY);

    // middle
    StretchBlt(hdcDIB, 32, 32, width - 64, height - 64, hdcBox, 32, 32, 1, 1, SRCCOPY);

	SelectObject(hdcDIB, hOldDIB);
	SelectObject(hdcBox, hOldBox);

	DeleteDC(hdcBox);
	DeleteDC(hdcDIB);

	ReleaseDC(0, hdcSrc);
	return hbm;
}

void UpdatePanelTrans(HWND hwndPanel, RECT *rect)
{
	POINT ptZero = { 0, 0 };
	COLORREF crKey = RGB(0, 0, 0);

	const BYTE SourceConstantAlpha = 220;//255;
	BLENDFUNCTION blendPixelFunction = { AC_SRC_OVER, 0, 0, AC_SRC_ALPHA };
	blendPixelFunction.SourceConstantAlpha = SourceConstantAlpha;

	POINT pt;
	pt.x = rect->left;
	pt.y = rect->top;
	SIZE sz;
	sz.cx = GetRectWidth(rect);
	sz.cy = GetRectHeight(rect);

	HDC hdcSrc = GetDC(0);
	HDC hdcMem = CreateCompatibleDC(hdcSrc);
	HBITMAP hbm;
	HANDLE hold;

	hbm = MakeDockPanelBitmap(rect);
	hold = SelectObject(hdcMem, hbm);

	UpdateLayeredWindow(hwndPanel,
		hdcSrc,
		&pt, //pos
		&sz, //size
		hdcMem,
		&ptZero,
		crKey,
		&blendPixelFunction,
		ULW_ALPHA);

	SelectObject(hdcMem, hold);
	DeleteDC(hdcMem);
	ReleaseDC(0, hdcSrc);

    DeleteObject(hbm);
}

//
//  Very simple window-procedure for the transparent window
//  all the drawing happens via the DOCKPANEL WM_TIMER,
//  and calls to UpdateLayeredWindow with a transparent PNG graphic
//
LRESULT CALLBACK TransWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NCHITTEST:
		return HTTRANSPARENT;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

ATOM InitTrans()
{
	WNDCLASSEX wc = { sizeof(wc) };

	wc.style = 0;
	wc.lpszClassName = WC_TRANSWINDOW;
	wc.lpfnWndProc = TransWndProc;

	return RegisterClassEx(&wc);
}


HWND ShowTransWindow(HWND hwnd)//, RECT *rect)
{
	HWND hwndTransPanel;
	RECT r, rect;

	__try
	{
		GetWindowRect(hwnd, &r);
		rect = r;

		InitTrans();

		hwndTransPanel = CreateWindowEx(
			WS_EX_TOOLWINDOW | WS_EX_LAYERED,
			WC_TRANSWINDOW,
			0,
			WS_POPUP,
			r.left, r.top,
			r.right - r.left,
			r.bottom - r.top,
			0, 0, 0, &rect);

		UpdatePanelTrans(hwndTransPanel, &r);

		SetWindowPos(hwndTransPanel, HWND_TOPMOST,
			0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);

		return hwndTransPanel;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0;
	}
}