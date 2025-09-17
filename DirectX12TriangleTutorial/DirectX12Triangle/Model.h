#pragma once

// Model.h
#pragma once
#include <vector>
#include <DirectXMath.h>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
};

struct Model {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

struct alignas(256) MVPConstants {
    DirectX::XMMATRIX mvp;
};

Model createCube() {

    Model model;
    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;

    model.vertices = {
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

    model.indices = {
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

    return model;
}

// borrowed code but could be handy 
Model loadModelFromObj(const std::string& path) {
    std::ifstream file(path);  // Open the file

    if (!file.is_open()) {
        return {};
    }

    Model model;
    std::vector<DirectX::XMFLOAT3> temp_positions;
    std::vector<DirectX::XMFLOAT3> temp_normals;
    std::map<std::pair<unsigned int, unsigned int>, unsigned int> index_map;
    std::vector<unsigned int> temp_face_indices;

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);

        std::string prefix;
        ss >> prefix;

        if (prefix == "v") { //vertex position
            DirectX::XMFLOAT3 position;
            ss >> position.x >> position.y >> position.z;
            temp_positions.push_back(position);
        }
        else if (prefix == "vn") { //vertex normal
            DirectX::XMFLOAT3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (prefix == "f") {
            temp_face_indices.clear();

            std::string vertexindex_slashslash_normalindex;
            while (ss >> vertexindex_slashslash_normalindex) {
                size_t slash_pos = vertexindex_slashslash_normalindex.find("//");

                std::string vertex_index_str = vertexindex_slashslash_normalindex.substr(0, slash_pos);
                std::string normal_index_str = vertexindex_slashslash_normalindex.substr(slash_pos + 2);
                unsigned int vertex_index = std::stoi(vertex_index_str) - 1;
                unsigned int normal_index = std::stoi(normal_index_str) - 1;

                // Check if the vertex and normal index pair already exists in the map
                auto it = index_map.find({ vertex_index, normal_index });

                if (it != index_map.end()) {
                    temp_face_indices.push_back(it->second);
                }
                else {
                    // If it doesn't exist, create a new vertex and add it to the model
                    Vertex vertex;
                    vertex.position = temp_positions[vertex_index];
                    vertex.normal = temp_normals[normal_index];
                    unsigned int new_index = static_cast<unsigned int>(model.vertices.size());
                    model.vertices.push_back(vertex);
                    // Store the new index in the map
                    index_map[{ vertex_index, normal_index }] = new_index;
                    temp_face_indices.push_back(new_index);
                }
            }

            for (int i = 0; i < (int)temp_face_indices.size() - 2; i++) {
                model.indices.push_back(temp_face_indices[0]);
                model.indices.push_back(temp_face_indices[i + 1]);
                model.indices.push_back(temp_face_indices[i + 2]);
            }
        }
    }
    file.close();  // Close the file
    return model;
}