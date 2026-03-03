# RootUpdateY vs delta_angle — Root Rotation Misalignment Notes

## Summary

When comparing Unity GNN and AnimHost GNN root rotation outputs side-by-side, the
rotation traces have the **same shape and direction** but differ in magnitude by ~8–9×
even after correct unit conversion (radians → degrees). This is **not a bug** — it is a
structural consequence of the two models being trained with different root-frame definitions.

---

## Root Frame Definitions

| Model | How the root frame is defined |
|-------|------------------------------|
| **Unity** | Root bone: an explicit skeletal joint placed by the rig. Its orientation is set directly by the animator/physics pipeline and updated each frame via `Quaternion.AngleAxis(RootUpdateY, up)`. |
| **AnimHost** | Limb-geometry synthetic root: derived at training time from the midpoint and orientation of `(front_limb_mid → rear_limb_mid)`. Implemented in `LocomotionPreprocessNode.cpp → prepareBipedRoot()`. |

### AnimHost synthetic root (simplified)

```
front_mid = (leftFront + rightFront) / 2
rear_mid  = (leftRear  + rightRear)  / 2
root_forward = normalize(front_mid - rear_mid)
root_pos     = (front_mid + rear_mid) / 2
```

`delta_angle` is then:
```
delta_angle = glm::orientedAngle({0,1}, ForwardTo(nextRoot, currentRoot))  // radians
```

---

## Why the Magnitudes Differ (~8–9×)

The Unity root bone rotates with **both**:
1. The large-scale directional turn signal
2. High-frequency gait-cycle oscillation (each stride rocks the root)

The AnimHost synthetic root averages four limb positions, so gait-cycle oscillations
**cancel out** at the averaging step. Only the slow directional turn signal survives.

Result: `delta_angle` is a much smoother, lower-amplitude signal than `RootUpdateY`,
even though both encode "how much did the character's heading change this frame."

---

## Unit Conversion Reference

| Feature | Unit at model output | Scale to apply for comparison |
|---------|---------------------|-------------------------------|
| `RootUpdateY` (Unity) | degrees | × 1.0 |
| `delta_angle` (AnimHost) | radians | × (180 / π) ≈ × 57.3 |
| `RootUpdateX/Z` (Unity) | normalised (~0.01 m) | × 1.0 |
| `delta_x`, `delta_y` (AnimHost) | SI metres | × 100 to match Unity scale |

---

## The Mismatch Is on Both Input and Output Sides

In `QuadrupedController_GNN_Unified.cs → Feed()`, all bone positions and trajectory
features are expressed relative to `Actor.GetRoot().GetWorldMatrix()` — the **skeletal
root bone**. The AnimHost model was trained with features expressed relative to its
**limb-geometry root**. So:

- **Input features**: bone positions/directions in Unity are in a different local frame
  than what AnimHost was trained on.
- **Output `delta_angle`**: encodes heading change in the limb-geometry frame, which
  the Unity controller then applies as a skeletal root rotation update.

Both mismatches are symmetric and inherent to cross-framework inference.

---

## Current Unity Controller Handling (AnimHost branch)

```csharp
// QuadrupedController_GNN_Unified.cs ~line 189
float dx     = NeuralNetwork.Read() / 100f;           // metres → Unity units
float dz     = NeuralNetwork.Read() / 100f;
float dAngle = NeuralNetwork.Read();                   // radians
Vector3 offset = Vector3.Lerp(
    Vector3.zero,
    new Vector3(dx, dAngle * Mathf.Rad2Deg, dz),      // rad → deg
    update);                                            // velocity dampening
root = Actor.GetRoot().GetWorldMatrix() * Matrix4x4.TRS(
    new Vector3(offset.x, 0f, offset.z),
    Quaternion.AngleAxis(offset.y, Vector3.up),
    Vector3.one);
```

This is the correct practical approximation given the constraints.

---

## What Would Be Needed for Full Parity

To eliminate the root-frame mismatch entirely, Unity would need to:

1. Re-implement `prepareBipedRoot()` in C# to compute the synthetic limb-geometry root
   at runtime from the four end-effector positions.
2. Express all input features relative to that synthetic root (not the skeletal root bone).
3. Apply `delta_angle` as an update to the synthetic root, then copy its orientation back
   to the skeletal root.

This is out of scope for cross-framework inference parity testing. Accept the ~8–9×
rotation magnitude difference as structural, not a calibration error.

---

## Visualization

Use `analyze_gnn_data.py` with `MODE = "root_output"` to overlay the matched pairs:

```python
ROOT_OUTPUT_PAIRS = [
    ("RootUpdateX", "delta_x",     "X translation delta  (m)",    100.0,           1.0),
    ("RootUpdateY", "delta_angle", "Y rotation delta  (deg)",       1.0, 180.0/math.pi),
    ("RootUpdateZ", "delta_y",     "Z translation delta  (m)",    100.0,           1.0),
]
```

Expected: translation panels match closely; rotation panel shows same shape/direction
but AnimHost amplitude is ~8–9× smaller — confirming this note.
