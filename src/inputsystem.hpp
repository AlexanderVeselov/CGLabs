#ifndef INPUTSYSTEM_HPP
#define INPUTSYSTEM_HPP

class InputSystem
{
public:
    InputSystem();

    void KeyDown(unsigned int);
    void KeyUp(unsigned int);
    void SetMousePos(unsigned short x, unsigned short y) const;
    bool IsKeyDown(unsigned int) const;
    void GetMousePos(unsigned short *x, unsigned short *y) const;

private:
    bool m_Keys[256];

};

extern InputSystem* input;

#endif // INPUTSYSTEM_HPP
