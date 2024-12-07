
#include "Framebuffer.h"
#include "GL/glew.h"
#include <Texture.h>

Framebuffer::Framebuffer(int width, int height) : width(width), height(height)
{
}

std::shared_ptr<Framebuffer> Framebuffer::create(int width, int height)
{

    auto fbo = std::make_shared<Framebuffer>(width, height);

    glCreateFramebuffers(1, &fbo->handle);

    { // COLOR ATTACHMENT 0

        auto texture = Texture::create(fbo->width, fbo->height, GL_RGBA8);
        fbo->colorAttachments.push_back(texture);

        glNamedFramebufferTexture(fbo->handle, GL_COLOR_ATTACHMENT0, texture->handle, 0);
    }

    { // DEPTH ATTACHMENT

        auto texture = Texture::create(fbo->width, fbo->height, GL_DEPTH_COMPONENT32F);
        fbo->depth = texture;

        glNamedFramebufferTexture(fbo->handle, GL_DEPTH_ATTACHMENT, texture->handle, 0);
    }

    GLenum status = glCheckNamedFramebufferStatus(fbo->handle, GL_FRAMEBUFFER);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Framebuffer not complete");
    }

    return fbo;
}

void Framebuffer::setSize(int width, int height)
{

    bool needsResize = this->width != width || this->height != height;

    if (needsResize)
    {

        // COLOR
        for (int i = 0; i < this->colorAttachments.size(); i++)
        {
            auto &attachment = this->colorAttachments[i];
            attachment->setSize(width, height);
            glNamedFramebufferTexture(this->handle, GL_COLOR_ATTACHMENT0 + i, attachment->handle, 0);
        }

        { // DEPTH
            this->depth->setSize(width, height);
            glNamedFramebufferTexture(this->handle, GL_DEPTH_ATTACHMENT, this->depth->handle, 0);
        }

        this->width = width;
        this->height = height;
    }
}