#define GLM_ENABLE_EXPERIMENTAL
#include "ExtrusionManager.h"
#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include <algorithm>




/*
void ExtrusionManager::updateExtrusion(const glm::dvec2& currentMousePos, VoxelWorld& voxelWorld) {
    if (!extruding) return;

    glm::dvec2 deltaMouse = currentMousePos - initialMousePos;

    // Check for any sudden large jumps and reset if detected
    if (glm::length(deltaMouse) > 100.0) {
        std::cout << "Detected sudden large jump in mouse position. Resetting current mouse position." << std::endl;
        deltaMouse = glm::dvec2(0.0, 0.0);
    }

    float distance = glm::length(deltaMouse);
    int layers = static_cast<int>(distance / 10.0f);

    std::cout << "Current Mouse Position: " << glm::to_string(currentMousePos) << std::endl;
    std::cout << "Initial Mouse Position: " << glm::to_string(initialMousePos) << std::endl;
    std::cout << "Delta Mouse: " << glm::to_string(deltaMouse) << std::endl;
    std::cout << "Distance: " << distance << std::endl;
    std::cout << "Updating extrusion. Mouse delta: " << glm::to_string(deltaMouse) << ", Layers: " << layers << std::endl;

    if (layers > currentLayers) {
        addVoxels(layers - currentLayers, voxelWorld);
    } else if (layers < currentLayers) {
        removeVoxels(currentLayers - layers, voxelWorld);
    }
    currentLayers = layers;
}*/




void ExtrusionManager::startExtrusion(const glm::ivec3& startVoxel, const glm::vec3& normal, FaceDirection face, const glm::dvec2& initialMousePos) {
    this->extrusionStart = startVoxel;
    this->hitFace = face;
    this->initialMousePos = initialMousePos;
    this->extruding = true;
    this->currentLayers = 0;
    this->newVoxels.clear();

    
    
    // Set extrusionNormal based on face direction
    switch (face) {
        case FaceDirection::UP:
            //std::cout << "face: UP" << std::endl;
            this->extrusionNormal = glm::vec3(0.0f, 1.0f, 0.0f);
            break;
        case FaceDirection::DOWN:
            //std::cout << "face: DOWN" << std::endl;
            this->extrusionNormal = glm::vec3(0.0f, -1.0f, 0.0f);
            break;
        case FaceDirection::LEFT:
            //std::cout << "face: LEFT" << std::endl;
            this->extrusionNormal = glm::vec3(-1.0f, 0.0f, 0.0f);
            break;
        case FaceDirection::RIGHT:
            //std::cout << "face: RIGHT" << std::endl;
            this->extrusionNormal = glm::vec3(1.0f, 0.0f, 0.0f);
            break;
        case FaceDirection::FORWARD:
            //std::cout << "face: FORWARD" << std::endl;
            this->extrusionNormal = glm::vec3(0.0f, 0.0f, 1.0f);
            break;
        case FaceDirection::BACKWARD:
            //std::cout << "face: BACKWARD" << std::endl;
            this->extrusionNormal = glm::vec3(0.0f, 0.0f, -1.0f);
            break;
        default:
            //std::cout << "Warning: Invalid face direction" << std::endl;
            this->extrusionNormal = glm::vec3(0.0f, 0.0f, 0.0f);
            break;
    }

    //std::cout << "Extrusion started at voxel: " << glm::to_string(startVoxel) << " with face: " << face << std::endl;
    //std::cout << "Extrusion normal set to: " << glm::to_string(this->extrusionNormal) << std::endl;
}

