#include "Model.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <limits>
#include "File.h"

#ifdef max
#undef max
#endif
using namespace std;
#ifdef min
#undef min
#endif

void Model::UpdateTextures() {
	for (auto& mat : materials) {
		if (!mat.diffuseMap.empty()) {
			mat.textureImage.LoadFromImage(mat.diffuseMap);
			mat.initialized = true;
		}
	}
}

bool Model::LoadFromObj(const std::string& filename) {
	// Open the file
	std::ifstream file = OpenAssetFile(filename);
	if (!file.is_open()) {
		std::cerr << "Failed to open OBJ file: " << filename << std::endl;
		return false;
	}

	// Temporary storage for parsing
	std::string line;
	std::vector<DirectX::XMFLOAT3> temp_vertices;
	std::vector<DirectX::XMFLOAT2> temp_uvs;
	std::vector<DirectX::XMFLOAT3> temp_normals;
	std::vector<unsigned int> temp_materialIndices;
	std::vector<std::string> mtl_files;

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	unsigned int currentMaterialIndex = -1;

	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;
		if (prefix == "v") {
			DirectX::XMFLOAT3 vertex;
			iss >> vertex.x >> vertex.y >> vertex.z;
			temp_vertices.push_back(vertex);
		}
		else if (prefix == "vt") {
			DirectX::XMFLOAT2 uv;
			iss >> uv.x >> uv.y;
			// OBJ UV coordinates have Y=0 at bottom, but DirectX expects Y=0 at top
			// So we need to flip the V coordinate
			uv.y = 1.0f - uv.y;
			temp_uvs.push_back(uv);
		}
		else if (prefix == "vn") {
			DirectX::XMFLOAT3 normal;
			iss >> normal.x >> normal.y >> normal.z;
			temp_normals.push_back(normal);
		}
		else if (prefix == "vp") {
			// Parameter space vertices (not commonly used)
			DirectX::XMFLOAT3 paramVertex;
			iss >> paramVertex.x >> paramVertex.y >> paramVertex.z;
			// Ignored in this implementation
		}
		else if (prefix == "f") {
			// Parse entire face (which may be triangle, quad, or ngon) and triangulate (fan)
			struct FaceVert { int v; int vt; int vn; };
			std::vector<FaceVert> faceVerts;
			std::string token;
			while (iss >> token) {
				if (token.empty()) continue;
				FaceVert fv{ -1, -1, -1 };
				
				// Parse vertex/texture/normal indices
				size_t slash1 = token.find('/');
				if (slash1 == std::string::npos) {
					// Just vertex index
					fv.v = std::stoi(token) - 1;
				} else {
					// Has at least vertex/texture
					fv.v = std::stoi(token.substr(0, slash1)) - 1;
					size_t slash2 = token.find('/', slash1 + 1);
					if (slash2 == std::string::npos) {
						// vertex/texture format
						std::string uvStr = token.substr(slash1 + 1);
						if (!uvStr.empty()) {
							fv.vt = std::stoi(uvStr) - 1;
						}
					} else {
						// vertex/texture/normal or vertex//normal format
						std::string uvStr = token.substr(slash1 + 1, slash2 - slash1 - 1);
						if (!uvStr.empty()) {
							fv.vt = std::stoi(uvStr) - 1;
						}
						std::string normStr = token.substr(slash2 + 1);
						if (!normStr.empty()) {
							fv.vn = std::stoi(normStr) - 1;
						}
					}
				}
				faceVerts.push_back(fv);
			}

			if (faceVerts.size() < 3) continue; // not a valid face
			
			// Convert OBJ indices (1-based) to 0-based, handling negative indices
			auto convertIndex = [](int idx, size_t count) -> unsigned int {
				if (idx < 0) return static_cast<unsigned int>(count + idx);
				return static_cast<unsigned int>(idx);
			};

			// Fan triangulation
			for (size_t t = 1; t + 1 < faceVerts.size(); ++t) {
				// Triangle: 0, t, t+1
				vertexIndices.push_back(convertIndex(faceVerts[0].v, temp_vertices.size()));
				vertexIndices.push_back(convertIndex(faceVerts[t].v, temp_vertices.size()));
				vertexIndices.push_back(convertIndex(faceVerts[t + 1].v, temp_vertices.size()));
				
				// UV indices - use max value if not specified
				if (faceVerts[0].vt >= 0) {
					uvIndices.push_back(convertIndex(faceVerts[0].vt, temp_uvs.size()));
				} else {
					uvIndices.push_back(std::numeric_limits<unsigned int>::max());
				}
				
				if (faceVerts[t].vt >= 0) {
					uvIndices.push_back(convertIndex(faceVerts[t].vt, temp_uvs.size()));
				} else {
					uvIndices.push_back(std::numeric_limits<unsigned int>::max());
				}
				
				if (faceVerts[t + 1].vt >= 0) {
					uvIndices.push_back(convertIndex(faceVerts[t + 1].vt, temp_uvs.size()));
				} else {
					uvIndices.push_back(std::numeric_limits<unsigned int>::max());
				}
				
				// Normal indices, use max value if not specified
				if (faceVerts[0].vn >= 0) {
					normalIndices.push_back(convertIndex(faceVerts[0].vn, temp_normals.size()));
				} else {
					normalIndices.push_back(std::numeric_limits<unsigned int>::max());
				}
				
				if (faceVerts[t].vn >= 0) {
					normalIndices.push_back(convertIndex(faceVerts[t].vn, temp_normals.size()));
				} else {
					normalIndices.push_back(std::numeric_limits<unsigned int>::max());
				}
				
				if (faceVerts[t + 1].vn >= 0) {
					normalIndices.push_back(convertIndex(faceVerts[t + 1].vn, temp_normals.size()));
				} else {
					normalIndices.push_back(std::numeric_limits<unsigned int>::max());
				}
				
				// Store material index for this face
				temp_materialIndices.push_back(currentMaterialIndex);
				temp_materialIndices.push_back(currentMaterialIndex);
				temp_materialIndices.push_back(currentMaterialIndex);
			}
		}
		else if (prefix == "mtllib") {
			std::string mtlFile;
			iss >> mtlFile;
			if (!mtlFile.empty()) {
				LoadMTL(mtlFile);
				mtl_files.push_back(mtlFile);
			}
		}
		else if (prefix == "usemtl") {
			std::string materialName;
			iss >> materialName;
			// Need to ensure that the faces parsed after this are associated with the correct material
			if (materialMap.find(materialName) != materialMap.end()) {
				currentMaterialIndex = materialMap[materialName];
			}
			else {
				std::cerr << "Warning: Material " << materialName << " not found in material map." << std::endl;
			}

		}
	}

	// Now create the final vertex list
	std::map<std::tuple<unsigned int, unsigned int, unsigned int>, unsigned int> uniqueVertexMap; 
	bool missingNormals = false;
	for (size_t i = 0; i < vertexIndices.size(); i++) {
		unsigned int vIdx = vertexIndices[i];
		unsigned int uvIdx = uvIndices[i];
		unsigned int nIdx = normalIndices[i];
		unsigned int mIdx = temp_materialIndices[i];
		auto key = std::make_tuple(vIdx, uvIdx, nIdx);  
		if (uniqueVertexMap.find(key) == uniqueVertexMap.end()) {
			Vertex vert;
			vert.position = temp_vertices[vIdx];
			if (uvIdx == std::numeric_limits<unsigned int>::max() || uvIdx >= temp_uvs.size()) {
				vert.uv = { 0.0f, 0.0f };
			}
			else {
				vert.uv = temp_uvs[uvIdx];
			}
			if (nIdx == std::numeric_limits<unsigned int>::max() || nIdx >= temp_normals.size()) {
				vert.normal = { 0.0f, 0.0f, 0.0f };
				missingNormals = true;
			}
			else {
				vert.normal = temp_normals[nIdx];
			}
			vertices.push_back(vert);
			unsigned int newIndex = static_cast<unsigned int>(vertices.size() - 1);
			uniqueVertexMap[key] = newIndex;
			indices.push_back(newIndex);
			// Store material per face (every 3 indices = 1 face)
			if (i % 3 == 0) {  // Only store once per face
				materialIndices.push_back(mIdx);
			}
		}
		else {
			indices.push_back(uniqueVertexMap[key]);
		}
	}

	// If any normals were missing, compute flat normals.
	if (missingNormals) {
		ComputeNormals();
	}

	file.close();

	return true;
}

