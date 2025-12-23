import math

FIXED_SHIFT = 8
FIXED_ONE = 1 << FIXED_SHIFT

ANGLE_FULL = 512
ANGLE_QUARTER = 128

lut = []

for i in range(ANGLE_QUARTER + 1):
    # Map i ∈ [0..128] → angle ∈ [0..π/2]
    rad = (i / ANGLE_FULL) * 2.0 * math.pi
    val = int(round(math.sin(rad) * FIXED_ONE))

    # Clamp just in case of rounding edge cases
    val = max(0, min(FIXED_ONE, val))

    lut.append(val)

# Sanity checks
assert lut[0] == 0
assert lut[-1] == FIXED_ONE
assert all(0 <= v <= FIXED_ONE for v in lut)

# Emit C array
print("static const int16_t sin_lut[129] = {")
for i in range(0, len(lut), 8):
    chunk = ", ".join(f"{v:3d}" for v in lut[i : i + 8])
    print(f"    {chunk},")
print("};")
