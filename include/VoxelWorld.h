#ifndef VOXELWORLD_H
#define VOXELWORLD_H

#include <vector>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>

class ExtrusionManager; // Forward declaration

enum FaceDirection {
    RIGHT,
    LEFT,
    UP,
    DOWN,
    FORWARD,
    BACKWARD,
    NONE // For cases where no face is hit
};

struct Vertex {
    float x, y, z;
    float r, g, b;
    float nx, ny, nz;
    float u, v;
    bool selected;
    Vertex(float x, float y, float z, float r, float g, float b, float nx = 0.0f, float ny = 0.0f, float nz = 0.0f, float u = 0.0f, float v = 0.0f, bool selected = false)
        : x(x), y(y), z(z), r(r), g(g), b(b), nx(nx), ny(ny), nz(nz), u(u), v(v), selected(selected) {}
};

struct Voxel {
    int type;
    glm::vec3 color;
    bool selected;
    bool highlighted;
    std::string texture;
    glm::ivec3 position;

    Voxel(int type = 0, const glm::vec3& color = glm::vec3(1.0f), bool selected = false, bool highlighted = false, const std::string& texture = "", const glm::ivec3& position = glm::ivec3(0))
        : type(type), color(color), selected(selected), highlighted(highlighted), texture(texture), position(position) {}
};

struct VoxelIndexHasher {
    std::size_t operator()(const glm::ivec3& voxel) const {
        std::size_t xHash = std::hash<int>()(voxel.x);
        std::size_t yHash = std::hash<int>()(voxel.y);
        std::size_t zHash = std::hash<int>()(voxel.z);

        return ((xHash ^ (yHash << 1)) >> 1) ^ (zHash << 1);
    }
};

class VoxelWorld {
public:
    VoxelWorld(int size);
    void setVoxel(int x, int y, int z, int type, const std::string& color, const std::string& texture);
    void generateMeshData();
    std::vector<Vertex>& getVertices() { return vertices; }
    std::vector<unsigned int>& getIndices() { return indices; }
    GLuint loadTexture(const std::string& path);

    bool raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::ivec3& hitVoxel, glm::vec3& hitNormal, FaceDirection& hitFace);
    void updateVoxelColor(const glm::ivec3& voxel, const glm::vec3& color);
    void selectVoxel(const glm::ivec3& voxel);
    bool rayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, glm::vec3& hitPoint);
    //void clearSelections();
    void clearSelections(ExtrusionManager& extrusionManager); // Update this method signature

    std::vector<Vertex> selectedVertices;
    std::vector<unsigned int> selectedIndices;
    std::vector<Vertex> unselectedVertices;
    std::vector<unsigned int> unselectedIndices;

    void highlightVoxel(const glm::ivec3& voxel);
    void resetHighlight(const glm::ivec3& voxel);
    bool isVoxelSelected(const glm::ivec3& voxel) const;

    const std::vector<Vertex>& getUnselectedVertices() const { return unselectedVertices; }
    const std::vector<unsigned int>& getUnselectedIndices() const { return unselectedIndices; }
    const std::vector<Vertex>& getSelectedVertices() const { return selectedVertices; }
    const std::vector<unsigned int>& getSelectedIndices() const { return selectedIndices; }

    void extrudeVoxels(int direction, int layers);
    void removeSelectedVoxels();
    void removeVoxel(const glm::ivec3& position); // Add this declaration
    
private:
    int size;
    std::unordered_map<glm::ivec3, Voxel, VoxelIndexHasher> voxels;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<float> normals;

    glm::ivec3 getVoxelIndex(int x, int y, int z);
    void addFace(std::vector<Vertex>& vertexBuffer, std::vector<unsigned int>& indexBuffer, int x, int y, int z, const std::vector<Vertex>& faceVertices, const std::vector<unsigned int>& faceIndices, const glm::vec3& color);
    void calculateNormals(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
};

#endif
