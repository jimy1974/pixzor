#include "VoxelWorld.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <limits>
#include <tuple>
#include <algorithm> // For std::min and std::max
#include "ExtrusionManager.h" // Include the header for the ExtrusionManager

// Include necessary libraries
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Define the necessary VBO and EBO variables
GLuint unselectedVBO, unselectedEBO, selectedVBO, selectedEBO;

VoxelWorld::VoxelWorld(int size) : size(size) {
    std::cout << "VoxelWorld created with size " << size << std::endl;
}

glm::ivec3 VoxelWorld::getVoxelIndex(int x, int y, int z) {
    return glm::ivec3(x, y, z);
}

void VoxelWorld::setVoxel(int x, int y, int z, int type, const std::string& color, const std::string& texture) {
    glm::vec3 colorVec(1.0f, 1.0f, 1.0f);
    if (color == "red") {
        colorVec = glm::vec3(1.0f, 0.0f, 0.0f);        
    } else if (color == "green") {
        colorVec = glm::vec3(0.0f, 1.0f, 0.0f);
    } else if (color == "blue") {
        colorVec = glm::vec3(0.0f, 0.0f, 1.0f);
    } else if (color == "gray") {
        colorVec = glm::vec3(0.5f, 0.5f, 0.5f); // Temporary voxel color
    }
    glm::ivec3 position(x, y, z);
    voxels[position] = Voxel(type, colorVec, false, false, texture, position);
    //std::cout << "Set voxel at: (" << x << ", " << y << ", " << z << ")\n";
}


GLuint VoxelWorld::loadTexture(const std::string& path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void VoxelWorld::calculateNormals(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    std::vector<float> normals(vertices.size() * 3, 0.0f);

    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int idx0 = indices[i];
        unsigned int idx1 = indices[i + 1];
        unsigned int idx2 = indices[i + 2];

        Vertex& v0 = vertices[idx0];
        Vertex& v1 = vertices[idx1];
        Vertex& v2 = vertices[idx2];

        float ux = v1.x - v0.x;
        float uy = v1.y - v0.y;
        float uz = v1.z - v0.z;
        float vx = v2.x - v0.x;
        float vy = v2.y - v0.y;
        float vz = v2.z - v0.z;

        float nx = uy * vz - uz * vy;
        float ny = uz * vx - ux * vz;
        float nz = ux * vy - uy * vx;

        normals[idx0 * 3] += nx;
        normals[idx0 * 3 + 1] += ny;
        normals[idx0 * 3 + 2] += nz;
        normals[idx1 * 3] += nx;
        normals[idx1 * 3 + 1] += ny;
        normals[idx1 * 3 + 2] += nz;
        normals[idx2 * 3] += nx;
        normals[idx2 * 3 + 1] += ny;
        normals[idx2 * 3 + 2] += nz;
    }

    for (size_t i = 0; i < vertices.size(); ++i) {
        float length = std::sqrt(normals[i * 3] * normals[i * 3] + normals[i * 3 + 1] * normals[i * 3 + 1] + normals[i * 3 + 2] * normals[i * 3 + 2]);
        if (length > 0) {
            normals[i * 3] /= length;
            normals[i * 3 + 1] /= length;
            normals[i * 3 + 2] /= length;
        }
        vertices[i].nx = normals[i * 3];
        vertices[i].ny = normals[i * 3 + 1];
        vertices[i].nz = normals[i * 3 + 2];
    }
}

void VoxelWorld::addFace(std::vector<Vertex>& vertexBuffer, std::vector<unsigned int>& indexBuffer, int x, int y, int z, const std::vector<Vertex>& faceVertices, const std::vector<unsigned int>& faceIndices, const glm::vec3& color) {
    unsigned int baseIndex = static_cast<unsigned int>(vertexBuffer.size());

    float scale = 1.0f / 5.0f;  // Assuming we want the texture to repeat every 5 voxels

    for (const auto& vertex : faceVertices) {
        float u = vertex.u * scale;
        float v = vertex.v * scale;

        // Adjust the UV coordinates based on the face being processed
        if (vertex.nx != 0) { // Left or Right face
            u += (z % 5) * scale;
            v += (y % 5) * scale;
        } else if (vertex.ny != 0) { // Top or Bottom face
            u += (x % 5) * scale;
            v += (z % 5) * scale;
        } else if (vertex.nz != 0) { // Front or Back face
            u += (x % 5) * scale;
            v += (y % 5) * scale;
        }

        vertexBuffer.emplace_back(vertex.x + x, vertex.y + y, vertex.z + z, color.r, color.g, color.b, vertex.nx, vertex.ny, vertex.nz, u, v);
    }

    for (const auto& index : faceIndices) {
        indexBuffer.push_back(baseIndex + index);
    }
}


