#pragma once
#include "stdafx.h"

class DirectXProgram;
class Win32Application
{
private:
    static HWND m_hwnd;
protected:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
public:
    static int Run(DirectXProgram* d3dProgram, HINSTANCE hInstance, int nCmdShow);
    static HWND GetHwnd() { return m_hwnd; }
};


