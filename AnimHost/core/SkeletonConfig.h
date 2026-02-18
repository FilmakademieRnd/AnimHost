/*
 ***************************************************************************************

 *   Copyright (c) 2024 Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs
 *   https://research.animationsinstitut.de/animhost
 *   https://github.com/FilmakademieRnd/AnimHost
 *
 *   AnimHost is a development by Filmakademie Baden-Wuerttemberg, Animationsinstitut
 *   R&D Labs in the scope of the EU funded project MAX-R (101070072).
 *
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE. See the MIT License for more details.
 *   You should have received a copy of the MIT License along with this program;
 *   if not go to https://opensource.org/licenses/MIT

 ***************************************************************************************
 */

#ifndef SKELETONCONFIG_H
#define SKELETONCONFIG_H

#include <vector>
#include <string>

/**
 * Enum for skeleton type selection.
 * Used by multiple plugins to configure skeleton-specific processing.
 */
enum class SkeletonType {
    Bipedal = 0,
    Quadrupedal = 1
};

/**
 * Bone name configuration for root transform calculation.
 * Used by LocomotionPreprocessNode to determine character facing direction.
 */
struct SkeletonBoneConfig {
    const char* rootBone;       ///< Center of mass / root position bone
    const char* rearRight;      ///< Right rear joint (hip for biped, back leg for quad)
    const char* rearLeft;       ///< Left rear joint
    const char* frontRight;     ///< Right front joint (shoulder for biped, front leg for quad)
    const char* frontLeft;      ///< Left front joint
};

/**
 * Sub-skeleton configuration for filtering FBX scene hierarchy nodes.
 * Used by AssimpLoader to extract only the actual skeleton bones from FBX files,
 * removing scene root nodes like filenames and "RootNode" entries.
 */
struct SubSkeletonConfig {
    const char* rootBone;                       ///< Skeleton root bone name (e.g., "hip", "Hips")
    std::vector<std::string> leafBones;         ///< End effector bones to stop hierarchy traversal
    bool applyChangeOfBasis;                    ///< Whether to transform root bone coordinates
};

/**
 * Bone configurations for root transform calculation (LocomotionPreprocessNode).
 */
namespace SkeletonBoneConfigs {
    /// Biped skeleton (e.g., Animationsinstitut SURVIVOR robot)
    constexpr SkeletonBoneConfig Bipedal = {
        "hip",          // rootBone
        "pelvis_R",     // rearRight
        "pelvis_L",     // rearLeft
        "shoulder_R",   // frontRight
        "shoulder_L"    // frontLeft
    };

    /// Quadruped skeleton (e.g., MANN/DeepPhase dog)
    constexpr SkeletonBoneConfig Quadrupedal = {
        "Hips",             // rootBone
        "RightUpLeg",       // rearRight
        "LeftUpLeg",        // rearLeft
        "RightShoulder",    // frontRight
        "LeftShoulder"      // frontLeft
    };
}

/**
 * Sub-skeleton configurations for FBX import filtering (AssimpLoader).
 */
namespace SubSkeletonConfigs {
    /**
     * Bipedal sub-skeleton (Animationsinstitut Survivor robot).
     * Source data uses Y-up coordinate system, requires basis transformation.
     */
    inline const SubSkeletonConfig& Bipedal() {
        static const SubSkeletonConfig config = {
            "hip",
            { "hand_R", "hand_L", "head", "toe_L", "toe_R", "heel_02_L", "heel_02_R" },
            true  // Apply Y-up to Z-up transformation
        };
        return config;
    }

    /**
     * Quadrupedal sub-skeleton (MANN/DeepPhase dog).
     * Source data uses Y-up coordinate system (same as Survivor), requires basis transformation.
     */
    inline const SubSkeletonConfig& Quadrupedal() {
        static const SubSkeletonConfig config = {
            "Hips",
            { "Head", "LeftHandSite", "RightHandSite", "LeftFootSite", "RightFootSite", "Tail1Site" },
            true  // Apply Y-up to Z-up transformation (MANN data is Y-up)
        };
        return config;
    }
}

/**
 * Get the sub-skeleton configuration for a given skeleton type.
 * @param type The skeleton type
 * @return Reference to the corresponding SubSkeletonConfig
 */
inline const SubSkeletonConfig& getSubSkeletonConfig(SkeletonType type) {
    switch (type) {
    case SkeletonType::Quadrupedal:
        return SubSkeletonConfigs::Quadrupedal();
    case SkeletonType::Bipedal:
    default:
        return SubSkeletonConfigs::Bipedal();
    }
}

/**
 * Get the bone configuration for root transform calculation.
 * @param type The skeleton type
 * @return Reference to the corresponding SkeletonBoneConfig
 */
inline const SkeletonBoneConfig& getSkeletonBoneConfig(SkeletonType type) {
    switch (type) {
    case SkeletonType::Quadrupedal:
        return SkeletonBoneConfigs::Quadrupedal;
    case SkeletonType::Bipedal:
    default:
        return SkeletonBoneConfigs::Bipedal;
    }
}

#endif // SKELETONCONFIG_H
