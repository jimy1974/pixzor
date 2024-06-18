#include "SelectionManager.h"
#include "VoxelWorld.h"
#include <iostream>

using namespace std;

SelectionManager::SelectionManager() : selecting(false) {}

void SelectionManager::startSelection(const glm::ivec3& start) {
    selectionStart = start;
    selecting = true;
    highlightedVoxels.clear(); // Clear previous highlights
}

void SelectionManager::updateSelection(const glm::ivec3& current, VoxelWorld& voxelWorld) {
    selectionEnd = current;
    for (const auto& voxel : highlightedVoxels) {
        voxelWorld.resetHighlight(voxel); // Clear previous highlights
    }
    highlightedVoxels.clear();

    int minX = std::min(selectionStart.x, selectionEnd.x);
    int minY = std::min(selectionStart.y, selectionEnd.y);
    int minZ = std::min(selectionStart.z, selectionEnd.z);
    int maxX = std::max(selectionStart.x, selectionEnd.x);
    int maxY = std::max(selectionStart.y, selectionEnd.y);
    int maxZ = std::max(selectionStart.z, selectionEnd.z);

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                glm::ivec3 voxelPos(x, y, z);
                highlightedVoxels.insert(voxelPos);
                voxelWorld.highlightVoxel(voxelPos); // Highlight voxel
            }
        }
    }

    voxelWorld.generateMeshData(); // Regenerate mesh data to update highlights
}

void SelectionManager::endSelection(VoxelWorld& voxelWorld) {
    if (selecting) {
        int minX = std::min(selectionStart.x, selectionEnd.x);
        int minY = std::min(selectionStart.y, selectionEnd.y);
        int minZ = std::min(selectionStart.z, selectionEnd.z);
        int maxX = std::max(selectionStart.x, selectionEnd.x);
        int maxY = std::max(selectionStart.y, selectionEnd.y);
        int maxZ = std::max(selectionStart.z, selectionEnd.z);

        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                for (int z = minZ; z <= maxZ; ++z) {
                    glm::ivec3 voxelPos(x, y, z);
                    selectedVoxels.insert(voxelPos);
                    voxelWorld.selectVoxel(voxelPos);
                    //cout << "Selected voxel at: " << x << ", " << y << ", " << z << endl;
                }
            }
        }
        selecting = false;
        highlightedVoxels.clear(); // Clear highlights after selection
        voxelWorld.generateMeshData(); // Regenerate mesh data to update selections
    }
}

std::vector<glm::ivec3> SelectionManager::getSelectedVoxels() const {
    return std::vector<glm::ivec3>(selectedVoxels.begin(), selectedVoxels.end());
}

void SelectionManager::clearSelections() {
    selectedVoxels.clear();
}
