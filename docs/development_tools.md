# Development Tools

This document describes the custom development tools built for Kart Mania to streamline asset conversion, debugging, and multiplayer testing.

---

## Overview

The `tools/` directory contains Python scripts that solve platform-specific issues and automate common development tasks:

```
tools/
├── audio/              # Audio format conversion
├── img/                # Image processing and debugging
├── network/            # Multiplayer testing utilities
└── other/              # Math table generation
```

---

## Audio Tools

### `tools/audio/mp3_to_wav.py`

Converts MP3 audio files to WAV format with Nintendo DS-compatible settings.

**Purpose**: The maxmod audio library requires WAV files with specific properties. This tool automates the conversion from MP3 source files to the correct format.

**Usage**:
```bash
cd tools/audio
python mp3_to_wav.py <input.mp3>
```

**Output**: Creates `<input>.wav` in the same directory with the following settings:
- Sample rate: 22050 Hz (DS audio standard)
- Bit depth: 16-bit
- Channels: Mono (1 channel)

**Example**:
```bash
python mp3_to_wav.py CLICK.mp3
# Output: CLICK.wav (22050 Hz, 16-bit mono)
```

**Dependencies**: `pydub` library
```bash
pip install pydub
```

**Note**: You may also need FFmpeg installed for MP3 decoding:
```bash
# macOS
brew install ffmpeg

# Linux
sudo apt-get install ffmpeg
```

---

## Image Tools

### `tools/img/swap_rb_grit_fix.py`

Fixes RGB color channel swapping and ensures proper transparency for grit asset conversion on macOS.

**The Problem**: On macOS, the `grit` tool (used to convert PNG files to Nintendo DS tile/sprite format) has a bug that swaps the red and blue color channels. Additionally, partially transparent pixels can cause rendering artifacts.

