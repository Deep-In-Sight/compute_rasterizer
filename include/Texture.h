#pragma once
#include <memory>

struct Texture {
	unsigned int handle = -1;
	unsigned int colorType = -1;
	int width = 0;
	int height = 0;

	static std::shared_ptr<Texture> create(int width, int height, unsigned int colorType);
	void setSize(int width, int height);
};