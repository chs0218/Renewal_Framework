#pragma once
#include "stdafx.h"

class Win32Application
{
private:
    static HWND m_hwnd;
protected:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
public:
    static int Run(int framebufferwidth, int frameBufferheight, HINSTANCE hInstance, int nCmdShow);
    static HWND GetHwnd() { return m_hwnd; }
};


