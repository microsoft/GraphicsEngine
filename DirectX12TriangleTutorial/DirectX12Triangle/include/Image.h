#pragma once

#include <iostream>
#include <vector>
#include <windows.h>

struct Pixel {
	uint8_t r, g, b, a;
};

class Image {
public:
	Image()
		: width(0), height(0), channels(0), pixels(), raw_data(nullptr) { }
	void SetPixel(int x, int y, const Pixel& color);
	void saveBitmap(const std::string& filename);
	void Clear(const Pixel& color);
	void GetDimensions(int& outWidth, int& outHeight) const;
	void GetPixels(std::vector<Pixel>& outPixels) const;
	void LoadFromImage(const std::string& filename);
	Pixel GetPixel(int x, int y) const;
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }
    unsigned char* data() { return raw_data; }
    const unsigned char* data() const { return raw_data; } 

private:
	int width, height, channels;
	unsigned char* raw_data;
	std::vector<Pixel> pixels;
};