#include "Globals.h"
#include "Module.h"

class ModuleRingBuffer : public Module {
public:
	ModuleRingBuffer(HWND _hWnd) : hWnd(_hWnd) {}
	~ModuleRingBuffer() {}

	bool init() override;
	bool allocate();

private:
	HWND hWnd;
	ComPtr<ID3D12Resource> buffer;
	unsigned int head;
	unsigned int tail;
	unsigned int totalSize;
};