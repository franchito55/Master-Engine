#include "Globals.h"
#include "ModuleRingBuffer.h"
#include "Application.h"
#include "ModuleResources.h"

#define BUFFER_SIZE_IN_256B_BLOCKS 1000

extern Application* app;

bool ModuleRingBuffer::init() {
	app->getModuleResources()->createUploadBuffer(buffer, 1000 * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	// Map once to get a pointer
	buffer->Map(0, nullptr, nullptr);
}

bool ModuleRingBuffer::allocate(const void* pData, const unsigned int size) {
	if (tail > head && size <= tail - head) {
		// allocate
	} else if (tail < head )
}