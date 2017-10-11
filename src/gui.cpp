#include "gui.hpp"

static GuiManager g_GuiManager;
GuiManager* guimanager = &g_GuiManager;

HFONT g_DefaultFont = CreateFont(
    15, 6, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET,
    OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
    DEFAULT_PITCH | FF_DONTCARE, "Tahoma");

HWND CreateCaption(HWND parent, const char* text, int x, int y, int customWidth = 0)
{
    SIZE size;
    HDC hdc = GetDC(0);
    GetTextExtentPoint32(hdc, text, strlen(text), &size);

    HWND caption = CreateWindow("Static", text, WS_CHILD | WS_VISIBLE, x, y, (customWidth > 0) ? customWidth : size.cx, size.cy, parent, NULL, NULL, NULL);
    SendMessage(caption, WM_SETFONT, (WPARAM)g_DefaultFont, TRUE);

    return caption;

}

Trackbar::Trackbar(HWND parent, int x, int y, int width, int height, const char* caption, float minval, float maxval, float startval, float tickSize)
    : GuiElement(caption), m_MinValue(minval), m_MaxValue(maxval), m_TickSize(tickSize), m_CurrentValue(startval)
{
    m_Handle = CreateWindow(TRACKBAR_CLASS,                 // Class name
                    "Trackbar Control",                    // Window name
                    WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,  // Style
                    x,                                      // x
                    y,                                      // x
                    width,                                  // Width
                    height,                                 // Height
                    parent,                                 // Parent
                    nullptr,                                // Menu
                    nullptr,                                // hInstance
                    nullptr                                 // lpParam
                );

    SendMessage(m_Handle, TBM_SETRANGE, TRUE, MAKELONG(0, (int)((m_MaxValue - m_MinValue) / m_TickSize)));
    SendMessage(m_Handle, TBM_SETPAGESIZE, 0, 20); // ???
    SendMessage(m_Handle, TBM_SETTICFREQ, 1, 0);
    SendMessage(m_Handle, TBM_SETPOS, FALSE, (int)((startval - m_MinValue) / m_TickSize));
    
    char buf[64];
    sprintf(buf, "%.2f", minval);
    m_MinValueText = CreateCaption(parent, buf, 0, 0);
    sprintf(buf, "%.2f", maxval);
    m_MaxValueText = CreateCaption(parent, buf, 0, 0);

    SendMessage(m_Handle, TBM_SETBUDDY, TRUE, (LPARAM)m_MinValueText);
    SendMessage(m_Handle, TBM_SETBUDDY, FALSE, (LPARAM)m_MaxValueText);

    sprintf(buf, "%s: %.2f", m_Name, m_CurrentValue);
    m_ValueText = CreateCaption(parent, buf, x + 10, y - 20, width - 10);
    
}

void Trackbar::Update()
{
    LRESULT pos = SendMessage(m_Handle, TBM_GETPOS, 0, 0);
    m_CurrentValue = m_MinValue + pos * m_TickSize;

    char buf[64];
    sprintf(buf, "%s: %.2f", m_Name, m_CurrentValue);
    SetWindowText(m_ValueText, buf);

}

Checkbox::Checkbox(HWND parent, int x, int y, int width, int height, const char* caption) : GuiElement(caption), m_Checked(false)
{
    m_Handle = CreateWindow("button", caption, WS_VISIBLE | WS_CHILD | BS_CHECKBOX, x, y, width, height, parent, nullptr, nullptr, nullptr);
    SendMessage(m_Handle, WM_SETFONT, (WPARAM)g_DefaultFont, TRUE);

}

void Checkbox::Update()
{
    SendMessage(m_Handle, BM_SETCHECK, m_Checked ? BST_UNCHECKED : BST_CHECKED, TRUE);
    m_Checked = !m_Checked;
    
}

void GuiManager::Init(HWND mainWindow)
{
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    m_MainWindow = mainWindow;
}

void GuiManager::Shutdown()
{
    for (int i = 0; i < elements.size(); ++i)
    {
        if (elements[i])
        {
            delete elements[i];
        }
    }
    elements.clear();

}

Trackbar* GuiManager::AddTrackbar(int x, int y, int width, const char* caption, float minval, float maxval, float startval, float tickSize)
{
    Trackbar* trackbar = new Trackbar(m_MainWindow, x, y, width, 32, caption, minval, maxval, startval, tickSize);
    elements.push_back(trackbar);
    return trackbar;

}

Checkbox* GuiManager::AddCheckbox(int x, int y, int width, const char* caption)
{
    Checkbox* checkbox = new Checkbox(m_MainWindow, x, y, width, 32, caption);
    elements.push_back(checkbox);
    return checkbox;

}

GuiElement* GuiManager::GetElementByHwnd(HWND handle) const
{
    for (int i = 0; i < elements.size(); ++i)
    {
        if (elements[i]->GetHandle() == handle)
        {
            return elements[i];
        }
    }
    return nullptr;

}

void GuiManager::UpdateElement(HWND handle)
{
    GuiElement* element = GetElementByHwnd(handle);
    if (element)
    {
        element->Update();
    }

}
