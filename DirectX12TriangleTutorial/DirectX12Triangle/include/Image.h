#pragma once

#include <iostream>
#include <vector>
#include <windows.h>

struct Pixel {
	uint8_t r, g, b, a;
};

class Image {
public:
	Image(int width, int height)
		: width(width), height(height), pixels(width* height) { }
	void SetPixel(int x, int y, const Pixel& color);
	void saveBitmap(const std::string& filename);
	void Clear(const Pixel& color);
	void GetDimensions(int& outWidth, int& outHeight) const;
	void GetPixels(std::vector<Pixel>& outPixels) const;
	Pixel GetPixel(int x, int y) const;

private:
	int width, height;
	std::vector<Pixel> pixels;
};