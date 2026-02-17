"""
Batch mirror BVH files and export to FBX using Blender's Python API.

This script mirrors animation data consistently with AI4Animation/SIGGRAPH_2022/Unity:
- Mirror axis: X-axis (across Y-Z plane)
- Position mirroring: Negate X component
- Rotation mirroring: Negate quaternion X and W components
- Bone symmetry: Swap Left<->Right bones using pattern matching

Usage:
    blender --background --python bvh_mirror_batch.py -- <input_path> <output_dir> [--suffix SUFFIX]

    input_path: Either a single .bvh file or a directory containing .bvh files

Example:
    blender --background --python bvh_mirror_batch.py -- "D:\anim-ws\MANN_qudruped_data" "D:\anim-ws\MANN_qudruped_data\fbx_output"
    & "C:\Program Files\Blender Foundation\Blender 4.2\blender.exe" --background --python bvh_mirror_batch.py -- "D:\anim-ws\MANN_qudruped_data" "D:\anim-ws\MANN_qudruped_data\fbx_output"
    & "C:\Program Files\Blender Foundation\Blender 4.2\blender.exe" --background --python bvh_mirror_batch.py -- "D:\anim-ws\MANN_qudruped_data\D1_053_KAN01_002.bvh" "D:\anim-ws\MANN_qudruped_data\fbx_output"
"""

import bpy
import sys
import re
from pathlib import Path
from mathutils import Vector, Quaternion, Matrix


# Symmetry detection patterns (same as Unity AI4Animation)
# Each tuple: (left_pattern, right_pattern, is_prefix, is_suffix, is_infix)
SYMMETRY_PATTERNS = [
    ("Left", "Right", False, False, False),   # Contains "Left" <-> "Right"
    ("left", "right", False, False, False),
    ("_L_", "_R_", False, False, True),       # Infix "_L_" <-> "_R_"
    ("_l_", "_r_", False, False, True),
    ("L_", "R_", True, False, False),          # Prefix "L_" <-> "R_"
    ("l_", "r_", True, False, False),
    ("_L", "_R", False, True, False),          # Suffix "_L" <-> "_R"
    ("_l", "_r", False, True, False),
    (".L", ".R", False, True, False),          # Blender convention suffix
    (".l", ".r", False, True, False),
]


def find_mirror_bone_name(bone_name, all_bone_names):
    """
    Find the mirrored bone name using Unity-compatible symmetry patterns.
    Returns the mirrored bone name, or the same name if no mirror found.
    """
    for left_pat, right_pat, is_prefix, is_suffix, is_infix in SYMMETRY_PATTERNS:
        # Check if this bone matches the left pattern
        if is_prefix:
            if bone_name.startswith(left_pat):
                mirror_name = right_pat + bone_name[len(left_pat):]
                if mirror_name in all_bone_names:
                    return mirror_name
            elif bone_name.startswith(right_pat):
                mirror_name = left_pat + bone_name[len(right_pat):]
                if mirror_name in all_bone_names:
                    return mirror_name
        elif is_suffix:
            if bone_name.endswith(left_pat):
                mirror_name = bone_name[:-len(left_pat)] + right_pat
                if mirror_name in all_bone_names:
                    return mirror_name
            elif bone_name.endswith(right_pat):
                mirror_name = bone_name[:-len(right_pat)] + left_pat
                if mirror_name in all_bone_names:
                    return mirror_name
        elif is_infix:
            if left_pat in bone_name:
                mirror_name = bone_name.replace(left_pat, right_pat, 1)
                if mirror_name in all_bone_names:
                    return mirror_name
            elif right_pat in bone_name:
                mirror_name = bone_name.replace(right_pat, left_pat, 1)
                if mirror_name in all_bone_names:
                    return mirror_name
        else:
            # General contains check
            if left_pat in bone_name:
                mirror_name = bone_name.replace(left_pat, right_pat, 1)
                if mirror_name in all_bone_names:
                    return mirror_name
            elif right_pat in bone_name:
                mirror_name = bone_name.replace(right_pat, left_pat, 1)
                if mirror_name in all_bone_names:
                    return mirror_name

    return bone_name  # No mirror found, maps to itself


def build_symmetry_map(armature):
    """
    Build a symmetry mapping for all bones in the armature.
    Returns dict: {bone_name: mirrored_bone_name}
    """
    bone_names = [bone.name for bone in armature.pose.bones]
    symmetry_map = {}

    for bone_name in bone_names:
        mirror_name = find_mirror_bone_name(bone_name, bone_names)
        symmetry_map[bone_name] = mirror_name

    return symmetry_map


def mirror_vector_x(vec):
    """Mirror a vector across the Y-Z plane (negate X component)."""
    return Vector((-vec.x, vec.y, vec.z))


def mirror_quaternion_x(quat):
    """
    Mirror a quaternion across the Y-Z plane (X-axis mirror).
    Unity implementation: negate X and W components.
    """
    return Quaternion((-quat.w, -quat.x, quat.y, quat.z))


