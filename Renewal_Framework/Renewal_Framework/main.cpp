#include "stdafx.h"
#include "Win32Application.h"
#include "DirectXProgram.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DirectXProgram m_Program{ FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, L"Renewal_Framework_Program_1" };
	return Win32Application::Run(&m_Program, hInstance, nCmdShow);
}