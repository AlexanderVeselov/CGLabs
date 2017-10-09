#include "render.hpp"
#include "gui.hpp"
#include "inputsystem.hpp"
#include <Windows.h>

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define WINDOW_CLASS "WindowClass1"
#define WINDOW_TITLE "CG Labs"

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    // sort through and find what code to run for the message given
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_HSCROLL:
    case WM_COMMAND:
        guimanager->UpdateElement((HWND)lParam);
        break;
    case WM_KEYDOWN:
        input->KeyDown((unsigned int)wParam);
        break;
    case WM_KEYUP:
        input->KeyUp((unsigned int)wParam);
        break;
    case WM_LBUTTONDOWN:
        input->MousePressed(MK_LBUTTON);
        break;
    case WM_LBUTTONUP:
        input->MouseReleased(MK_LBUTTON);
        break;
    case WM_RBUTTONDOWN:
        input->MousePressed(MK_RBUTTON);
        break;
    case WM_RBUTTONUP:
        input->MouseReleased(MK_RBUTTON);
        break;
    //case WM_MOUSEMOVE:
    //    input->MousePos(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    //    break;
    default:
        // Handle any messages the switch statement didn't
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;

}

HWND InitMainWindow(HINSTANCE hInstance, int nCmdShow)
{
    // the handle for the window, filled by a function
    HWND hWnd;
    // this struct holds information for the window class
    WNDCLASSEX wc;

    // clear out the window class for use
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = WINDOW_CLASS;

    // register the window class
    RegisterClassEx(&wc);

    // create the window and use the result as the handle
    hWnd = CreateWindowEx(NULL,
        WINDOW_CLASS,                       // name of the window class
        WINDOW_TITLE,                       // title of the window
        WS_OVERLAPPED | WS_SYSMENU,         // window style
        100,                                // x-position of the window
        100,                                // y-position of the window
        1552,                               // width of the window
        758,                                // height of the window
        NULL,                               // we have no parent window, NULL
        NULL,                               // we aren't using menus, NULL
        hInstance,                          // application handle
        NULL);                              // used with multiple windows, NULL

    ShowWindow(hWnd, nCmdShow);

    return hWnd;
}

void InitApp(HINSTANCE hInstance, int nCmdShow)
{
    HWND hwnd = InitMainWindow(hInstance, nCmdShow);
    
    guimanager->Init(hwnd);

    HWND hViewportPanel = CreateWindow("Static", NULL, WS_CHILD | WS_VISIBLE | SS_SUNKEN,
        256, 0, 1280 + 2, 720 + 2, hwnd, NULL, NULL, NULL);

    render->Init(hViewportPanel);

}

void ShutdownApp()
{
    render->Shutdown();
    guimanager->Shutdown();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    InitApp(hInstance, nCmdShow);

    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        // wait for the next message in the queue, store the result in 'msg'
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            // translate keystroke messages into the right format
            TranslateMessage(&msg);
            // send the message to the WindowProc function
            DispatchMessage(&msg);
        }
        // Render
        render->RenderFrame();
    }

    ShutdownApp();

    // return this part of the WM_QUIT message to Windows
    return msg.wParam;

}
