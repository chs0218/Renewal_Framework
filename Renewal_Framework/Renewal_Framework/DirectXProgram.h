#pragma once
#include "stdafx.h"

class DirectXProgram
{
private:
	UINT m_ProgramWidth;
	UINT m_ProgramHeight;
	std::wstring m_ProgramTitle;
public:
	DirectXProgram(UINT width, UINT height, std::wstring title);
	const UINT& GetProgramWidth() { return m_ProgramWidth; }
	const UINT& GetProgramHeight() { return m_ProgramHeight; }
	const std::wstring& GetProgramTitle() { return m_ProgramTitle; }
};

