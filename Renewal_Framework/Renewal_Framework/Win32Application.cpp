#include "Win32Application.h"

LRESULT Win32Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ACTIVATE:
	case WM_SIZE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_KEYDOWN:
	case WM_KEYUP:
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	default:
		return(::DefWindowProc(hWnd, message, wParam, lParam));
	}

	return 0;
}

int Win32Application::Run(int framebufferwidth, int frameBufferheight, HINSTANCE hInstance, int nCmdShow)
{
	// 윈도우 클래스 등록
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = TEXT("Renewal_Framework");
	wcex.hIconSm = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	RegisterClassEx(&wcex);

	// 윈도우 크기 계산
	RECT rc = { 0, 0, framebufferwidth, frameBufferheight };
	DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_BORDER;
	AdjustWindowRect(&rc, dwStyle, FALSE);

	// 윈도우 생성
	HWND hMainWnd = CreateWindow(TEXT("Renewal_Framework"), TEXT("Renewal_Framework_Program_1"), dwStyle, FIRST_WINDOW_POS_WIDTH, FIRST_WINDOW_POS_HEIGHT,
		rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);

	// 윈도우 출력
	ShowWindow(hMainWnd, nCmdShow);
	UpdateWindow(hMainWnd);


	// 이벤트 처리
	MSG Message;
	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	return Message.wParam;
}