void Model::LoadMTL(const std::string& path) {
	std::ifstream file = OpenAssetFile(path);
	if (!file.is_open()) {
		std::cerr << "Failed to open MTL file: " << path << std::endl;
		return;
	}

	// Read and parse MTL file as needed
	std::string line;
	Material currentMaterial;
	currentMaterial.initialized = false;
	while (std::getline(file, line)) {
		// Simple parsing example; expand as needed
		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;
		if (prefix == "newmtl") {
			std::string materialName;
			iss >> materialName;
			// Handle new material
			if (currentMaterial.initialized) {
				// Store the previous material before starting a new one
				materials.push_back(currentMaterial);
				materialMap[materialNames.back()] = static_cast<unsigned int>(materials.size() - 1);
			}
			currentMaterial = Material();
			currentMaterial.initialized = true;
			materialNames.push_back(materialName);
		}
		else if (prefix == "Kd") {
			float r, g, b;
			iss >> r >> g >> b;
			// Handle diffuse color
			currentMaterial.diffuse = { r, g, b };
		}
		else if (prefix == "Ka") {
			float r, g, b;
			iss >> r >> g >> b;
			// Handle ambient color
			currentMaterial.ambient = { r, g, b };
		}
		else if (prefix == "Ks") {
			float r, g, b;
			iss >> r >> g >> b;
			// Handle specular color
			currentMaterial.specular = { r, g, b };
		}
		else if (prefix == "Ns") {
			float shininess;
			iss >> shininess;
			// Handle shininess
			currentMaterial.shininess = shininess;
		}
		else if (prefix == "map_Kd") {
			std::string texturePath;
			iss >> texturePath;
			// Handle texture map
			currentMaterial.diffuseMap = texturePath;
			currentMaterial.textureImage.LoadFromImage(texturePath);
		}
	}

	if(currentMaterial.initialized) {
		// Store the last material
		materials.push_back(currentMaterial);
		materialMap[materialNames.back()] = static_cast<unsigned int>(materials.size() - 1);
	}

	file.close();
}

