<<<<<<< HEAD
#include <windows.h>
#include <stdio.h>
#include "resources.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HBITMAP hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL),
                                             MAKEINTRESOURCE(IDB_BIRD),
                                             IMAGE_BITMAP,
                                             0, 0,
                                             LR_CREATEDIBSECTION);
        if (hBitmap)
        {
            HDC memDC = CreateCompatibleDC(hdc);
            SelectObject(memDC, hBitmap);

            BITMAP bm;
            GetObject(hBitmap, sizeof(BITMAP), &bm);

            BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, memDC, 0, 0, SRCCOPY);

            DeleteDC(memDC);
            DeleteObject(hBitmap);
        }
        else
        {
            char msg[256];
            sprintf(msg, "LoadImage failed. Error: %lu", GetLastError());
            MessageBox(hwnd, msg, "Error", MB_OK | MB_ICONERROR);
        }

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int main()
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "BitmapWindowClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, "Bitmap Loader",
                               WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                               800, 600, NULL, NULL, wc.hInstance, NULL);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
=======
#include <windows.h>
#include <stdio.h>
#include "resources.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HBITMAP hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL),
                                             MAKEINTRESOURCE(IDB_BIRD),
                                             IMAGE_BITMAP,
                                             0, 0,
                                             LR_CREATEDIBSECTION);
        if (hBitmap)
        {
            HDC memDC = CreateCompatibleDC(hdc);
            SelectObject(memDC, hBitmap);

            BITMAP bm;
            GetObject(hBitmap, sizeof(BITMAP), &bm);

            BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, memDC, 0, 0, SRCCOPY);

            DeleteDC(memDC);
            DeleteObject(hBitmap);
        }
        else
        {
            char msg[256];
            sprintf(msg, "LoadImage failed. Error: %lu", GetLastError());
            MessageBox(hwnd, msg, "Error", MB_OK | MB_ICONERROR);
        }

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int main()
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "BitmapWindowClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, "Bitmap Loader",
                               WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                               800, 600, NULL, NULL, wc.hInstance, NULL);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
>>>>>>> 7169786 (Initial commit)
