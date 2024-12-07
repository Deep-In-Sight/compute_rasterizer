
#pragma once

#include <memory>

struct GLBuffer
{
    unsigned int handle = -1;
    int64_t size = 0;
};

struct Buffer;
GLBuffer createBuffer(int64_t size);
GLBuffer createSparseBuffer(int64_t size);
GLBuffer createUniformBuffer(int64_t size);
std::shared_ptr<Buffer> readBuffer(GLBuffer glBuffer, uint32_t offset, uint32_t size);