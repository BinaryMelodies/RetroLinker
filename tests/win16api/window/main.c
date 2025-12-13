
#include "windows.h"

#define CLASS_NAME "SampleWindowClass"
#define MESSAGE "Welcome to RetroLinker! (16-bit)"

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lresult;
	asm volatile("pushw\t%ss\n\tpopw\t%ds"); // work around for -mno-callee-assume-ds-ss
	asm volatile("incw\t(%bp)"); // make stack walking possible
	switch(msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		lresult = 0;
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			TextOut(hdc, 10, 10, MESSAGE, sizeof MESSAGE - 1);
			EndPaint(hWnd, &ps);
		}
		lresult = 0;
		break;
	default:
		lresult = DefWindowProc(hWnd, msg, wParam, lParam);
		break;
	}

	asm volatile("decw\t(%bp)"); // make stack walking possible
	return lresult;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MessageBox(0, "Hello World!", "Message", 0);
	if(!hPrevInstance)
	{
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = MainWndProc;
		wc.hInstance = hInstance;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
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

