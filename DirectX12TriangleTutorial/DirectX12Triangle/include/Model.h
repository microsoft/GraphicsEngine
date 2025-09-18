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

class Model {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    

public:

    void LoadFromObj(const std::string& path);
	void MinMax(float& minX, float& minY, float& minZ, float& maxX, float& maxY, float& maxZ);
	void Clear();
	void GetVertices(std::vector<Vertex>& outVertices) const;
	void GetPositions(std::vector<DirectX::XMFLOAT3>& outPositions) const;
	void GetUVs(std::vector<DirectX::XMFLOAT2>& outUVs) const;
	void GetNormals(std::vector<DirectX::XMFLOAT3>& outNormals) const;
	void GetIndices(std::vector<unsigned int>& outIndices) const;
	unsigned int GetNumFaces() const;
	unsigned int GetNumVertices() const;
	unsigned int GetNumIndices() const;
	void ComputeNormals();
	void Scale(float scaleFactor);
    void createCube();
};