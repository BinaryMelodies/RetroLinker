
#include "windows.h"

#define CLASS_NAME "SampleWindowClass"

LRESULT CALLBACK __attribute__((near_section)) MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	asm volatile("pushw\t%ss\n\tpopw\t%ds"); // work around for -mno-callee-assume-ds-ss
	switch(msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
#if 0
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
			EndPaint(hWnd, &ps);
		}
		return 0;
#endif
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int PASCAL __attribute__((near_section)) WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MessageBox(0, "Hello World!", "Message", 0);
	if(!hPrevInstance)
	{
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = MainWndProc;
		wc.hInstance = hInstance;
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszClassName = CLASS_NAME;
		RegisterClass(&wc);
	}

	HWND hWnd = CreateWindow(CLASS_NAME, "Window title", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;

	while(GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