void Model::MinMax(float& minX, float& minY, float& minZ, float& maxX, float& maxY, float& maxZ) {
	if (vertices.empty()) return;
	minX = maxX = vertices[0].position.x;
	minY = maxY = vertices[0].position.y;
	minZ = maxZ = vertices[0].position.z;
	for (const auto& v : vertices) {
		if (v.position.x < minX) minX = v.position.x;
		if (v.position.x > maxX) maxX = v.position.x;
		if (v.position.y < minY) minY = v.position.y;
		if (v.position.y > maxY) maxY = v.position.y;
		if (v.position.z < minZ) minZ = v.position.z;
		if (v.position.z > maxZ) maxZ = v.position.z;
	}
}
void Model::Clear() {
	vertices.clear();
	indices.clear();
}

void Model::GetPositions(std::vector<DirectX::XMFLOAT3>& outPositions) const {
	for (const auto& v : vertices) {
		outPositions.push_back(v.position);
	}
}
void Model::GetUVs(std::vector<DirectX::XMFLOAT2>& outUVs) const {
	for (const auto& v : vertices) {
		outUVs.push_back(v.uv);
	}
}
void Model::GetNormals(std::vector<DirectX::XMFLOAT3>& outNormals) const {
	for (const auto& v : vertices) {
		outNormals.push_back(v.normal);
	}
}

unsigned int Model::GetNumFaces() const {
	return static_cast<unsigned int>(indices.size() / 3);
}
unsigned int Model::GetNumVertices() const {
	return static_cast<unsigned int>(vertices.size());
}
unsigned int Model::GetNumIndices() const {
	return static_cast<unsigned int>(indices.size());
}

