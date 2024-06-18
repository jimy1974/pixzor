#ifndef EXTRUSION_MANAGER_H
#define EXTRUSION_MANAGER_H

#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>
#include "VoxelWorld.h"
#include "glm_hash.h"

class ExtrusionManager {
public:
    void startExtrusion(const glm::ivec3& startVoxel, const glm::vec3& normal, FaceDirection face, const glm::dvec2& initialMousePos);
    void updateExtrusion(const glm::dvec2& currentMousePos, VoxelWorld& voxelWorld);
    void endExtrusion(VoxelWorld& voxelWorld);
    bool isExtruding() const;
    glm::ivec3 getExtrusionStart() const;
    void setSelectedVoxels(const std::unordered_set<glm::ivec3>& selectedVoxels);
    void clearSelectedVoxels();

private:
    glm::ivec3 extrusionStart;
    glm::vec3 extrusionNormal;
    FaceDirection hitFace;
    glm::dvec2 initialMousePos;
    bool extruding = false;
    int currentLayers = 0;
    std::unordered_set<glm::ivec3> newVoxels;
    std::unordered_set<glm::ivec3> selectedVoxels;
    void addVoxels(int layers, VoxelWorld& voxelWorld);
    void removeVoxels(int layers, VoxelWorld& voxelWorld);
    void ExtrusionManager::drawVector(const glm::dvec2& start, const glm::dvec2& end, const glm::vec3& color);
};

#endif // EXTRUSION_MANAGER_H
