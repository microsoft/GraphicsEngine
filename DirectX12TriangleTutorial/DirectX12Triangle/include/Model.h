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

struct BoundingBox {
    DirectX::XMFLOAT3 min;
    DirectX::XMFLOAT3 max;

    BoundingBox() : min(FLT_MAX, FLT_MAX, FLT_MAX), max(-FLT_MAX, -FLT_MAX, -FLT_MAX) {}

    bool Intersects(const DirectX::XMFLOAT3& point) const {
        return point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z;
    }

    bool Intersects(const BoundingBox& other) const {
        return !(max.x < other.min.x || min.x > other.max.x ||
            max.y < other.min.y || min.y > other.max.y ||
            max.z < other.min.z || min.z > other.max.z);
    }
};

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
    std::vector<unsigned int> materialIndices;
    std::map<std::string, unsigned int> materialMap;
    std::vector<Material> materials;
    std::vector<std::string> materialNames;

    // Transformation properties
    DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f }; // Euler angles in radians
    DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

    

public:

    bool LoadFromObj(const std::string& path);
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

    // Transformation methods
    void SetPosition(float x, float y, float z) { position = { x, y, z }; }
    void SetRotation(float x, float y, float z) { rotation = { x, y, z }; }
    void SetScale(float x, float y, float z) { scale = { x, y, z }; }
    
    DirectX::XMFLOAT3 GetPosition() const { return position; }
    DirectX::XMFLOAT3 GetRotation() const { return rotation; }
    DirectX::XMFLOAT3 GetScale() const { return scale; }
    
    // Get the model's transformation matrix
    DirectX::XMMATRIX GetModelMatrix() const {
        DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
        DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
        return S * R * T;
    }
	
	void ApplyTransformation();
	void SortByMaterial();

    // Bounding box methods
    const BoundingBox& GetBoundingBox() const { return boundingBox; }
    const BoundingBox& GetWorldBoundingBox() const { return worldBoundingBox; }
    void UpdateBoundingBox();
    void UpdateWorldBoundingBox();

    // Bounding boxes
    BoundingBox boundingBox;      // Local space
    BoundingBox worldBoundingBox; // World space
};