void Model::ComputeNormals() {
	for (size_t i = 0; i < indices.size(); i += 3) {
		unsigned int idx0 = indices[i];
		unsigned int idx1 = indices[i + 1];
		unsigned int idx2 = indices[i + 2];
		DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&vertices[idx0].position);
		DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&vertices[idx1].position);
		DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&vertices[idx2].position);
		DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(v1, v0);
		DirectX::XMVECTOR edge2 = DirectX::XMVectorSubtract(v2, v0);
		DirectX::XMVECTOR faceNormal = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(edge1, edge2));
		DirectX::XMStoreFloat3(&vertices[idx0].normal, DirectX::XMVector3Normalize(faceNormal));
		DirectX::XMStoreFloat3(&vertices[idx1].normal, DirectX::XMVector3Normalize(faceNormal));
		DirectX::XMStoreFloat3(&vertices[idx2].normal, DirectX::XMVector3Normalize(faceNormal));
	}
}

void Model::Scale(float scaleFactor) {
	for (auto& vertex : vertices) {
		vertex.position.x *= scaleFactor;
		vertex.position.y *= scaleFactor;
		vertex.position.z *= scaleFactor;
	}
}

void Model::ApplyTransformation() {
    DirectX::XMMATRIX transformMatrix = GetModelMatrix();
    
	float minX = 999999.0f;
	float minZ = 999999.0f;
	float maxX = -999999.0f;
	float maxZ = -999999.0f;
	float minY = 999999.0f;
	float maxY = -999999.0f;

    for (auto& vertex : vertices) {
        // Transform position
        DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&vertex.position);
        pos = DirectX::XMVector3Transform(pos, transformMatrix);
        DirectX::XMStoreFloat3(&vertex.position, pos);
        
        // Transform normal (use inverse transpose for correct normal transformation)
        DirectX::XMVECTOR det;
        DirectX::XMMATRIX normalMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, transformMatrix));
        DirectX::XMVECTOR norm = DirectX::XMLoadFloat3(&vertex.normal);
        norm = DirectX::XMVector3TransformNormal(norm, normalMatrix);
        norm = DirectX::XMVector3Normalize(norm);
        DirectX::XMStoreFloat3(&vertex.normal, norm);

		minX = std::min(minX, vertex.position.x);
		minZ = std::min(minZ, vertex.position.z);
		maxX = std::max(maxX, vertex.position.x);
		maxZ = std::max(maxZ, vertex.position.z);
		minY = std::min(minY, vertex.position.y);
		maxY = std::max(maxY, vertex.position.y);
    }

	b.SetBbox(minX, maxX, minZ, maxZ, minY, maxY);
    
    // Reset transformation after applying
    position = { 0.0f, 0.0f, 0.0f };
    rotation = { 0.0f, 0.0f, 0.0f };
    scale = { 1.0f, 1.0f, 1.0f };

}

// Add this method to Model class
void Model::SortByMaterial() {
	if (materials.empty()) return;

	// Create new sorted arrays
	std::vector<Vertex> sortedVertices;
	std::vector<unsigned int> sortedIndices;
	std::vector<unsigned int> sortedMaterialIndices;

	// Group indices by material
	for (unsigned int matIdx = 0; matIdx < materials.size(); ++matIdx) {
		for (size_t i = 0; i < materialIndices.size(); i += 3) {
			if (materialIndices[i] == matIdx) {
				// Add these three indices
				sortedIndices.push_back(indices[i]);
				sortedIndices.push_back(indices[i + 1]);
				sortedIndices.push_back(indices[i + 2]);

				sortedMaterialIndices.push_back(matIdx);
				sortedMaterialIndices.push_back(matIdx);
				sortedMaterialIndices.push_back(matIdx);
			}
		}
	}

	indices = sortedIndices;
	materialIndices = sortedMaterialIndices;
}

void Model::ComputeBoundingBox() {
	float minX, minY, minZ, maxX, maxY, maxZ;
	Model::MinMax(minX, minY, minZ, maxX, maxY, maxZ);

	b.SetBbox(minX, maxX, minZ, maxZ, minY, maxY);
}