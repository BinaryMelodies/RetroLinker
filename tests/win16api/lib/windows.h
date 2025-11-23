#ifndef __WINDOWS_H
#define __WINDOWS_H

#include <stdint.h>
#include <stddef.h>
#undef NULL
#define NULL 0

#define FAR __far

#define PASCAL __far __pascal
#define CALLBACK __far __pascal
#define WINAPI __far __pascal

typedef int BOOL;
typedef uint16_t HANDLE;
typedef uint16_t UINT;
typedef char FAR * LPSTR;
typedef char FAR const * LPCSTR;
typedef HANDLE HWND;
typedef HANDLE HINSTANCE;
typedef HANDLE HICON;
typedef HANDLE HCURSOR;
typedef HANDLE HBRUSH;
typedef int32_t LONG_PTR;
typedef LONG_PTR LPARAM;
typedef uint16_t UINT_PTR;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LRESULT;
typedef uint16_t WORD;
typedef WORD ATOM;
typedef uint32_t DWORD;
typedef HANDLE HMENU;
typedef void FAR * LPVOID;
typedef int32_t LONG;

typedef LRESULT FAR (__pascal * WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct
{
	UINT style;
	WNDPROC lpfnWndProc;
	int cbClsExtra;
	int cbWndExtra;
	HINSTANCE hInstance;
	HICON hIcon;
	HCURSOR hCursor;
	HBRUSH hbrBackground;
	LPCSTR lpszMenuName;
	LPCSTR lpszClassName;
} WNDCLASS;

typedef struct
{
	LONG x;
	LONG y;
} POINT;

typedef struct
{
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD time;
	POINT pt;
	DWORD lPrivate;
} MSG;

typedef MSG FAR * LPMSG;

#define WS_OVERLAPPED  0x00000000L
#define WS_MAXIMIZEBOX 0x00010000L
#define WS_MINIMIZEBOX 0x00020000L
#define WS_THICKFRAME  0x00040000L
#define WS_SYSMENU     0x00080000L
#define WS_CAPTION     0x00C00000L
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)

#define CW_USEDEFAULT 0x8000U

#define MessageBox $$IMPORT$USER$0001
extern int WINAPI MessageBox(HWND hwnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

#define CreateWindow $$IMPORT$USER$0029
extern HWND WINAPI CreateWindow(LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

#define ShowWindow $$IMPORT$USER$002A
extern BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow);

#define RegisterClass $$IMPORT$USER$0039
extern ATOM WINAPI RegisterClass(const WNDCLASS FAR * lpWndClass);

#define DefWindowProc $$IMPORT$USER$006B
extern LRESULT WINAPI DefWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define GetMessage $$IMPORT$USER$006C
extern BOOL WINAPI GetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);

#define TranslateMessage $$IMPORT$USER$0071
extern BOOL WINAPI TranslateMessage(const MSG FAR * lpMsg);

#define DispatchMessage $$IMPORT$USER$0072
extern LRESULT WINAPI DispatchMessage(const MSG FAR * lpMsg);

#define UpdateWindow $$IMPORT$USER$007C
extern BOOL WINAPI UpdateWindow(HWND hWnd);

#endif /* __WINDOWS_H */