void VoxelWorld::generateMeshData() {
    selectedVertices.clear();
    selectedIndices.clear();
    unselectedVertices.clear();
    unselectedIndices.clear();

    const float halfSize = 0.5f;

    const std::vector<Vertex> faceVertices[6] = {
        // Front face
        { Vertex(-halfSize, -halfSize, halfSize, 1, 0, 0, 0, 0, 1, 0, 0),
          Vertex(halfSize, -halfSize, halfSize, 0, 1, 0, 0, 0, 1, 1, 0),
          Vertex(halfSize, halfSize, halfSize, 0, 0, 1, 0, 0, 1, 1, 1),
          Vertex(-halfSize, halfSize, halfSize, 1, 1, 0, 0, 0, 1, 0, 1) },
        // Back face
        { Vertex(-halfSize, -halfSize, -halfSize, 1, 0, 0, 0, 0, -1, 0, 0),
          Vertex(halfSize, -halfSize, -halfSize, 0, 1, 0, 0, 0, -1, 1, 0),
          Vertex(halfSize, halfSize, -halfSize, 0, 0, 1, 0, 0, -1, 1, 1),
          Vertex(-halfSize, halfSize, -halfSize, 1, 1, 0, 0, 0, -1, 0, 1) },
        // Left face
        { Vertex(-halfSize, -halfSize, -halfSize, 1, 0, 0, -1, 0, 0, 0, 0),
          Vertex(-halfSize, -halfSize, halfSize, 0, 1, 0, -1, 0, 0, 1, 0),
          Vertex(-halfSize, halfSize, halfSize, 0, 0, 1, -1, 0, 0, 1, 1),
          Vertex(-halfSize, halfSize, -halfSize, 1, 1, 0, -1, 0, 0, 0, 1) },
        // Right face
        { Vertex(halfSize, -halfSize, -halfSize, 1, 0, 0, 1, 0, 0, 0, 0),
          Vertex(halfSize, -halfSize, halfSize, 0, 1, 0, 1, 0, 0, 1, 0),
          Vertex(halfSize, halfSize, halfSize, 0, 0, 1, 1, 0, 0, 1, 1),
          Vertex(halfSize, halfSize, -halfSize, 1, 1, 0, 1, 0, 0, 0, 1) },
        // Top face
        { Vertex(-halfSize, halfSize, -halfSize, 1, 0, 0, 0, 1, 0, 0, 0),
          Vertex(halfSize, halfSize, -halfSize, 0, 1, 0, 0, 1, 0, 1, 0),
          Vertex(halfSize, halfSize, halfSize, 0, 0, 1, 0, 1, 0, 1, 1),
          Vertex(-halfSize, halfSize, halfSize, 1, 1, 0, 0, 1, 0, 0, 1) },
        // Bottom face
        { Vertex(-halfSize, -halfSize, -halfSize, 1, 0, 0, 0, -1, 0, 0, 0),
          Vertex(halfSize, -halfSize, -halfSize, 0, 1, 0, 0, -1, 0, 1, 0),
          Vertex(halfSize, -halfSize, halfSize, 0, 0, 1, 0, -1, 0, 1, 1),
          Vertex(-halfSize, -halfSize, halfSize, 1, 1, 0, 0, -1, 0, 0, 1) }
    };

    const std::vector<unsigned int> faceIndices = { 0, 1, 2, 2, 3, 0 };

    for (const auto& voxelPair : voxels) {
        const glm::ivec3& pos = voxelPair.second.position;
        glm::vec3 color = voxelPair.second.color;

        if (voxelPair.second.selected) {
            color = glm::vec3(1.0f, 0.0f, 0.0f); // Red for selected
        } else if (voxelPair.second.highlighted) {
            color = glm::vec3(1.0f, 0.7f, 0.0f); // Orange for highlighted
        }

        auto& vertexBuffer = voxelPair.second.selected || voxelPair.second.highlighted ? selectedVertices : unselectedVertices;
        auto& indexBuffer = voxelPair.second.selected || voxelPair.second.highlighted ? selectedIndices : unselectedIndices;

        if (!voxels.count(getVoxelIndex(pos.x, pos.y, pos.z + 1))) {
            addFace(vertexBuffer, indexBuffer, pos.x, pos.y, pos.z, faceVertices[0], faceIndices, color);
        }
        if (!voxels.count(getVoxelIndex(pos.x, pos.y, pos.z - 1))) {
            addFace(vertexBuffer, indexBuffer, pos.x, pos.y, pos.z, faceVertices[1], faceIndices, color);
        }
        if (!voxels.count(getVoxelIndex(pos.x - 1, pos.y, pos.z))) {
            addFace(vertexBuffer, indexBuffer, pos.x, pos.y, pos.z, faceVertices[2], faceIndices, color);
        }
        if (!voxels.count(getVoxelIndex(pos.x + 1, pos.y, pos.z))) {
            addFace(vertexBuffer, indexBuffer, pos.x, pos.y, pos.z, faceVertices[3], faceIndices, color);
        }
        if (!voxels.count(getVoxelIndex(pos.x, pos.y + 1, pos.z))) {
            addFace(vertexBuffer, indexBuffer, pos.x, pos.y, pos.z, faceVertices[4], faceIndices, color);
        }
        if (!voxels.count(getVoxelIndex(pos.x, pos.y - 1, pos.z))) {
            addFace(vertexBuffer, indexBuffer, pos.x, pos.y, pos.z, faceVertices[5], faceIndices, color);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, unselectedVBO);
    glBufferData(GL_ARRAY_BUFFER, unselectedVertices.size() * sizeof(Vertex), unselectedVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unselectedEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, unselectedIndices.size() * sizeof(unsigned int), unselectedIndices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, selectedVBO);
    glBufferData(GL_ARRAY_BUFFER, selectedVertices.size() * sizeof(Vertex), selectedVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, selectedEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, selectedIndices.size() * sizeof(unsigned int), selectedIndices.data(), GL_STATIC_DRAW);
}

bool VoxelWorld::rayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, glm::vec3& hitPoint) {
    const float EPSILON = 0.0000001f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(rayDirection, edge2);
    float a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON)
        return false; // Ray is parallel to the triangle.

    float f = 1.0f / a;
    glm::vec3 s = rayOrigin - v0;
    float u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f)
        return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(rayDirection, q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    float t = f * glm::dot(edge2, q);
    if (t > EPSILON) {
        hitPoint = rayOrigin + rayDirection * t;
        return true;
    }

    return false;
}

