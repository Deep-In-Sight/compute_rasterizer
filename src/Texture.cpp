#include "Texture.h"
#include "GL/glew.h"
#include <iostream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

std::shared_ptr<Texture> Texture::create(int width, int height, GLuint colorType)
{
    GLuint handle;
    glCreateTextures(GL_TEXTURE_2D, 1, &handle);

    auto texture = std::make_shared<Texture>();
    texture->handle = handle;
    texture->colorType = colorType;
    texture->setSize(width, height);

    return texture;
}

void Texture::setSize(int width, int height)
{

    bool needsResize = this->width != width || this->height != height;

    if (needsResize)
    {

        glDeleteTextures(1, &this->handle);
        glCreateTextures(GL_TEXTURE_2D, 1, &this->handle);

        glTextureParameteri(this->handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(this->handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(this->handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(this->handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTextureStorage2D(this->handle, 1, this->colorType, width, height);

        this->width = width;
        this->height = height;
    }
}

void Texture::saveToFile(const std::string &filename)
{
    // check if filename ends with .png if not throw error
    if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".png")
    {
        std::cerr << "Unsupported file format. Only PNG is supported." << std::endl;
        return;
    }
    // Determine the format and type
    auto format = (colorType == GL_RGBA8) ? GL_RGBA : (colorType == GL_DEPTH_COMPONENT32F) ? GL_DEPTH_COMPONENT : 0;
    auto type = (colorType == GL_RGBA8) ? GL_UNSIGNED_BYTE : (colorType == GL_DEPTH_COMPONENT32F) ? GL_FLOAT : 0;
    int comp = (colorType == GL_RGBA8) ? 4 : (colorType == GL_DEPTH_COMPONENT32F) ? 1 : 0;

    if (!format || !type || !comp)
    {
        std::cerr << "Unsupported texture format or type." << std::endl;
        return;
    }

    // Allocate memory for the texture data
    unsigned char *data = new unsigned char[width * height * comp];

    // Bind the texture and read its data
    glBindTexture(GL_TEXTURE_2D, handle);
    glGetTexImage(GL_TEXTURE_2D, 0, format, type, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Write the data to a PNG file
    stbi_write_png(filename.c_str(), width, height, comp, data, width * comp);

    // Free the allocated memory
    delete[] data;
}
