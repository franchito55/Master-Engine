#include "Globals.h"
#include "Application.h"
#include "ModuleInput.h"
#include "ModuleD3D12.h"
#include "ModuleEditor.h"
#include "ModuleBuffer.h"
#include "ModuleExercise3.h"
#include "ModuleCameraEditor.h"

Application::Application(int argc, wchar_t** argv, void* hWnd)
{
    RECT windowRect; 
    GetClientRect((HWND)hWnd, &windowRect);
    setWindowWidth(windowRect.right - windowRect.left);
    setWindowHeight(windowRect.bottom - windowRect.top);
    modules.push_back(new ModuleCameraEditor((HWND)hWnd));
    modules.push_back(new ModuleInput((HWND)hWnd));
    modules.push_back(new ModuleD3D12((HWND)hWnd));
    modules.push_back(new ModuleBuffer((HWND)hWnd));
    //modules.push_back(new ModuleEditor((HWND)hWnd));
    modules.push_back(new ModuleExercise3((HWND)hWnd));
}

Application::~Application()
{
    cleanUp();

	for(auto it = modules.rbegin(); it != modules.rend(); ++it)
    {
        delete *it;
    }
}
 
bool Application::init()
{
	bool ret = true;

	for(auto it = modules.begin(); it != modules.end() && ret; ++it)
		ret = (*it)->init();

    //((ModuleEditor*)modules.at(2))->postInit((ModuleD3D12*)modules.at(1));

    lastMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	return ret;
}

void Application::update()
{
    using namespace std::chrono_literals;

    // Update milis
    uint64_t currentMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    elapsedMilis = currentMilis - lastMilis;
    lastMilis = currentMilis;
    tickSum -= tickList[tickIndex];
    tickSum += elapsedMilis;
    tickList[tickIndex] = elapsedMilis;
    tickIndex = (tickIndex + 1) % MAX_FPS_TICKS;

    if (!app->paused)
    {
        for (int i = 0; i < modules.size(); i++) {
            modules.at(i)->update();
        }

        for (int i = 0; i < modules.size(); i++) {
            modules.at(i)->preRender();
        }

        for (int i = 0; i < modules.size(); i++) {
            modules.at(i)->render();
        }

        for (int i = 0; i < modules.size(); i++) {
            modules.at(i)->postRender();
        }
    }
}

bool Application::cleanUp()
{
	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
