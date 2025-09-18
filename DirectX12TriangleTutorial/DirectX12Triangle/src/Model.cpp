#include "Model.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <limits>

void Model::createCube() {

	this->vertices = {
		// Front face
		{ {-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} },  // 0
		{ {-0.5f, +0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} },  // 1
		{ {+0.5f, +0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} },  // 2
		{ {+0.5f, -0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} },  // 3

		// Back face
		{ {+0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f} }, // 4
		{ {+0.5f, +0.5f, -0.5f}, {0.0f, 0.0f, -1.0f} }, // 5
		{ {-0.5f, +0.5f, -0.5f}, {0.0f, 0.0f, -1.0f} }, // 6
		{ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f} }, // 7

		// Left face
		{ {-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f} }, // 8
		{ {-0.5f, +0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f} }, // 9
		{ {-0.5f, +0.5f, +0.5f}, {-1.0f, 0.0f, 0.0f} }, // 10
		{ {-0.5f, -0.5f, +0.5f}, {-1.0f, 0.0f, 0.0f} }, // 11

		// Right face
		{ {+0.5f, -0.5f, +0.5f}, {1.0f, 0.0f, 0.0f} },  // 12
		{ {+0.5f, +0.5f, +0.5f}, {1.0f, 0.0f, 0.0f} },  // 13
		{ {+0.5f, +0.5f, -0.5f}, {1.0f, 0.0f, 0.0f} },  // 14
		{ {+0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f} },  // 15

		// Top face
		{ {-0.5f, +0.5f, +0.5f}, {0.0f, 1.0f, 0.0f} },  // 16
		{ {-0.5f, +0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },  // 17
		{ {+0.5f, +0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },  // 18
		{ {+0.5f, +0.5f, +0.5f}, {0.0f, 1.0f, 0.0f} },  // 19

		// Bottom face
		{ {-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f} }, // 20
		{ {-0.5f, -0.5f, +0.5f}, {0.0f, -1.0f, 0.0f} }, // 21
		{ {+0.5f, -0.5f, +0.5f}, {0.0f, -1.0f, 0.0f} }, // 22
		{ {+0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f} }  // 23
	};

	this->indices = {
		// Front face (0,1,2,3)
		0, 1, 2,  0, 2, 3,
		// Back face (4,5,6,7)
		4, 5, 6,  4, 6, 7,
		// Left face (8,9,10,11)
		8, 9, 10, 8, 10, 11,
		// Right face (12,13,14,15)
		12, 13, 14, 12, 14, 15,
		// Top face (16,17,18,19)
		16, 17, 18, 16, 18, 19,
		// Bottom face (20,21,22,23)
		20, 21, 22, 20, 22, 23
	};
}

void Model::LoadFromObj(const std::string& filename) {
	// Open the file
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Failed to open OBJ file: " << filename << std::endl;
		return;
	}

	// Temporary storage for parsing
	std::string line;
	std::vector<DirectX::XMFLOAT3> temp_vertices;
	std::vector<DirectX::XMFLOAT2> temp_uvs;
	std::vector<DirectX::XMFLOAT3> temp_normals;

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;

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
			// Supported formats: v, v/vt, v//vn, v/vt/vn; supports negative indices per OBJ spec.
			struct FaceVert { int v; int vt; int vn; };
			std::vector<FaceVert> faceVerts;
			std::string token;
			while (iss >> token) {
				if (token.empty()) continue;
				FaceVert fv{ -1, -1, -1 };
				// Split token by '/'
				size_t firstSlash = token.find('/');
				if (firstSlash == std::string::npos) {
					fv.v = std::stoi(token);
				}
				else {
					std::string vPart = token.substr(0, firstSlash);
					fv.v = vPart.empty() ? 0 : std::stoi(vPart);
					size_t secondSlash = token.find('/', firstSlash + 1);
					if (secondSlash == std::string::npos) {
						// v/vt
						std::string vtPart = token.substr(firstSlash + 1);
						if (!vtPart.empty()) fv.vt = std::stoi(vtPart);
					}
					else {
						// v//vn or v/vt/vn
						std::string vtPart = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
						std::string vnPart = token.substr(secondSlash + 1);
						if (!vtPart.empty()) fv.vt = std::stoi(vtPart);
						if (!vnPart.empty()) fv.vn = std::stoi(vnPart);
					}
				}
				faceVerts.push_back(fv);
			}

			if (faceVerts.size() < 3) continue; // not a valid face

			auto convertIndex = [](int idx, size_t count) -> unsigned int {
				if (idx > 0) return static_cast<unsigned int>(idx - 1); // 1-based positive
				if (idx < 0) return static_cast<unsigned int>(static_cast<int>(count) + idx); // negative relative
				return 0; // should not happen (OBJ indices are 1-based) but guard
			};

			for (size_t t = 1; t + 1 < faceVerts.size(); ++t) {
				FaceVert tri[3] = { faceVerts[0], faceVerts[t], faceVerts[t + 1] };
				for (int k = 0; k < 3; ++k) {
					unsigned int vIdx = convertIndex(tri[k].v, temp_vertices.size());
					unsigned int vtIdx = (tri[k].vt == -1) ? std::numeric_limits<unsigned int>::max() : convertIndex(tri[k].vt, temp_uvs.size());
					unsigned int vnIdx = (tri[k].vn == -1) ? std::numeric_limits<unsigned int>::max() : convertIndex(tri[k].vn, temp_normals.size());
					vertexIndices.push_back(vIdx);
					uvIndices.push_back(vtIdx);
					normalIndices.push_back(vnIdx);
				}
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
void Model::GetVertices(std::vector<Vertex>& outVertices) const {
	outVertices = vertices;
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
void Model::GetIndices(std::vector<unsigned int>& outIndices) const {
	outIndices = indices;
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