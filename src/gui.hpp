#ifndef GUIMANAGER_HPP
#define GUIMANAGER_HPP

#include <vector>
#include <Windows.h>
#include <commctrl.h>

class GuiElement
{
public:
    GuiElement(const char* name) : m_Name(name) {}
    ~GuiElement() { DestroyWindow(m_Handle); }
    const char* GetName() const { return m_Name; }
    HWND GetHandle() const { return m_Handle; }
    
    virtual void Update() = 0;

protected:
    HWND m_Handle;
    const char* m_Name;

};

class Trackbar : public GuiElement
{
public:
    Trackbar(HWND parent, int x, int y, int width, int height, const char* caption, float minval = 0.0f, float maxval = 1.0f, float startval = 0.0f, float tickSize = 0.1f);
    virtual void Update();
    float GetValue() const { return m_CurrentValue; }

private:
    HWND m_ValueText;
    HWND m_MinValueText;
    HWND m_MaxValueText;
    float m_MinValue;
    float m_MaxValue;
    float m_TickSize;
    float m_CurrentValue;

};

class Checkbox : public GuiElement
{
public:
    Checkbox(HWND parent, int x, int y, int width, int height, const char* caption);
    virtual void Update();
    bool IsChecked() const { return m_Checked; }
    
private:
    bool m_Checked;

};

class GuiManager
{
public:
    void Init(HWND mainWindow);
    void Shutdown();
    Trackbar* AddTrackbar(int x, int y, int width, const char* caption, float minval = 0.0f, float maxval = 1.0f, float startval = 0.0f, float tickSize = 0.01f);
    Checkbox* AddCheckbox(int x, int y, int width, const char* caption);
    GuiElement* GetElementByHwnd(HWND handle) const;
    template <class T> const T* GetElementByName(const char* name) const;
    void UpdateElement(HWND handle);

private:
    HWND m_MainWindow;
    std::vector<GuiElement*> elements;


};

// Template methods should be implemented in header files
template <class T> const T* GuiManager::GetElementByName(const char* name) const
{
    for (size_t i = 0; i < elements.size(); ++i)
    {
        if (elements[i]->GetName() == name)
        {
            return (T*)elements[i];
        }
    }
    return nullptr;
}

//inline void DebugPrint(float f)
//{
//    char str[64];
//    sprintf(str, "%f", f);
//    TextOut(GetDC(0), 200, 200, str, strlen(str));
//}

extern GuiManager* guimanager;
extern HFONT g_DefaultFont;

#endif // GUIMANAGER_HPP