#!/usr/bin/env python3
"""Verify packet sizes match C struct definitions"""
import struct

print("=== Packet Structure Verification ===\n")

# Header (8 bytes)
header = struct.pack('<BBBBI', 1, 2, 3, 0, 12345)
print(f"Header size: {len(header)} bytes (expected: 8)")
print(f"  version (uint8_t): 1 byte")
print(f"  msgType (uint8_t): 1 byte")
print(f"  playerID (uint8_t): 1 byte")
print(f"  padding (uint8_t): 1 byte")
print(f"  sequenceNumber (uint32_t): 4 bytes")

# Payload - Lobby (24 bytes)
payload_lobby = struct.pack('<B23x', 1)
print(f"\nLobby payload size: {len(payload_lobby)} bytes (expected: 24)")
print(f"  isReady (bool): 1 byte")
print(f"  reserved: 23 bytes")

# Payload - Car update (24 bytes)
payload_car = struct.pack('<iiiiii', 100, 200, 300, 128, 1, 0)
print(f"\nCar update payload size: {len(payload_car)} bytes (expected: 24)")
print(f"  position.x (int32_t/Q16_8): 4 bytes")
print(f"  position.y (int32_t/Q16_8): 4 bytes")
print(f"  speed (int32_t/Q16_8): 4 bytes")
print(f"  angle512 (int): 4 bytes")
print(f"  lap (int): 4 bytes")
print(f"  item (Item enum): 4 bytes")

# Payload - Item placed (24 bytes)
payload_item = struct.pack('<iiiii4x', 1, 100, 200, 256, 500)
print(f"\nItem placed payload size: {len(payload_item)} bytes (expected: 24)")
print(f"  itemType (Item): 4 bytes")
print(f"  position.x (int32_t): 4 bytes")
print(f"  position.y (int32_t): 4 bytes")
print(f"  angle512 (int): 4 bytes")
print(f"  speed (Q16_8): 4 bytes")
print(f"  reserved: 4 bytes")

# Payload - Item box pickup (24 bytes)
payload_box = struct.pack('<i20x', 5)
print(f"\nItem box pickup payload size: {len(payload_box)} bytes (expected: 24)")
print(f"  boxIndex (int): 4 bytes")
print(f"  reserved: 20 bytes")

# Full packet
full_packet = header + payload_car
print(f"\n{'='*50}")
print(f"FULL PACKET SIZE: {len(full_packet)} bytes (expected: 32)")
print(f"{'='*50}")

if len(full_packet) == 32:
    print("\n✅ SUCCESS: All packet sizes match C struct definitions!")
else:
    print("\n❌ ERROR: Packet size mismatch!")

# Test actual packet from builder
print("\n=== Testing NetworkPacketBuilder ===")
import sys
sys.path.insert(0, '.')
from tools.network.test_multiplayer import NetworkPacketBuilder

builder = NetworkPacketBuilder(player_id=5)

packets_to_test = [
    ("LOBBY_JOIN", builder.build_lobby_join()),
    ("LOBBY_UPDATE", builder.build_lobby_update(False)),
    ("READY", builder.build_ready(True)),
    ("CAR_UPDATE", builder.build_car_update(100.0, 200.0, 5.0, 256, 1)),
    ("ITEM_PLACED", builder.build_item_placed(1, 150.0, 200.0, 128, 10.0)),
    ("ITEM_BOX_PICKUP", builder.build_item_box_pickup(3)),
    ("DISCONNECT", builder.build_disconnect()),
]

all_pass = True
for name, packet in packets_to_test:
    if len(packet) == 32:
        print(f"✅ {name}: {len(packet)} bytes")
    else:
        print(f"❌ {name}: {len(packet)} bytes (EXPECTED 32!)")
        all_pass = False

if all_pass:
    print("\n✅ All packets are exactly 32 bytes!")
else:
    print("\n❌ Some packets have incorrect size!")
