#include "stdafx.h"
#include "Win32Application.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return Win32Application::Run(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, hInstance, nCmdShow);
}