def mirror_matrix_x(matrix):
    """
    Mirror a 4x4 transformation matrix across the Y-Z plane.
    Matches Unity's Matrix4x4Extensions.GetMirror for XPositive axis.
    """
    m = matrix.copy()
    # Position X
    m[0][3] *= -1
    # Rotation components
    m[0][1] *= -1  # up.x
    m[0][2] *= -1  # forward.x
    m[1][0] *= -1  # right.y
    m[2][0] *= -1  # right.z
    return m


def get_bone_world_matrix(pose_bone):
    """Get the world-space transformation matrix of a pose bone."""
    return pose_bone.id_data.matrix_world @ pose_bone.matrix


def set_bone_world_matrix(pose_bone, world_matrix):
    """Set a pose bone's transformation from a world-space matrix."""
    armature_inv = pose_bone.id_data.matrix_world.inverted()
    pose_bone.matrix = armature_inv @ world_matrix


def mirror_animation(armature, symmetry_map):
    """
    Mirror all keyframes of the armature animation.
    This swaps bone transforms between symmetric pairs and applies X-axis mirroring.
    """
    if not armature.animation_data or not armature.animation_data.action:
        print("  Warning: No animation data found")
        return

    action = armature.animation_data.action
    scene = bpy.context.scene

    # Get frame range
    frame_start = int(action.frame_range[0])
    frame_end = int(action.frame_range[1])

    # Store all transforms for all frames first (we need to swap between bones)
    frame_transforms = {}  # {frame: {bone_name: (location, rotation_quaternion, scale)}}

    for frame in range(frame_start, frame_end + 1):
        scene.frame_set(frame)
        frame_transforms[frame] = {}

        for pose_bone in armature.pose.bones:
            # Store world-space matrix
            world_matrix = get_bone_world_matrix(pose_bone)
            frame_transforms[frame][pose_bone.name] = world_matrix.copy()

    # Now apply mirrored transforms
    for frame in range(frame_start, frame_end + 1):
        scene.frame_set(frame)

        for pose_bone in armature.pose.bones:
            bone_name = pose_bone.name
            mirror_bone_name = symmetry_map.get(bone_name, bone_name)

            # Get the transform from the mirror bone
            source_matrix = frame_transforms[frame][mirror_bone_name]

            # Apply X-axis mirroring to the matrix
            mirrored_matrix = mirror_matrix_x(source_matrix)

            # Set the mirrored transform
            set_bone_world_matrix(pose_bone, mirrored_matrix)

            # Insert keyframes
            pose_bone.keyframe_insert(data_path="location", frame=frame)
            pose_bone.keyframe_insert(data_path="rotation_quaternion", frame=frame)
            if pose_bone.rotation_mode == 'QUATERNION':
                pose_bone.keyframe_insert(data_path="rotation_quaternion", frame=frame)
            else:
                pose_bone.keyframe_insert(data_path="rotation_euler", frame=frame)
            pose_bone.keyframe_insert(data_path="scale", frame=frame)


def mirror_animation_fcurves(armature, symmetry_map):
    """
    Mirror animation by directly manipulating F-Curves.
    More efficient and preserves exact keyframe data.
    """
    if not armature.animation_data or not armature.animation_data.action:
        print("  Warning: No animation data found")
        return

    action = armature.animation_data.action

    # Group F-Curves by bone
    bone_fcurves = {}  # {bone_name: {data_path_suffix: {index: fcurve}}}

    for fcurve in action.fcurves:
        # Parse data path like 'pose.bones["BoneName"].location'
        match = re.match(r'pose\.bones\["([^"]+)"\]\.(.+)', fcurve.data_path)
        if match:
            bone_name = match.group(1)
            prop_path = match.group(2)

            if bone_name not in bone_fcurves:
                bone_fcurves[bone_name] = {}
            if prop_path not in bone_fcurves[bone_name]:
                bone_fcurves[bone_name][prop_path] = {}

            bone_fcurves[bone_name][prop_path][fcurve.array_index] = fcurve

    print(f"  Processing {len(bone_fcurves)} bones with animation data...")

    # Store all keyframe values first
    stored_values = {}  # {bone_name: {prop_path: {index: [keyframe_data]}}}

    for bone_name, props in bone_fcurves.items():
        stored_values[bone_name] = {}
        for prop_path, indices in props.items():
            stored_values[bone_name][prop_path] = {}
            for idx, fcurve in indices.items():
                keyframes = []
                for kp in fcurve.keyframe_points:
                    keyframes.append({
                        'co': (kp.co[0], kp.co[1]),
                        'handle_left': (kp.handle_left[0], kp.handle_left[1]),
                        'handle_right': (kp.handle_right[0], kp.handle_right[1]),
                        'interpolation': kp.interpolation,
                    })
                stored_values[bone_name][prop_path][idx] = keyframes

    print(f"  Stored keyframe data, applying mirrored transforms...")

    # Apply mirrored values by modifying keyframe values in place
    bones_processed = 0
    for bone_name, props in bone_fcurves.items():
        mirror_bone_name = symmetry_map.get(bone_name, bone_name)

        if mirror_bone_name not in stored_values:
            continue

        for prop_path, indices in props.items():
            if prop_path not in stored_values[mirror_bone_name]:
                continue

            for idx, fcurve in indices.items():
                if idx not in stored_values[mirror_bone_name][prop_path]:
                    continue

                source_keyframes = stored_values[mirror_bone_name][prop_path][idx]

                # Determine if this component needs sign flip for X-axis mirroring
                flip_sign = False
                if prop_path == 'location' and idx == 0:  # X component
                    flip_sign = True
                elif prop_path == 'rotation_quaternion':
                    if idx == 0:  # W component
                        flip_sign = True
                    elif idx == 1:  # X component
                        flip_sign = True
                elif prop_path == 'rotation_euler' and idx == 0:  # X rotation
                    flip_sign = True

                # Modify keyframes in place (much faster than remove/insert)
                kp_list = fcurve.keyframe_points
                num_source = len(source_keyframes)
                num_existing = len(kp_list)

                # Ensure we have the right number of keyframes
                if num_existing != num_source:
                    # Need to rebuild - clear and add
                    # Use faster batch clear
                    fcurve.keyframe_points.clear()
                    fcurve.keyframe_points.add(num_source)

                # Set values
                for i, kf_data in enumerate(source_keyframes):
                    frame, value = kf_data['co']
                    if flip_sign:
                        value = -value

                    kp = kp_list[i]
                    kp.co = (frame, value)

                    # Copy handle data
                    hl_x, hl_y = kf_data['handle_left']
                    hr_x, hr_y = kf_data['handle_right']
                    if flip_sign:
                        hl_y = -hl_y
                        hr_y = -hr_y
                    kp.handle_left = (hl_x, hl_y)
                    kp.handle_right = (hr_x, hr_y)
                    kp.interpolation = kf_data['interpolation']

                fcurve.update()

        bones_processed += 1
        if bones_processed % 10 == 0:
            print(f"  Processed {bones_processed}/{len(bone_fcurves)} bones...")

    print(f"  Mirroring complete.")


