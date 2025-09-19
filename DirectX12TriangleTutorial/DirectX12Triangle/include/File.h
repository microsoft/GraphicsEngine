#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <windows.h>

inline std::string GetExecutablePath() {
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string fullPath(buffer);
	return fullPath.substr(0, fullPath.find_last_of("\\/"));
}

inline std::vector<std::string> EnumerateAssetFiles() {
	std::vector<std::string> assetFiles;
	std::filesystem::path exePath = GetExecutablePath();
	std::filesystem::path assetsDir = exePath.parent_path().parent_path() / "DirectX12Triangle" / "assets";
	std::vector<std::string> assetDirectories;
	
	assetFiles.clear();
	// Recursively find all subdirectories
	for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsDir)) {
		std::cout << entry.path().string() << std::endl;
		if (entry.is_regular_file()) {
			assetFiles.push_back(entry.path().string());
		}
	}

	return assetFiles;
}

inline std::string GetAssetPath(const std::string& assetName) {
	std::vector<std::string> assetFiles = EnumerateAssetFiles();

	// Match the asset name to the end of the path of the files found
	// EnumerateAssetFiles returns the full paths
	// We want to find the one that ends with assetName
	for (const auto& file : assetFiles) {

		// Extract just the filename (handles both / and \ as separators)
		size_t pos = file.find_last_of("/\\");
		std::string filename = (pos != std::string::npos) ? file.substr(pos + 1) : file;

		/*if (file.size() >= assetName.size() &&
			file.compare(file.size() - assetName.size(), assetName.size(), assetName) == 0) {
			return file;
		}*/

		if (filename.compare(assetName) == 0) {
			return file;
		}
	}

	return "";
}

inline std::ifstream OpenAssetFile(const std::string& assetName) { //TODO: deal with relative paths i.e. Textures/Tree/Tree1.obj
	std::string path = GetAssetPath(assetName);
	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Failed to open asset file: " << path << std::endl;
	}
	return file;
}