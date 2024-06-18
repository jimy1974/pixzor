#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>
#include "VoxelWorld.h"
#include "glm_hash.h" // Include custom hash header

class SelectionManager {
public:
    SelectionManager();
    void startSelection(const glm::ivec3& start);
    void updateSelection(const glm::ivec3& current, VoxelWorld& voxelWorld);
    void endSelection(VoxelWorld& voxelWorld);
    void clearSelections(); // Add this method

    std::vector<glm::ivec3> getSelectedVoxels() const;

private:
    glm::ivec3 selectionStart;
    glm::ivec3 selectionEnd;
    bool selecting;
    std::unordered_set<glm::ivec3> selectedVoxels;
    std::unordered_set<glm::ivec3> highlightedVoxels;
};


#endif // SELECTION_MANAGER_H