def convert_and_mirror_bvh_to_fbx(bvh_path, fbx_path):
    """Convert a single BVH file to mirrored FBX."""
    # Clear the scene
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()

    # Import BVH
    bpy.ops.import_anim.bvh(filepath=str(bvh_path))

    # Find the armature
    armature = None
    for obj in bpy.context.scene.objects:
        if obj.type == 'ARMATURE':
            armature = obj
            break

    if not armature:
        raise RuntimeError("No armature found after BVH import")

    # Build symmetry map
    symmetry_map = build_symmetry_map(armature)

    # Report symmetry pairs found
    pairs_found = sum(1 for k, v in symmetry_map.items() if k != v) // 2
    print(f"  Found {pairs_found} symmetric bone pairs")

    # Mirror the animation using F-Curves (more efficient)
    mirror_animation_fcurves(armature, symmetry_map)

    # Select all for export
    bpy.ops.object.select_all(action='SELECT')

    # Export FBX
    bpy.ops.export_scene.fbx(
        filepath=str(fbx_path),
        use_selection=True,
        bake_anim=True,
        add_leaf_bones=False
    )


def main():
    # Get arguments after "--"
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        print("Error: Please provide input path and output directory after '--'")
        print("Usage: blender --background --python bvh_mirror_batch.py -- <input_path> <output_dir> [--suffix SUFFIX]")
        sys.exit(1)

    if len(argv) < 2:
        print("Error: Please provide both input path and output directory")
        sys.exit(1)

    input_path = Path(argv[0])
    output_dir = Path(argv[1])

    # Parse optional suffix
    suffix = "_mirrored"
    if "--suffix" in argv:
        suffix_idx = argv.index("--suffix")
        if suffix_idx + 1 < len(argv):
            suffix = argv[suffix_idx + 1]

    if not input_path.exists():
        print(f"Error: Input path does not exist: {input_path}")
        sys.exit(1)

    output_dir.mkdir(parents=True, exist_ok=True)

    # Support both single file and directory input
    if input_path.is_file():
        if input_path.suffix.lower() != ".bvh":
            print(f"Error: Input file is not a BVH file: {input_path}")
            sys.exit(1)
        bvh_files = [input_path]
        print(f"Mirroring single file: {input_path.name}")
    else:
        bvh_files = list(input_path.glob("*.bvh"))
        print(f"Found {len(bvh_files)} BVH files in {input_path}")

    print(f"Output suffix: '{suffix}'")

    success_count = 0
    error_count = 0

    for i, bvh_file in enumerate(bvh_files, 1):
        fbx_file = output_dir / (bvh_file.stem + suffix + ".fbx")
        print(f"[{i}/{len(bvh_files)}] Mirroring: {bvh_file.name} -> {fbx_file.name}")
        try:
            convert_and_mirror_bvh_to_fbx(bvh_file, fbx_file)
            success_count += 1
        except Exception as e:
            print(f"  Error: {e}")
            error_count += 1

    print(f"\nDone! Mirrored {success_count} files to {output_dir}")
    if error_count > 0:
        print(f"Errors: {error_count} files failed")


if __name__ == "__main__":
    main()
