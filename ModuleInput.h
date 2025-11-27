#pragma once

#include "Module.h"

namespace DirectX { class Keyboard; class Mouse; class GamePad;  }

class ModuleInput : public Module
{
public:

    ModuleInput(HWND hWnd);
    Keyboard* GetKeyboard() const { return keyboard.get(); }
    bool init() override;

private:
    std::unique_ptr<Keyboard> keyboard;
    std::unique_ptr<Mouse> mouse;
    std::unique_ptr<GamePad> gamePad;
};
