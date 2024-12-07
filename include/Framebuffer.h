
#pragma once

#include <vector>
#include <memory>

struct Texture;
struct Framebuffer {
	Framebuffer(int width, int height);

	std::vector<std::shared_ptr<Texture>> colorAttachments;
	std::shared_ptr<Texture> depth;
	unsigned int handle = -1;

	int width = 0;
	int height = 0;

	static std::shared_ptr<Framebuffer> create(int width, int height);
	void setSize(int width, int height);
};