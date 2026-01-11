#pragma once

#include "Globals.h"

#include <array>
#include <vector>
#include <chrono>

class Module;
class ModuleD3D12;
class ModuleEditor;
class ModuleCameraEditor;
class ModuleInput;
class ModuleAssignment2;
class ModuleResources;
class ModuleShaderDescriptors;
class ModuleNonShaderDescriptors;
class ModuleImGui;

class Application
{
public:

	Application(int argc, wchar_t** argv, void* hWnd);
	~Application();

	bool         init();
	void         update();
	bool         cleanUp();

    
    float                       getFPS() const { return 1000.0f * float(MAX_FPS_TICKS) / tickSum; }
    float                       getAvgElapsedMs() const { return tickSum / float(MAX_FPS_TICKS); }
    uint64_t                    getElapsedMilis() const { return elapsedMilis; }

    bool                        isPaused() const { return paused; }
    bool                        setPaused(bool p) { paused = p; return paused; }

    std::vector<Module*> getModules() const { return modules; }

    ModuleD3D12* getModuleD3D12() const { return moduleD3D12; }
    void setModuleD3D12(ModuleD3D12* _moduleD3D12) { moduleD3D12 = _moduleD3D12; }

    ModuleEditor* getModuleEditor() const { return moduleEditor; }
    void setModuleEditor(ModuleEditor* _moduleEditor) { moduleEditor = _moduleEditor; }

    ModuleResources* getModuleResources() const { return moduleResources; }
    void setModuleResources(ModuleResources* _moduleResources) { moduleResources = _moduleResources; }

    ModuleInput* getModuleInput() const { return moduleInput; }
    void setModuleInput(ModuleInput* _moduleInput) { moduleInput = _moduleInput; }

    ModuleCameraEditor* getModuleCamera() const { return moduleCamera; }
    void setModuleCamera(ModuleCameraEditor* _moduleCamera) { moduleCamera = _moduleCamera; }

    ModuleAssignment2* getModuleAssignment2() const { return moduleAssignment2; }
    void setModuleAssignment2(ModuleAssignment2* _moduleAssignment2) { moduleAssignment2 = _moduleAssignment2; }

    ModuleShaderDescriptors* getModuleShaderDescriptors() const { return moduleShaderDescriptors; }
    void setModuleShaderDescriptors(ModuleShaderDescriptors* _moduleShaderDescriptors) { moduleShaderDescriptors = _moduleShaderDescriptors; }

    ModuleNonShaderDescriptors* getModuleNonShaderDescriptors() const { return moduleNonShaderDescriptors; }
    void setModuleNonShaderDescriptors(ModuleNonShaderDescriptors* _moduleNonShaderDescriptors) { moduleNonShaderDescriptors = _moduleNonShaderDescriptors; }

    ModuleImGui* getModuleImGui() const { return moduleImGui; }
    void setModuleImGui(ModuleImGui* _moduleImGui) { moduleImGui = _moduleImGui; }

    unsigned int getWindowWidth() const { return windowWidth; }
    void setWindowWidth(const unsigned int _windowWidth) { windowWidth = _windowWidth; }
    unsigned int getWindowHeight() const { return windowHeight; }
    void setWindowHeight(const unsigned int _windowHeight) { windowHeight = _windowHeight; }

    unsigned int getSceneRenderWindowWidth() const { return sceneRenderWindowWidth; }
    void setSceneRenderWindowWidth(const unsigned int _sceneRenderWindowWidth) { sceneRenderWindowWidth = _sceneRenderWindowWidth; }
    unsigned int getSceneRenderWindowHeight() const { return sceneRenderWindowHeight; }
    void setSceneRenderWindowHeight(const unsigned int _sceneRenderWindowHeight) { sceneRenderWindowHeight = _sceneRenderWindowHeight; }

private:
    enum { MAX_FPS_TICKS = 30 };
    typedef std::array<uint64_t, MAX_FPS_TICKS> TickList;

    std::vector<Module*> modules;

    uint64_t  lastMilis = 0;
    TickList  tickList;
    uint64_t  tickIndex;
    uint64_t  tickSum = 0;
    uint64_t  elapsedMilis = 0;
    bool      paused = false;

    ModuleD3D12* moduleD3D12;
    ModuleEditor* moduleEditor;
    ModuleResources* moduleResources;
    ModuleInput* moduleInput;
    ModuleCameraEditor* moduleCamera;
    ModuleAssignment2* moduleAssignment2;
    ModuleShaderDescriptors* moduleShaderDescriptors;
    ModuleNonShaderDescriptors* moduleNonShaderDescriptors;
    ModuleImGui* moduleImGui;

    unsigned int windowWidth;
    unsigned int windowHeight;

    unsigned int sceneRenderWindowWidth;
    unsigned int sceneRenderWindowHeight;
};

extern Application* app;
