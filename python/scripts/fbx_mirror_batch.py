"""
Batch mirror FBX/BVH animation files using Blender's native pose mirroring.

Uses Blender's built-in pose flip functionality (pose.paste with flipped=True)
which handles bone symmetry detection (.L/.R, Left/Right, etc.) and proper
transformation mirroring automatically.

Usage:
    blender --background --python fbx_mirror_batch.py -- <input_path> <output_dir> [--suffix SUFFIX]

    input_path: Either a single .fbx/.bvh file or a directory containing animation files

Example:
    blender --background --python fbx_mirror_batch.py -- "D:\anim-ws\MANN_qudruped_data\fbx_output" "D:\anim-ws\MANN_qudruped_data\fbx_output"
    & "C:\Program Files\Blender Foundation\Blender 4.2\blender.exe" --background --python fbx_mirror_batch.py -- "D:\anim-ws\MANN_qudruped_data\fbx_output" "D:\anim-ws\MANN_qudruped_data\fbx_output"
    & "C:\Program Files\Blender Foundation\Blender 4.2\blender.exe" --background --python fbx_mirror_batch.py -- "D:\anim-ws\MANN_qudruped_data\fbx_output\D1_053_KAN01_002.fbx" "D:\anim-ws\MANN_qudruped_data\fbx_output"
"""

import bpy
import sys
import io
import contextlib
from pathlib import Path


@contextlib.contextmanager
def suppress_output():
    """Context manager to suppress Blender operator output (stdout and stderr)."""
    old_stdout = sys.stdout
    old_stderr = sys.stderr
    sys.stdout = io.StringIO()
    sys.stderr = io.StringIO()
    try:
        yield
    finally:
        sys.stdout = old_stdout
        sys.stderr = old_stderr


def clear_scene():
    """Clear all objects from the scene."""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()


def import_animation(file_path):
    """Import FBX or BVH file, return the armature object."""
    ext = file_path.suffix.lower()

    if ext == '.fbx':
        bpy.ops.import_scene.fbx(filepath=str(file_path))
    elif ext == '.bvh':
        bpy.ops.import_anim.bvh(filepath=str(file_path))
    else:
        raise ValueError(f"Unsupported file format: {ext}")

    # Find the armature
    for obj in bpy.context.scene.objects:
        if obj.type == 'ARMATURE':
            return obj

    raise RuntimeError("No armature found after import")


def mirror_animation_native(armature):
    """
    Mirror animation using Blender's native pose flip.
    This uses Blender's built-in symmetry detection and transformation.
    """
    if not armature.animation_data or not armature.animation_data.action:
        print("  Warning: No animation data found")
        return

    action = armature.animation_data.action
    scene = bpy.context.scene

    # Get frame range
    frame_start = int(action.frame_range[0])
    frame_end = int(action.frame_range[1])
    total_frames = frame_end - frame_start + 1

    print(f"  Frame range: {frame_start} - {frame_end} ({total_frames} frames)")

    # Set armature as active and enter pose mode
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode='POSE')

    # Select all pose bones
    bpy.ops.pose.select_all(action='SELECT')

    # Store original poses for all frames first
    # (we need to read all before writing any, since mirroring swaps bones)
    print("  Pass 1: Storing original poses...")
    stored_poses = {}

    for frame in range(frame_start, frame_end + 1):
        scene.frame_set(frame)
        # Store a snapshot of bone transforms (no need for pose.copy here)
        stored_poses[frame] = {}
        for pbone in armature.pose.bones:
            stored_poses[frame][pbone.name] = {
                'location': pbone.location.copy(),
                'rotation_quaternion': pbone.rotation_quaternion.copy(),
                'rotation_euler': pbone.rotation_euler.copy(),
                'scale': pbone.scale.copy(),
            }

        if (frame - frame_start) % 500 == 0:
            print(f"    Stored frame {frame - frame_start + 1}/{total_frames}")

    # Now apply mirrored poses
    print("  Pass 2: Applying mirrored poses...")

    for frame in range(frame_start, frame_end + 1):
        scene.frame_set(frame)

        # Restore the original pose for this frame
        for pbone in armature.pose.bones:
            if pbone.name in stored_poses[frame]:
                data = stored_poses[frame][pbone.name]
                pbone.location = data['location']
                pbone.rotation_quaternion = data['rotation_quaternion']
                pbone.rotation_euler = data['rotation_euler']
                pbone.scale = data['scale']

        # Copy and paste with flip (suppress operator output)
        with suppress_output():
            bpy.ops.pose.copy()
            bpy.ops.pose.paste(flipped=True)

        # Insert keyframes for all bones
        for pbone in armature.pose.bones:
            pbone.keyframe_insert(data_path="location", frame=frame)
            pbone.keyframe_insert(data_path="rotation_quaternion", frame=frame)
            pbone.keyframe_insert(data_path="rotation_euler", frame=frame)
            pbone.keyframe_insert(data_path="scale", frame=frame)

        if (frame - frame_start) % 500 == 0:
            print(f"    Mirrored frame {frame - frame_start + 1}/{total_frames}")

    # Return to object mode
    bpy.ops.object.mode_set(mode='OBJECT')
    print("  Mirroring complete.")


def export_fbx(armature, output_path):
    """Export the armature to FBX."""
    # Select armature for export
    bpy.ops.object.select_all(action='DESELECT')
    armature.select_set(True)
    bpy.context.view_layer.objects.active = armature

    bpy.ops.export_scene.fbx(
        filepath=str(output_path),
        use_selection=True,
        bake_anim=True,
        add_leaf_bones=False
    )


def convert_and_mirror(input_path, output_path):
    """Import, mirror, and export a single animation file."""
    clear_scene()

    # Import
    armature = import_animation(input_path)
    print(f"  Imported armature with {len(armature.pose.bones)} bones")

    # Mirror using Blender's native functionality
    mirror_animation_native(armature)

    # Export
    export_fbx(armature, output_path)


def main():
    # Get arguments after "--"
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        print("Error: Please provide input path and output directory after '--'")
        print("Usage: blender --background --python fbx_mirror_batch.py -- <input_path> <output_dir> [--suffix SUFFIX]")
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
        if input_path.suffix.lower() not in ['.fbx', '.bvh']:
            print(f"Error: Input file must be .fbx or .bvh: {input_path}")
            sys.exit(1)
        anim_files = [input_path]
        print(f"Mirroring single file: {input_path.name}")
    else:
        # Find both FBX and BVH files
        anim_files = list(input_path.glob("*.fbx")) + list(input_path.glob("*.bvh"))
        print(f"Found {len(anim_files)} animation files in {input_path}")

    print(f"Output suffix: '{suffix}'")

    success_count = 0
    error_count = 0

    for i, anim_file in enumerate(anim_files, 1):
        fbx_file = output_dir / (anim_file.stem + suffix + ".fbx")
        print(f"[{i}/{len(anim_files)}] Mirroring: {anim_file.name} -> {fbx_file.name}")
        try:
            convert_and_mirror(anim_file, fbx_file)
            success_count += 1
        except Exception as e:
            print(f"  Error: {e}")
            import traceback
            traceback.print_exc()
            error_count += 1

    print(f"\nDone! Mirrored {success_count} files to {output_dir}")
    if error_count > 0:
        print(f"Errors: {error_count} files failed")


if __name__ == "__main__":
    main()
