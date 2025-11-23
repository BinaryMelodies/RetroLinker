#ifndef __WINDOWS_H
#define __WINDOWS_H

#include <stdint.h>

typedef uint16_t HANDLE;
typedef uint16_t UINT;
typedef char __far * LPSTR;
typedef char __far const * LPCSTR;
typedef HANDLE HWND;
typedef HANDLE HINSTANCE;

#define PASCAL __far __pascal

#define MessageBox $$IMPORT$USER$0001
extern int PASCAL MessageBox(HWND hwnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

#endif /* __WINDOWS_H */