//bool VoxelWorld::raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::ivec3& hitVoxel) {
bool VoxelWorld::raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::ivec3& hitVoxel, glm::vec3& hitNormal, FaceDirection& hitFace) {
    float closestDistance = std::numeric_limits<float>::max();
    bool hit = false;

    for (const auto& voxelPair : voxels) {
        const Voxel& voxel = voxelPair.second;
        glm::vec3 voxelMin = glm::vec3(voxel.position) - glm::vec3(0.5f);
        glm::vec3 voxelMax = glm::vec3(voxel.position) + glm::vec3(0.5f);

        float tMin = (voxelMin.x - rayOrigin.x) / rayDirection.x;
        float tMax = (voxelMax.x - rayOrigin.x) / rayDirection.x;

        if (tMin > tMax) std::swap(tMin, tMax);

        float tyMin = (voxelMin.y - rayOrigin.y) / rayDirection.y;
        float tyMax = (voxelMax.y - rayOrigin.y) / rayDirection.y;

        if (tyMin > tyMax) std::swap(tyMin, tyMax);

        if ((tMin > tyMax) || (tyMin > tMax))
            continue;

        if (tyMin > tMin)
            tMin = tyMin;

        if (tyMax < tMax)
            tMax = tyMax;

        float tzMin = (voxelMin.z - rayOrigin.z) / rayDirection.z;
        float tzMax = (voxelMax.z - rayOrigin.z) / rayDirection.z;

        if (tzMin > tzMax) std::swap(tzMin, tzMax);

        if ((tMin > tzMax) || (tzMin > tMax))
            continue;

        if (tzMin > tMin)
            tMin = tzMin;

        if (tzMax < tMax)
            tMax = tzMax;

        if (tMin < 0) tMin = tMax;

        if (tMin < closestDistance) {
            closestDistance = tMin;
            hitVoxel = voxel.position;
            hit = true;

            // Determine which face was hit based on the minimum t value
            glm::vec3 hitPoint = rayOrigin + tMin * rayDirection;
            glm::vec3 voxelCenter = glm::vec3(voxel.position);
            glm::vec3 localHitPoint = hitPoint - voxelCenter;

            if (abs(localHitPoint.x) > abs(localHitPoint.y) && abs(localHitPoint.x) > abs(localHitPoint.z)) {
                hitNormal = glm::vec3(glm::sign(localHitPoint.x), 0.0f, 0.0f);
                hitFace = localHitPoint.x > 0 ? RIGHT : LEFT;
            } else if (abs(localHitPoint.y) > abs(localHitPoint.x) && abs(localHitPoint.y) > abs(localHitPoint.z)) {
                hitNormal = glm::vec3(0.0f, glm::sign(localHitPoint.y), 0.0f);
                hitFace = localHitPoint.y > 0 ? UP : DOWN;
            } else {
                hitNormal = glm::vec3(0.0f, 0.0f, glm::sign(localHitPoint.z));
                hitFace = localHitPoint.z > 0 ? FORWARD : BACKWARD;
            }
        }
    }
    if (!hit) {
        hitFace = NONE; // No face was hit
    }
    return hit;
}



 