void ExtrusionManager::updateExtrusion(const glm::dvec2& currentMousePos, VoxelWorld& voxelWorld) {
    if (!extruding) return;

    glm::dvec2 deltaMouse = currentMousePos - initialMousePos;

    // Handle large jumps
    double distance = glm::length(deltaMouse);
    const double jumpThreshold = 100.0; // Arbitrary large value threshold
    if (distance > jumpThreshold) {
        std::cout << "Detected sudden large jump in mouse position. Limiting layer change." << std::endl;
        deltaMouse = glm::dvec2(0.0, 0.0);
        distance = 0.0;
    }

    // Convert deltaMouse to glm::vec2 for comparison
    glm::vec2 deltaMouse2D(deltaMouse.x, deltaMouse.y);

    // Calculate the projected 2D normal direction
    glm::vec2 projectedNormal;
    switch (hitFace) {
        case FaceDirection::UP:
            projectedNormal = glm::vec2(0.0f, -1.0f); // Inverted due to screen coordinates
            break;
        case FaceDirection::DOWN:
            projectedNormal = glm::vec2(0.0f, 1.0f);
            break;
        case FaceDirection::LEFT:
            projectedNormal = glm::vec2(-1.0f, 0.0f);
            break;
        case FaceDirection::RIGHT:
            projectedNormal = glm::vec2(1.0f, 0.0f);
            break;
        case FaceDirection::FORWARD:
            projectedNormal = glm::vec2(extrusionNormal.x, extrusionNormal.z);
            break;
        case FaceDirection::BACKWARD:
            projectedNormal = glm::vec2(-extrusionNormal.x, -extrusionNormal.z);
            break;
        default:
            projectedNormal = glm::vec2(0.0f, 0.0f); // Handle any unexpected cases
            break;
    }

    // Normalize the projected normal if it's not zero
    if (glm::length(projectedNormal) > 0.0f) {
        projectedNormal = glm::normalize(projectedNormal);
    } else {
        std::cout << "Warning: Zero-length normal vector" << std::endl;
    }

    // Determine if the mouse movement is in the same direction as the normal
    float direction = glm::dot(glm::normalize(deltaMouse2D), projectedNormal);

    // Calculate layers with a minimum threshold to avoid 0 layers due to small movements
    int layers = static_cast<int>((distance + 0.5f) / 10.0f);

    std::cout << "Current Mouse Position: " << glm::to_string(currentMousePos) << std::endl;
    std::cout << "Initial Mouse Position: " << glm::to_string(initialMousePos) << std::endl;
    std::cout << "Delta Mouse: " << glm::to_string(deltaMouse) << std::endl;
    std::cout << "Distance: " << distance << std::endl;
    std::cout << "Projected Normal: " << glm::to_string(projectedNormal) << std::endl;
    std::cout << "Direction: " << direction << std::endl;
    std::cout << "Layers: " << layers << std::endl;
    std::cout << "Current Layers: " << currentLayers << std::endl;

    if (direction > 0.1) { // Mouse moved in the same direction as the face normal
        if (layers > currentLayers) {
            int layersToAdd = layers - currentLayers;
            std::cout << "Adding Voxels: " << layersToAdd << " layers" << std::endl;
            addVoxels(layersToAdd, voxelWorld);
            currentLayers = layers;
        }
    } else if (direction < -0.1) { // Mouse moved in the opposite direction of the face normal
        if (layers > currentLayers) { // Note the change here from layers < currentLayers
            int layersToRemove = layers - currentLayers;
            std::cout << "Removing Voxels: " << layersToRemove << " layers" << std::endl;
            removeVoxels(layersToRemove, voxelWorld);
            currentLayers = layers;
        }
    }

    // Draw the vectors for visualization
    drawVector(initialMousePos, currentMousePos, glm::vec3(1.0f, 0.0f, 0.0f)); // Delta Mouse vector in red
    drawVector(initialMousePos, initialMousePos + glm::dvec2(projectedNormal.x, projectedNormal.y) * 100.0, glm::vec3(0.0f, 1.0f, 0.0f)); // Projected normal in green
}


void ExtrusionManager::addVoxels(int layers, VoxelWorld& voxelWorld) {
    std::cout << "Adding Voxels: " << layers << " layers" << std::endl;
    for (const auto& voxel : selectedVoxels) {
        for (int i = 0; i < layers; ++i) {
            glm::ivec3 newVoxelPos = voxel + glm::ivec3(extrusionNormal * static_cast<float>(currentLayers + 1 + i));
            voxelWorld.setVoxel(newVoxelPos.x, newVoxelPos.y, newVoxelPos.z, 1, "white", "default");
            newVoxels.insert(newVoxelPos);
            std::cout << "Added voxel at: " << glm::to_string(newVoxelPos) << std::endl;
        }
    }
}

void ExtrusionManager::removeVoxels(int layers, VoxelWorld& voxelWorld) {
    std::cout << "Removing Voxels: " << layers << " layers" << std::endl;
    for (const auto& voxel : selectedVoxels) {
        for (int i = 0; i < layers; ++i) {
            glm::ivec3 voxelToRemove = voxel + glm::ivec3(extrusionNormal * static_cast<float>(currentLayers - 1 - i));
            voxelWorld.removeVoxel(voxelToRemove); // Use the new method to remove the voxel
            newVoxels.erase(voxelToRemove);
            std::cout << "Removed voxel at: " << glm::to_string(voxelToRemove) << std::endl;
        }
    }
}




void ExtrusionManager::drawVector(const glm::dvec2& start, const glm::dvec2& end, const glm::vec3& color) {
    // Implement the function to draw a line from start to end with the given color
    // This function depends on your graphics API (e.g., OpenGL, DirectX)
}







void ExtrusionManager::endExtrusion(VoxelWorld& voxelWorld) {
    if (extruding) {
        std::cout << "Extrusion ended." << std::endl;
        extruding = false;
    }
}

bool ExtrusionManager::isExtruding() const {
    return extruding;
}

glm::ivec3 ExtrusionManager::getExtrusionStart() const {
    return extrusionStart;
}

void ExtrusionManager::setSelectedVoxels(const std::unordered_set<glm::ivec3>& selectedVoxels) {
    this->selectedVoxels = selectedVoxels;
}

void ExtrusionManager::clearSelectedVoxels() {
    selectedVoxels.clear();
    newVoxels.clear(); // Also clear the newVoxels set to avoid any residual state
}

