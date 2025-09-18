#pragma once

// Model.h
#pragma once
#include <vector>
#include <DirectXMath.h>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include "Primitives.h"
#include "Image.h"

struct Material {
	DirectX::XMFLOAT3 ambient = DirectX::XMFLOAT3(0.2f, 0.2f, 0.2f);   // Ka
	DirectX::XMFLOAT3 diffuse = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);   // Kd
	DirectX::XMFLOAT3 specular = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);  // Ks
	float shininess = 32.0f;             // Ns
	std::string diffuseMap = "";      // map_Kd
	bool initialized = false;
	Image textureImage;
};

class Model {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<unsigned int> materialIndices; // Material index per face
	std::map<std::string, unsigned int> materialMap; // Material name to index mapping
	std::vector<Material> materials; // List of materials
	std::vector<std::string> materialNames; // List of material names
    

public:

    void LoadFromObj(const std::string& path);
	void LoadMTL(const std::string& path);
	void MinMax(float& minX, float& minY, float& minZ, float& maxX, float& maxY, float& maxZ);
	void Clear();
	const std::vector<Vertex>& GetVertices() const { return vertices; }
	const std::vector<unsigned int>& GetIndices() const { return indices; }
	void GetPositions(std::vector<DirectX::XMFLOAT3>& outPositions) const;
	void GetUVs(std::vector<DirectX::XMFLOAT2>& outUVs) const;
	void GetNormals(std::vector<DirectX::XMFLOAT3>& outNormals) const;
	unsigned int GetNumFaces() const;
	unsigned int GetNumVertices() const;
	unsigned int GetNumIndices() const;
	void ComputeNormals();
	void Scale(float scaleFactor);
    void createCube();
	const std::vector<Material>& GetMaterials() const { return materials; }
	const std::vector<unsigned int>& GetFaceMaterialIndices() const { return materialIndices; }
};