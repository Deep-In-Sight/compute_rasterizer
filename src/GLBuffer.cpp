#include "GL/glew.h"
#include <GLBuffer.h>
#include <unsuck.hpp>

GLBuffer createBuffer(int64_t size)
{
    GLuint handle;
    glCreateBuffers(1, &handle);
    glNamedBufferStorage(handle, size, 0, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);

    GLBuffer buffer;
    buffer.handle = handle;
    buffer.size = size;

    return buffer;
}

GLBuffer createSparseBuffer(int64_t size)
{
    GLuint handle;
    glCreateBuffers(1, &handle);
    glNamedBufferStorage(handle, size, 0, GL_DYNAMIC_STORAGE_BIT | GL_SPARSE_STORAGE_BIT_ARB);

    GLBuffer buffer;
    buffer.handle = handle;
    buffer.size = size;

    return buffer;
}

GLBuffer createUniformBuffer(int64_t size)
{
    GLuint handle;
    glCreateBuffers(1, &handle);
    glNamedBufferStorage(handle, size, 0, GL_DYNAMIC_STORAGE_BIT);

    GLBuffer buffer;
    buffer.handle = handle;
    buffer.size = size;

    return buffer;
}

std::shared_ptr<Buffer> readBuffer(GLBuffer glBuffer, uint32_t offset, uint32_t size)
{

    auto target = std::make_shared<Buffer>(size);

    glGetNamedBufferSubData(glBuffer.handle, offset, size, target->data);

    return target;
}