#include "Globals.h"
#include "Application.h"
#include "ModuleInput.h"
#include "ModuleD3D12.h"
#include "ModuleCameraEditor.h"
#include "ModuleAssignment2.h"
#include "ModuleImGui.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleNonShaderDescriptors.h"

Application::Application(int argc, wchar_t** argv, void* hWnd)
{
    RECT windowRect; 
    GetClientRect((HWND)hWnd, &windowRect);
    setWindowWidth(windowRect.right - windowRect.left);
    setWindowHeight(windowRect.bottom - windowRect.top);
    moduleCameraEditor = new ModuleCameraEditor((HWND)hWnd);
    moduleInput = new ModuleInput((HWND)hWnd);
    moduleD3D12 = new ModuleD3D12((HWND)hWnd);
    moduleShaderDescriptors = new ModuleShaderDescriptors((HWND)hWnd);
    moduleResources = new ModuleResources((HWND)hWnd);
    moduleNonShaderDescriptors = new ModuleNonShaderDescriptors((HWND)hWnd);
    moduleAssignment2 = new ModuleAssignment2((HWND)hWnd);
    moduleImGui = new ModuleImGui((HWND)hWnd);
    modules.push_back(moduleCameraEditor);
    modules.push_back(moduleInput);
    modules.push_back(moduleD3D12);
    modules.push_back(moduleShaderDescriptors);
    modules.push_back(moduleResources);
    modules.push_back(moduleNonShaderDescriptors);
    modules.push_back(moduleAssignment2);
    modules.push_back(moduleImGui); // ModuleImGui MUST go last
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

    for (auto it = modules.begin(); it != modules.end() && ret; ++it)
        ret = (*it)->postInit();

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
            modules.at(i)->lateUpdate();
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