**The Solution**: This tool:
1. **Swaps R and B channels** - Pre-swaps red/blue so grit's swap results in correct colors
2. **Forces exact transparency** - Sets all pixels with alpha < 1 to the exact key color (magenta: #FF00FF)
3. **Optional morphology operations** - Can erode/dilate sprite masks to remove edge artifacts

**Usage**:
```bash
cd tools/img
python swap_rb_grit_fix.py <input.png>
```

**Output**: Creates `<input>_grit.png` with corrected RGB channels and proper transparency.

**Example**:
```bash
python swap_rb_grit_fix.py kart_sprite.png
# Output: kart_sprite_grit.png (ready for grit conversion)
```

**Parameters** (edit script to adjust):
- `KEY_RGB = (255, 0, 255)` - Transparency key color (magenta)
- `KEEP_ALPHA = 1` - Minimum alpha value to consider "opaque" (1 = any visible pixel)
- `ERODE = 0` - Pixels to erode from sprite edges (removes fringes)
- `DILATE = 0` - Pixels to expand sprite edges (prevents cutting)

**Workflow**:
```bash
# 1. Create sprite in editor (e.g., kart.png)
# 2. Run swap_rb_grit_fix.py
python swap_rb_grit_fix.py kart.png
# Output: kart_grit.png

# 3. Convert with grit (build system does this automatically)
grit kart_grit.png -fts -o kart

# 4. Result: kart.h, kart.s with correct colors
```

**Dependencies**: `Pillow` (PIL)
```bash
pip install Pillow
```

---

### `tools/img/pick_pixel_xy.py`

Interactive tool to get pixel coordinates and RGBA values from PNG images.

**Purpose**: Used during development to determine exact coordinates for:
- Track boundaries (wall collision detection)
- Item box placement positions
- UI element positioning


**Usage**:
```bash
cd tools/img
python pick_pixel_xy.py <image.png>
```

**How it works**:
1. Opens a matplotlib window displaying the image
2. Click any pixel to print its `(x, y)` coordinates and RGBA color
3. Coordinate system: `(0, 0)` is **top-left** (matches DS screen coordinates)
4. Close the window to quit

**Example**:
```bash
python pick_pixel_xy.py ../../data/tracks/scorching_sands_map.png
# Click on item box locations to get coordinates
# Output: (x, y) = (245, 678)   RGBA=(255,200,50,255)
```

**Use Cases**:

**Item Box Placement**:
```c
// Click item box locations on track map
static const Vec2 itemBoxSpawns[] = {
    {245, 678},  // From pick_pixel_xy.py
    {512, 423},
    {789, 156}
};
```

**Boundary Detection**:
```c
// Click track boundary pixels to find borders
if (pixel_y < 100) {  // Top boundary at y=100
    // Apply collision response
}
```

**Dependencies**: `Pillow`, `matplotlib`
```bash
pip install Pillow matplotlib
```

---

## Network Tools

### `tools/network/simulate_ds_player.py`

Simulates a Nintendo DS player for multiplayer development and testing.

**Purpose**: Testing multiplayer features requires multiple DS consoles. This tool emulates a DS player over the network, allowing solo testing of:
- Lobby connection/disconnection
- Ready status synchronization
- Car position updates
- Item placement
- Packet loss handling

**The Protocol**: The simulator implements the exact same UDP protocol as the DS:
- **Port**: 8888
- **Messages**: LOBBY_JOIN, LOBBY_UPDATE, READY, CAR_UPDATE, ITEM_PLACED, DISCONNECT
- **Format**: 4-byte header (version, msgType, playerID, seqNum) + 28-byte payload
- **Fixed-point**: Position/speed encoded as Q16.8 format (matches DS)

**Usage**:

**Basic Lobby Testing**:
```bash
cd tools/network
python simulate_ds_player.py --player-id 1
# Simulates Player 2 (ID 1) joining the lobby
```

**Target Specific DS**:
```bash
python simulate_ds_player.py --player-id 5 --target 192.168.1.100
# Simulates Player 6, sends packets to DS at 192.168.1.100
```

**Broadcast to All Devices**:
```bash
python simulate_ds_player.py --player-id 3 --target 255.255.255.255
# Broadcasts to all devices on local network
```

**Start in Race Mode**:
```bash
python simulate_ds_player.py --player-id 2 --race
# Skip lobby, go directly to racing
```

**Interactive Controls**:

**Lobby Mode**:
- `[space]` or `[enter]` - Toggle READY status
- `[r]` - Enter race mode
- `[q]` - Quit and disconnect

**Race Mode**:
- `[b]` - Drop banana
- `[g]` - Throw green shell
- `[l]` - Return to lobby
- `[q]` - Quit and disconnect

**Example Session**:
```bash
$ python simulate_ds_player.py --player-id 1

╔════════════════════════════════════════════════════════════╗
║          Kart Mania - DS Player Simulator                 ║
╚════════════════════════════════════════════════════════════╝

Player ID: 1 (Player 2)
Target IP: 192.168.1.255
Port: 8888

📡 Detected network: 192.168.1.50 -> broadcast: 192.168.1.255

🎮 Player 2 joining lobby...
📡 Sent LOBBY_JOIN (ready=False)

============================================================
LOBBY MODE - Commands:
  [space] or [enter] - Toggle READY status
  [r] - Start RACE mode (simulate racing)
  [q] - Quit
============================================================

Status: NOT READY ⏳ | Sent: 8 | Received: 2

[User presses space]
✅ Sent READY (ready=True)

Status: READY ✅ | Sent: 15 | Received: 7

[User presses 'r']
🏁 Starting RACE mode!

============================================================
RACE MODE - Commands:
  [b] - Drop BANANA
  [g] - Throw GREEN SHELL
  [l] - Return to LOBBY
  [q] - Quit
============================================================

[User presses 'b']
🎯 Sent ITEM_PLACED (ITEM_BANANA)

[User presses 'q']
👋 Sent DISCONNECT
📊 Final Stats:
   Packets Sent: 247
   Packets Received: 183
👋 Goodbye!
```

**Features**:
- **Automatic heartbeats** - Sends LOBBY_UPDATE every 1 second
- **Car physics simulation** - Moves in a circle during race mode (15 Hz updates)
- **Packet reception** - Displays important messages from other players
- **Statistics tracking** - Shows packets sent/received
- **Graceful shutdown** - Sends DISCONNECT on exit

**Dependencies**: Python 3.6+ (standard library only)

**Testing Workflows**:

**1. Test lobby with 4 simulated players**:
```bash
# Terminal 1
python simulate_ds_player.py --player-id 0

# Terminal 2
python simulate_ds_player.py --player-id 1

# Terminal 3
python simulate_ds_player.py --player-id 2

# Terminal 4
python simulate_ds_player.py --player-id 3

# All players press [space] to ready up
# DS lobby should show all 5 players ready
```

**2. Test item synchronization**:
```bash
# Start race mode
python simulate_ds_player.py --player-id 1 --race

# Press 'b' to drop bananas
# DS should see banana appear on track
# Press 'g' to throw shells
# DS should see shell projectile
```

**3. Test disconnection handling**:
```bash
# Join lobby, ready up, then press 'q'
# DS should detect disconnect and update lobby
```

---

## Math Tools

### `tools/other/gen_sin_lut_q16_8.py`

Generates a lookup table (LUT) for sine values in Q16.8 fixed-point format.

**Purpose**: The Nintendo DS ARM9 processor has no hardware floating-point unit (FPU). Calculating trigonometric functions (sin, cos) at runtime using software floating-point is extremely slow. A lookup table provides instant access to precomputed values.

**The Math**:
- **Angle System**: 512 units = 360° (used throughout Kart Mania)
- **Quarter Circle**: Store only 0° to 90° (129 values), use symmetry for the rest
- **Fixed-Point Format**: Q16.8 = 16 bits integer, 8 bits fraction
  - Range: -32768.0 to +32767.996
  - Precision: 1/256 ≈ 0.0039
  - Example: 1.0 = 256, 0.5 = 128

**Usage**:
```bash
cd tools/other
python gen_sin_lut_q16_8.py > sin_lut.c
```

**Output** (C code):
```c
static const int16_t sin_lut[129] = {
      0,   3,   6,  10,  13,  16,  19,  22,
     25,  28,  31,  35,  38,  41,  44,  47,
     50,  53,  56,  59,  62,  65,  68,  71,
    ...
    241, 244, 247, 250, 252, 253, 254, 256,
};
```

**Integration in Code**:

The generated LUT is used in `source/math/fixedmath.c`:

```c
#include "fixedmath.h"

// Generated by gen_sin_lut_q16_8.py
static const int16_t sin_lut[129] = {
    0,   3,   6,  10,  13,  16, ...
};

int fixedSin512(int angle512) {
    // Normalize angle to [0, 512)
    angle512 = ((angle512 % 512) + 512) % 512;

    int quadrant = angle512 / 128;
    int index = angle512 % 128;

    int result;
    switch (quadrant) {
        case 0: result =  sin_lut[index]; break;           // 0-90°
        case 1: result =  sin_lut[128 - index]; break;     // 90-180°
        case 2: result = -sin_lut[index]; break;           // 180-270°
        case 3: result = -sin_lut[128 - index]; break;     // 270-360°
    }

    return result << (FIXED_SHIFT - 8);  // Convert Q16.8 to Q16.16
}

int fixedCos512(int angle512) {
    return fixedSin512(angle512 + 128);  // cos(x) = sin(x + 90°)
}
```

**Performance**:
- **Without LUT**: ~5000 cycles per sin() call (software floating-point)
- **With LUT**: ~15 cycles per lookup (array access + symmetry logic)
- **Speedup**: ~333x faster!

**Use Cases in Kart Mania**:
- Car rotation calculations (steering physics)
- Item trajectory computations (shells, missiles)
- Camera scrolling (smooth following)
- Sprite rotation angles

**Dependencies**: Python 3.6+ (standard library only)

---

## Development Workflow

### Setting Up Tools

```bash
# Create Python virtual environment
python3 -m venv .venv
source .venv/bin/activate  # On Windows: .venv\Scripts\activate

# Install dependencies
pip install pydub Pillow matplotlib

# Verify tools work
cd tools/audio
python mp3_to_wav.py --help

cd ../img
python pick_pixel_xy.py --help

cd ../network
python simulate_ds_player.py --help
```

### Asset Pipeline

**Audio Workflow**:
```bash
# 1. Create sound effect as MP3
# 2. Convert to WAV
cd tools/audio
python mp3_to_wav.py ../../audio/BOX.mp3

# 3. Copy to audio directory
cp outputs/BOX.wav ../../audio/

# 4. Build includes in soundbank (automatic via Makefile)
make
```

**Graphics Workflow**:
```bash
# 1. Create sprite PNG with transparency
# 2. Fix grit color swap
cd tools/img
python swap_rb_grit_fix.py ../../data/sprites/kart.png

# 3. Use _grit.png version in data/
cp kart_grit.png ../../data/sprites/kart.png

# 4. Build converts with grit (automatic via Makefile)
make
```

**Multiplayer Testing Workflow**:
```bash
# 1. Start DS with kart-mania.nds
# 2. DS enters multiplayer lobby
# 3. Simulate additional players
cd tools/network
python simulate_ds_player.py --player-id 1 &
python simulate_ds_player.py --player-id 2 &

# 4. All players ready up (press space in terminals)
# 5. Race starts with 3 players (1 real DS, 2 simulated)
```

---

## Tool Maintenance

### Adding New Audio Formats

Edit `mp3_to_wav.py` to support other input formats:

```python
# Add support for OGG
audio = AudioSegment.from_ogg(input_file)
```

### Adjusting Image Processing

Edit `swap_rb_grit_fix.py` constants:

```python
KEEP_ALPHA = 128  # Only keep pixels with alpha >= 128
ERODE = 1         # Remove 1-pixel fringes
DILATE = 1        # Expand sprite by 1 pixel
```

### Extending Network Simulator

Add new message types to `simulate_ds_player.py`:

```python
class MessageType(IntEnum):
    MSG_LOBBY_JOIN = 0
    # ... existing types ...
    MSG_LAP_COMPLETE = 7  # Add new message

def send_lap_complete(self):
    """Send MSG_LAP_COMPLETE"""
    payload = struct.pack('<I24x', self.lap)
    packet = self.build_packet(MessageType.MSG_LAP_COMPLETE, payload)
    self.sock.sendto(packet, (self.target_ip, self.port))
```

---

## Troubleshooting

### Audio Tool Issues

**"pydub not found"**:
```bash
pip install pydub
```

**"ffmpeg not found"**:
```bash
# macOS
brew install ffmpeg

# Linux
sudo apt-get install ffmpeg
```

**"Output WAV has wrong sample rate"**:
- Check `sample_rate` parameter in `mp3_to_wav()`
- Verify with: `ffprobe output.wav`

---

### Image Tool Issues

**"Colors still wrong after swap_rb_grit_fix"**:
- Run script again on original PNG (not _grit.png)
- Verify input has correct colors before processing
- Check KEY_RGB matches your transparency color

**"Sprite edges have artifacts"**:
- Try `ERODE = 1` to remove fringes
- Try `DILATE = 1` to prevent cutting

**"pick_pixel_xy won't open image"**:
```bash
pip install matplotlib Pillow
```

**"Coordinates are flipped"**:
- Tool uses `(0,0) = top-left` (correct for DS)
- Some image editors use `(0,0) = bottom-left`
- Y coordinate increases downward on DS

---

### Network Tool Issues

**"Could not bind to port 8888"**:
- Another instance is already running
- Kill existing instances: `pkill -f simulate_ds_player`
- Or use different port: `--port 8889`

**"No packets received"**:
- Check firewall settings
- Verify DS and computer on same network
- Try broadcast: `--target 255.255.255.255`

**"Keyboard input not working"**:
- Unix/Linux/Mac: Terminal must support raw mode
- Windows: Limited keyboard support, use Ctrl+C to quit

**"DS doesn't see simulated player"**:
- Verify IP address: `--target <DS_IP>`
- Check DS WiFi is enabled in settings
- Confirm port 8888 is correct
- Try broadcast mode first

---

## References

- **pydub docs**: https://github.com/jiaaro/pydub
- **Pillow docs**: https://pillow.readthedocs.io/
- **grit docs**: https://www.coranac.com/man/grit/html/grit.htm
- **Fixed-Point Math**: https://en.wikipedia.org/wiki/Fixed-point_arithmetic
- **DS Networking**: https://www.devkitpro.org/wiki/Networking

---

## Authors

**Bahey Shalash, Hugo Svolgaard**

**Version**: 1.0
**Last Updated**: 06.01.2026

---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)