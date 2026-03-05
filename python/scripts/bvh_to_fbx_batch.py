"""
Batch convert BVH files to FBX using Blender's Python API.

Versioning
- v0: Initial implementation with working BVH to FBX conversion (scale 100x off, skeleton orientations use different axis conventions)
- v1: Consistent scale, consistent skeleton orientations

Usage:
    blender --background --python bvh_to_fbx_batch.py -- <input_path> <output_dir>

    input_path: Either a single .bvh file or a directory containing .bvh files

Example:
    blender --background --python bvh_to_fbx_batch.py -- "D:\anim-ws\MANN_qudruped_data" "D:\anim-ws\MANN_qudruped_data\fbx_output"
    & "D:\anim-sw\Blender Foundation\Blender 4.2\blender.exe" --background --python bvh_to_fbx_batch.py -- "D:\anim-ws\MANN_qudruped_data\p100" "D:\anim-ws\MANN_qudruped_data\p100\fbx-v1"
    & "D:\anim-sw\Blender Foundation\Blender 4.2\blender.exe" --background --python bvh_to_fbx_batch.py -- "D:\anim-ws\MANN_qudruped_data\p50\D1_007_KAN01_001.bvh" "D:\anim-ws\MANN_qudruped_data\p50\fbx"
"""

import bpy
import sys
from pathlib import Path
from mathutils import Vector


def parse_bvh_frame_count(bvh_path):
    """Parse BVH file to extract the frame count from the MOTION section."""
    with open(bvh_path, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('Frames:'):
                return int(line.split(':')[1].strip())
    return 0


def parse_bvh_end_sites(bvh_path):
    """Parse BVH file to extract End Site offsets with their parent joint names.

    Returns a dict: {parent_joint_name: Vector(offset)}
    Excludes Head End Site to match Unity pipeline convention.
    """
    end_sites = {}

    with open(bvh_path, 'r') as f:
        content = f.read()

    lines = content.split('\n')
    joint_stack = []
    i = 0

    while i < len(lines):
        line = lines[i].strip()

        if line.startswith('ROOT') or line.startswith('JOINT'):
            parts = line.split()
            joint_name = parts[1] if len(parts) > 1 else None
            if joint_name:
                joint_stack.append(joint_name)

        if line == '}':
            if joint_stack:
                joint_stack.pop()

        if 'End Site' in line:
            parent_joint = joint_stack[-1] if joint_stack else None

            # Skip Head End Site to match Unity pipeline
            if parent_joint and parent_joint != 'Head':
                for j in range(i + 1, min(i + 5, len(lines))):
                    offset_line = lines[j].strip()
                    if offset_line.startswith('OFFSET'):
                        parts = offset_line.split()
                        if len(parts) >= 4:
                            offset = Vector((float(parts[1]), float(parts[2]), float(parts[3])))
                            end_sites[parent_joint] = offset
                        break

        i += 1

    return end_sites


def add_end_site_bones(armature, end_sites):
    """Add End Site bones to the armature with correct offsets.

    Args:
        armature: The Blender armature object
        end_sites: Dict of {parent_joint_name: Vector(offset)}
    """
    # Enter edit mode to modify bones
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode='EDIT')

    edit_bones = armature.data.edit_bones

    for parent_name, offset in end_sites.items():
        # Find the parent bone
        parent_bone = edit_bones.get(parent_name)
        if not parent_bone:
            print(f"  Warning: Parent bone '{parent_name}' not found, skipping End Site")
            continue

        # Create the End Site bone
        site_name = f"{parent_name}Site"
        site_bone = edit_bones.new(site_name)

        # Blender's BVH importer already computed parent_bone.tail as the End Site position
        # (using parent's world rotation × offset internally)
        # Just use that position and extend using the 3D offset in local coordinates
        site_bone.head = parent_bone.tail.copy()

        if offset.length > 0:
            # Build local coordinate frame from parent bone
            # BVH offset convention: (x=forward, y=right, z=up)
            bone_forward = (parent_bone.tail - parent_bone.head).normalized()
            world_up = Vector((0, 1, 0))
            bone_right = bone_forward.cross(world_up).normalized()
            bone_up = bone_right.cross(bone_forward).normalized()

            world_offset = (bone_forward * offset.x +
                           bone_right * offset.y +
                           bone_up * offset.z)

            site_bone.tail = site_bone.head + world_offset
        else:
            print(f"  ERROR: Zero-length offset for End Site '{site_name}', skipping")
            edit_bones.remove(site_bone)
            continue

        # Set parent
        site_bone.parent = parent_bone

        print(f"  Added End Site bone: {site_name} at offset {offset}")

    # Return to object mode
    bpy.ops.object.mode_set(mode='OBJECT')


