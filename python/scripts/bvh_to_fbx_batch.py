"""
Batch convert BVH files to FBX using Blender's Python API.

Usage:
    blender --background --python bvh_to_fbx_batch.py -- <input_path> <output_dir>

    input_path: Either a single .bvh file or a directory containing .bvh files

Example:
    blender --background --python bvh_to_fbx_batch.py -- "D:\anim-ws\MANN_qudruped_data" "D:\anim-ws\MANN_qudruped_data\fbx_output"
    & "C:\Program Files\Blender Foundation\Blender 4.2\blender.exe" --background --python bvh_to_fbx_batch.py -- "D:\anim-ws\MANN_qudruped_data" "D:\anim-ws\MANN_qudruped_data\fbx_output"
    & "C:\Program Files\Blender Foundation\Blender 4.2\blender.exe" --background --python bvh_to_fbx_batch.py -- "D:\anim-ws\MANN_qudruped_data\D1_053_KAN01_002.bvh" "D:\anim-ws\MANN_qudruped_data\fbx_output"
"""

import bpy
import sys
import os
from pathlib import Path


def convert_bvh_to_fbx(bvh_path, fbx_path):
    """Convert a single BVH file to FBX."""
    # Clear the scene
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()

    # Import BVH
    bpy.ops.import_anim.bvh(filepath=str(bvh_path))

    # Select all (the imported armature)
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