void VoxelWorld::updateVoxelColor(const glm::ivec3& voxel, const glm::vec3& color) {
    if (voxels.find(voxel) != voxels.end()) {
        voxels[voxel].color = color;
        voxels[voxel].selected = true;
        generateMeshData();
    }
}

void VoxelWorld::selectVoxel(const glm::ivec3& voxel) {
    if (voxels.find(voxel) != voxels.end()) {
        voxels[voxel].selected = true;
        //std::cout << "Voxel selected: " << voxel.x << ", " << voxel.y << ", " << voxel.z << std::endl;
        generateMeshData();  // Regenerate mesh data to update the selection
    }
}

void VoxelWorld::highlightVoxel(const glm::ivec3& voxel) {
    if (voxels.find(voxel) != voxels.end() && !voxels[voxel].selected) {
        //std::cout << "highlightVoxel  selected: " << voxels[voxel].selected << " at " << voxel.x << ", " << voxel.y << ", " << voxel.z << std::endl;
        voxels[voxel].highlighted = true;
        generateMeshData(); // Regenerate mesh data to update the highlight
    }
}

void VoxelWorld::resetHighlight(const glm::ivec3& voxel) {
    if (voxels.find(voxel) != voxels.end() && !voxels[voxel].selected) {
        //std::cout << "resetHighlight  selected: " << voxels[voxel].selected << " at " << voxel.x << ", " << voxel.y << ", " << voxel.z << std::endl;
        voxels[voxel].highlighted = false;
        generateMeshData(); // Regenerate mesh data to update the highlight
    }
}



void VoxelWorld::clearSelections(ExtrusionManager& extrusionManager) {
    for (auto& pair : voxels) {
        pair.second.selected = false;
        pair.second.highlighted = false;
    }
    extrusionManager.clearSelectedVoxels(); // Clear extrusion manager's selected voxels
    generateMeshData();
}




bool VoxelWorld::isVoxelSelected(const glm::ivec3& voxel) const {
    auto it = voxels.find(voxel);
    return it != voxels.end() && it->second.selected;
}



void VoxelWorld::extrudeVoxels(int direction, int layers) {
    std::vector<Voxel> newVoxels;
    for (const auto& voxelPair : voxels) {
        const Voxel& voxel = voxelPair.second;
        if (voxel.selected) {
            for (int i = 1; i <= layers; ++i) {
                glm::ivec3 newPos = voxel.position;
                switch (direction) {
                    case 0: newPos.x += i; break; // Right
                    case 1: newPos.x -= i; break; // Left
                    case 2: newPos.y += i; break; // Up
                    case 3: newPos.y -= i; break; // Down
                    case 4: newPos.z += i; break; // Forward
                    case 5: newPos.z -= i; break; // Backward
                }
                if (voxels.find(newPos) == voxels.end()) {
                    newVoxels.emplace_back(voxel.type, voxel.color, false, false, voxel.texture, newPos);
                }
            }
        }
    }
    for (const auto& voxel : newVoxels) {
        voxels[voxel.position] = voxel;
    }
    generateMeshData();
}

void VoxelWorld::removeSelectedVoxels() {
    for (auto it = voxels.begin(); it != voxels.end(); ) {
        if (it->second.selected) {
            it = voxels.erase(it);  // Remove the voxel if it's selected
        } else {
            ++it;
        }
    }
    generateMeshData();  // Regenerate mesh data to update the scene
}

void VoxelWorld::removeVoxel(const glm::ivec3& position) {
    voxels.erase(position); // Correctly remove the voxel from the map
    generateMeshData(); // Regenerate mesh data to update the scene
}


