"""
Verification script to check bpy.ops.export_scene.fbx parameters in your Blender version.
Run this in Blender's Python console or as a script to see all available parameters.

Usage in Blender:
    1. Open Blender
    2. Go to Scripting workspace
    3. Open this file
    4. Click "Run Script"

Or run from command line:
    blender --background --python verify_fbx_export_api.py
"""

import bpy
import inspect

def print_fbx_export_parameters():
    """Print all parameters available for bpy.ops.export_scene.fbx"""
    print("\n" + "="*80)
    print("bpy.ops.export_scene.fbx() Parameters")
    print("="*80)

    # Get the operator
    try:
        # Get the operator's properties
        op = bpy.ops.export_scene.fbx

        print("\nOperator:", op)
        print("\nDocumentation:")
        print(op.__doc__)

        # Try to get operator RNA info
        print("\n" + "-"*80)
        print("To see all parameters interactively, run in Blender's Python console:")
        print(">>> help(bpy.ops.export_scene.fbx)")
        print("-"*80)

        # Alternative: Check the operator's bl_rna
        try:
            # Access via the operator class
            from io_scene_fbx import export_fbx
            print("\nFBX Exporter module found!")
            print("Module:", export_fbx)
        except ImportError:
            print("\nNote: Direct module access not available")
            print("Use help(bpy.ops.export_scene.fbx) in Blender console for parameter details")

    except Exception as e:
        print(f"Error: {e}")
        print("\nThis script must be run inside Blender!")
        print("Try: blender --background --python verify_fbx_export_api.py")

    print("\n" + "="*80)
    print("Verification complete")
    print("="*80 + "\n")

if __name__ == "__main__":
    print_fbx_export_parameters()