def convert_bvh_to_fbx(bvh_path, fbx_path):
    """Convert a single BVH file to FBX with proper End Site bones."""
    # Clear scene - use direct removal for reliability (operators can fail silently)
    for obj in bpy.data.objects:
        bpy.data.objects.remove(obj, do_unlink=True)
    # Clear actions (animation data persists separately from objects!)
    for action in bpy.data.actions:
        bpy.data.actions.remove(action, do_unlink=True)
    # Clear armature data blocks
    for armature in bpy.data.armatures:
        bpy.data.armatures.remove(armature, do_unlink=True)

    # Parse frame count and End Sites from BVH before import
    bvh_frames = parse_bvh_frame_count(bvh_path)
    end_sites = parse_bvh_end_sites(bvh_path)
    print(f"  Found {len(end_sites)} End Sites: {list(end_sites.keys())}")

    # Import BVH
    bpy.ops.import_anim.bvh(filepath=str(bvh_path))

    # Find the imported armature
    armature = None
    for obj in bpy.context.scene.objects:
        if obj.type == 'ARMATURE':
            armature = obj
            break

    # Get frame count from armature's action (the actual animation data)
    fbx_frames = 0
    frame_start = 0
    frame_end = 0
    if armature and armature.animation_data and armature.animation_data.action:
        action = armature.animation_data.action
        frame_start, frame_end = action.frame_range
        fbx_frames = int(frame_end - frame_start) + 1
    print(f"  Frames: BVH={bvh_frames}, FBX={fbx_frames}, Range={frame_start}-{frame_end}")

    if armature and end_sites:
        add_end_site_bones(armature, end_sites)

    if not armature:
        print(f"  ERROR: No armature found after import, skipping export")
        return

    # Select only this armature for export (not select_all which could grab stale objects)
    bpy.ops.object.select_all(action='DESELECT')
    armature.select_set(True)
    bpy.context.view_layer.objects.active = armature

    # Set scene frame range to match the animation (FBX exporter uses scene range)
    # IMPORTANT: bpy.ops.export_scene.fbx() does NOT have frame_start/frame_end parameters!
    # It uses the scene's frame range instead - this is documented behavior
    bpy.context.scene.frame_start = int(frame_start)
    bpy.context.scene.frame_end = int(frame_end)
    print(f"  Set scene frame range: {int(frame_start)}-{int(frame_end)}")

    # Export FBX with full animation data
    # API Reference: https://docs.blender.org/api/4.2/bpy.ops.export_scene.html#bpy.ops.export_scene.fbx
    # To verify parameters in your Blender version, run in Python console: help(bpy.ops.export_scene.fbx)
    bpy.ops.export_scene.fbx(
        # === File & Selection ===
        filepath=str(fbx_path),  # (string) Output file path
        use_selection=True,  # (bool) Export only selected objects (our armature)

        # === Scale ===
        global_scale=0.01,  # (float) Scale all data. Keep at 1.0 to preserve cm scale for Unity pipeline

        # === Transform & Axis Conversion ===
        axis_forward='-Z',
        axis_up='Y',

        # === Animation Baking ===
        bake_anim=True,  # (bool) Export baked keyframe animation
        bake_anim_use_all_bones=True,  # (bool) Force exporting at least one key of animation for all bones (ensures consistent keyframe counts)
        bake_anim_use_nla_strips=False,  # (bool) Export each non-muted NLA strip as a separated FBX AnimStack (disabled - single animation)
        bake_anim_use_all_actions=False,  # (bool) Export each action as a separated FBX AnimStack (disabled - single animation)
        bake_anim_step=1.0,  # (float, 0.01-100) Sampling rate - how often to evaluate animated values in frames. 1.0 = every frame
        bake_anim_simplify_factor=0.0,  # (float, 0-100) Simplification - how much to simplify baked values. 0.0 = disabled (keep ALL keyframes)
        bake_anim_force_startend_keying=True,  # (bool) Always add a keyframe at start and end of actions for animated channels

        # === Armature ===
        add_leaf_bones=False  # (bool) Append a final bone to the end of each chain to specify last bone length (disabled - we add custom End Site bones)
    )


def main():
    # Get arguments after "--"
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        print("Error: Please provide input path and output directory after '--'")
        print("Usage: blender --background --python bvh_to_fbx_batch.py -- <input_dir> <output_dir>")
        sys.exit(1)

    if len(argv) < 2:
        print("Error: Please provide both input path and output directory")
        sys.exit(1)

    input_path = Path(argv[0])
    output_dir = Path(argv[1])

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
        print(f"Converting single file: {input_path.name}")
    else:
        bvh_files = list(input_path.glob("*.bvh"))
        print(f"Found {len(bvh_files)} BVH files in {input_path}")

    for i, bvh_file in enumerate(bvh_files, 1):
        fbx_file = output_dir / (bvh_file.stem + ".fbx")
        print(f"[{i}/{len(bvh_files)}] Converting: {bvh_file.name} -> {fbx_file.name}")
        try:
            convert_bvh_to_fbx(bvh_file, fbx_file)
        except Exception as e:
            print(f"  Error converting {bvh_file.name}: {e}")

    print(f"\nDone! Converted {len(bvh_files)} files to {output_dir}")


if __name__ == "__main__":
    main()
