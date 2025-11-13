#pragma once

#include "Globals.h"

#include <array>
#include <vector>
#include <chrono>

class Module;
class ModuleD3D12;
class ModuleEditor;
class ModuleBuffer;

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

    ModuleBuffer* getModuleBuffer() const { return moduleBuffer; }
    void setModuleBuffer(ModuleBuffer* _moduleBuffer) { moduleBuffer = _moduleBuffer; }

    unsigned int getWindowWidth() const { return windowWidth; }
    void setWindowWidth(const unsigned int _windowWidth) { windowWidth = _windowWidth; }
    unsigned int getWindowHeight() const { return windowHeight; }
    void setWindowHeight(const unsigned int _windowHeight) { windowHeight = _windowHeight; }

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

    ModuleD3D12* moduleD3D12 = nullptr;
    ModuleEditor* moduleEditor = nullptr;
    ModuleBuffer* moduleBuffer = nullptr;

    unsigned int windowWidth;
    unsigned int windowHeight;
};

extern Application* app;
