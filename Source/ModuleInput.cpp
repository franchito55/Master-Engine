#include "Globals.h"
#include "Application.h"
#include "ModuleInput.h"

#include "Keyboard.h"
#include "Mouse.h"
#include "GamePad.h"

extern Application* app;

ModuleInput::ModuleInput(HWND hWnd)
{
    keyboard = std::make_unique<Keyboard>();
    mouse = std::make_unique<Mouse>();
    gamePad = std::make_unique<GamePad>();

    mouse->SetWindow(hWnd);
}

bool ModuleInput::init() {

    return true;
}