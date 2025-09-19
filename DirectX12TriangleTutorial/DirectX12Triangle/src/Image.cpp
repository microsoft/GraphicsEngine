#include "Image.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>
#include "File.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void Image::SetPixel(int x, int y, const Pixel& color) {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		pixels[y * width + x] = color;
	}
}

void Image::saveBitmap(const std::string& filename) {
	FILE* file = nullptr;
	fopen_s(&file, filename.c_str(), "wb");
	if (!file) {
		std::cerr << "Failed to open file for writing: " << filename << std::endl;
		return;
	}
	BITMAPFILEHEADER fileHeader = {};
	BITMAPINFOHEADER infoHeader = {};
	int rowSize = (width * 4 + 3) & (~3); // Row size must be a multiple of 4 bytes
	int dataSize = rowSize * height;
	fileHeader.bfType = 0x4D42; // 'BM'
	fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dataSize;
	fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	infoHeader.biSize = sizeof(BITMAPINFOHEADER);
	infoHeader.biWidth = width;
	infoHeader.biHeight = height;
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = 32; // RGBA
	infoHeader.biCompression = BI_RGB;
	infoHeader.biSizeImage = dataSize;
	fwrite(&fileHeader, sizeof(fileHeader), 1, file);
	fwrite(&infoHeader, sizeof(infoHeader), 1, file);
	for (int y = height - 1; y >= 0; --y) { // BMP files are bottom to top
		for (int x = 0; x < width; ++x) {
			Pixel& p = pixels[y * width + x];
			fwrite(&p, sizeof(Pixel), 1, file);
		}
		// Padding for row alignment
		uint8_t padding[3] = {0, 0, 0};
		fwrite(padding, (4 - (width * 4) % 4) % 4, 1, file);
	}
	fclose(file);
}

void Image::LoadFromImage(const std::string& filename) {
	std::string fullPath = GetAssetPath(filename);
	this->raw_data = stbi_load(fullPath.c_str(), &width, &height, &channels, 4);
}

void Image::Clear(const Pixel& color) {
	std::fill(pixels.begin(), pixels.end(), color);
}

void Image::GetDimensions(int& outWidth, int& outHeight) const {
	outWidth = width;
	outHeight = height;
}

void Image::GetPixels(std::vector<Pixel>& outPixels) const {
	outPixels = pixels;
}

Pixel Image::GetPixel(int x, int y) const {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		return pixels[y * width + x];
	} else {
		return {0, 0, 0, 0}; // Return a default color if out of bounds
	}
}
