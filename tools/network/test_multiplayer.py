#!/usr/bin/env python3
"""
UDP Packet Tester for Kart Mania Multiplayer
Simulates network packets for testing the Nintendo DS multiplayer functionality
"""

import socket
import struct
import time
import argparse
from enum import IntEnum

# Protocol Constants
PROTOCOL_VERSION = 1
GAME_PORT = 8888  # UDP port from WiFi_minilib.c (LOCAL_PORT/OUT_PORT)
MAX_MULTIPLAYER_PLAYERS = 8

class MessageType(IntEnum):
    MSG_LOBBY_JOIN = 0       # "I'm joining the lobby"
    MSG_LOBBY_UPDATE = 1     # "I'm still here" (heartbeat)
    MSG_READY = 2            # "I pressed SELECT"
    MSG_CAR_UPDATE = 3       # "Here's my car state" (during race)
    MSG_ITEM_PLACED = 4      # "I placed/threw an item on the track"
    MSG_ITEM_BOX_PICKUP = 5  # "I picked up an item box"
    MSG_DISCONNECT = 6       # "I'm leaving"

class Item(IntEnum):
    ITEM_NONE = 0
    ITEM_BANANA = 1
    ITEM_GREEN_SHELL = 2
    ITEM_RED_SHELL = 3
    ITEM_BOOST = 4

class NetworkPacketBuilder:
    """
    Builds properly formatted network packets for the Kart Mania protocol
    Packet format (32 bytes total):
    - version (1 byte)
    - msgType (1 byte)
    - playerID (1 byte)
    - padding (1 byte)
    - sequenceNumber (4 bytes)
    - payload (24 bytes)
    """

    def __init__(self, player_id=0):
        self.player_id = player_id
        self.sequence_number = 0

    def build_lobby_join(self):
        """Build MSG_LOBBY_JOIN packet"""
        # Header: version, msgType, playerID, padding, sequenceNumber
        header = struct.pack('<BBBBI', PROTOCOL_VERSION, MessageType.MSG_LOBBY_JOIN,
                           self.player_id, 0, self.sequence_number)
        self.sequence_number += 1

        # Payload: isReady (bool) + 23 bytes reserved
        payload = struct.pack('<B23x', 0)  # isReady = False

        return header + payload

    def build_lobby_update(self, is_ready=False):
        """Build MSG_LOBBY_UPDATE packet (heartbeat)"""
        header = struct.pack('<BBBBI', PROTOCOL_VERSION, MessageType.MSG_LOBBY_UPDATE,
                           self.player_id, 0, self.sequence_number)
        self.sequence_number += 1

        payload = struct.pack('<B23x', int(is_ready))

        return header + payload

    def build_ready(self, is_ready=True):
        """Build MSG_READY packet"""
        header = struct.pack('<BBBBI', PROTOCOL_VERSION, MessageType.MSG_READY,
                           self.player_id, 0, self.sequence_number)
        self.sequence_number += 1

        payload = struct.pack('<B23x', int(is_ready))

        return header + payload

    def build_car_update(self, pos_x=0, pos_y=0, speed=0, angle512=0, lap=0, item=Item.ITEM_NONE):
        """
        Build MSG_CAR_UPDATE packet
        Position: Vec2 (8 bytes: 2 x int32_t in Q16.8 fixed-point)
        Speed: Q16_8 (4 bytes: int32_t in Q16.8 fixed-point)
        Angle: int (4 bytes)
        Lap: int (4 bytes)
        Item: Item enum (4 bytes)
        """
        header = struct.pack('<BBBBI', PROTOCOL_VERSION, MessageType.MSG_CAR_UPDATE,
                           self.player_id, 0, self.sequence_number)
        self.sequence_number += 1

        # Convert to Q16.8 fixed-point (multiply by 256)
        pos_x_q16_8 = int(pos_x * 256)
        pos_y_q16_8 = int(pos_y * 256)
        speed_q16_8 = int(speed * 256)

        # Payload: position (8), speed (4), angle (4), lap (4), item (4)
        payload = struct.pack('<iiiiii', pos_x_q16_8, pos_y_q16_8, speed_q16_8,
                            angle512, lap, item)

        return header + payload

    def build_item_placed(self, item_type, pos_x=0, pos_y=0, angle512=0, speed=0):
        """Build MSG_ITEM_PLACED packet"""
        header = struct.pack('<BBBBI', PROTOCOL_VERSION, MessageType.MSG_ITEM_PLACED,
                           self.player_id, 0, self.sequence_number)
        self.sequence_number += 1

        # Convert to Q16.8 fixed-point
        pos_x_q16_8 = int(pos_x * 256)
        pos_y_q16_8 = int(pos_y * 256)
        speed_q16_8 = int(speed * 256)

        # Payload: itemType (4), position (8), angle512 (4), speed (4), reserved (4)
        payload = struct.pack('<iiiii4x', item_type, pos_x_q16_8, pos_y_q16_8,
                            angle512, speed_q16_8)

        return header + payload

    def build_item_box_pickup(self, box_index):
        """Build MSG_ITEM_BOX_PICKUP packet"""
        header = struct.pack('<BBBBI', PROTOCOL_VERSION, MessageType.MSG_ITEM_BOX_PICKUP,
                           self.player_id, 0, self.sequence_number)
        self.sequence_number += 1

        # Payload: boxIndex (4) + 20 bytes reserved
        payload = struct.pack('<i20x', box_index)

        return header + payload

    def build_disconnect(self):
        """Build MSG_DISCONNECT packet"""
        header = struct.pack('<BBBBI', PROTOCOL_VERSION, MessageType.MSG_DISCONNECT,
                           self.player_id, 0, self.sequence_number)
        self.sequence_number += 1

        # Empty payload
        payload = b'\x00' * 24

        return header + payload


def send_packet(sock, packet, target_ip, port):
    """Send a packet to the target"""
    sock.sendto(packet, (target_ip, port))
    print(f"  → Sent {len(packet)} bytes to {target_ip}:{port}")


def test_lobby_sequence(target_ip, port, player_id):
    """Test the lobby join sequence"""
    print(f"\n=== Testing Lobby Sequence (Player {player_id}) ===")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    builder = NetworkPacketBuilder(player_id)

    # 1. Send LOBBY_JOIN
    print("\n1. Sending LOBBY_JOIN...")
    packet = builder.build_lobby_join()
    send_packet(sock, packet, target_ip, port)
    time.sleep(0.5)

    # 2. Send periodic LOBBY_UPDATE (heartbeat)
    print("\n2. Sending LOBBY_UPDATE heartbeats (5 times)...")
    for i in range(5):
        packet = builder.build_lobby_update(is_ready=False)
        send_packet(sock, packet, target_ip, port)
        time.sleep(1)

    # 3. Send READY
    print("\n3. Sending READY...")
    packet = builder.build_ready(is_ready=True)
    send_packet(sock, packet, target_ip, port)
    time.sleep(0.5)

    # 4. Send DISCONNECT
    print("\n4. Sending DISCONNECT...")
    packet = builder.build_disconnect()
    send_packet(sock, packet, target_ip, port)

    sock.close()
    print("\n✓ Lobby sequence test complete")


def test_race_sequence(target_ip, port, player_id):
    """Test the race update sequence"""
    print(f"\n=== Testing Race Sequence (Player {player_id}) ===")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    builder = NetworkPacketBuilder(player_id)

    print("\nSimulating car movement (10 updates)...")
    for i in range(10):
        # Simulate movement in a circle
        angle = i * 51  # angle512 units (512 = full circle)
        pos_x = 100.0 + 50.0 * (i / 10.0)
        pos_y = 100.0 + 50.0 * (i / 10.0)
        speed = 5.0 + (i * 0.5)

        packet = builder.build_car_update(
            pos_x=pos_x,
            pos_y=pos_y,
            speed=speed,
            angle512=angle,
            lap=1,
            item=Item.ITEM_NONE
        )
        send_packet(sock, packet, target_ip, port)
        print(f"    Car pos: ({pos_x:.1f}, {pos_y:.1f}), speed: {speed:.1f}, angle: {angle}")
        time.sleep(0.1)  # 10 Hz update rate

    sock.close()
    print("\n✓ Race sequence test complete")


def test_item_usage(target_ip, port, player_id):
    """Test item placement and pickup"""
    print(f"\n=== Testing Item Usage (Player {player_id}) ===")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    builder = NetworkPacketBuilder(player_id)

    # 1. Send item box pickup
    print("\n1. Picking up item box #3...")
    packet = builder.build_item_box_pickup(box_index=3)
    send_packet(sock, packet, target_ip, port)
    time.sleep(0.5)

    # 2. Place a banana
    print("\n2. Placing a banana...")
    packet = builder.build_item_placed(
        item_type=Item.ITEM_BANANA,
        pos_x=150.0,
        pos_y=200.0,
        angle512=128,
        speed=0.0
    )
    send_packet(sock, packet, target_ip, port)
    time.sleep(0.5)

    # 3. Throw a green shell
    print("\n3. Throwing a green shell...")
    packet = builder.build_item_placed(
        item_type=Item.ITEM_GREEN_SHELL,
        pos_x=150.0,
        pos_y=200.0,
        angle512=256,
        speed=10.0
    )
    send_packet(sock, packet, target_ip, port)

    sock.close()
    print("\n✓ Item usage test complete")


def listen_for_packets(port, duration=10):
    """Listen for incoming packets (debugging)"""
    print(f"\n=== Listening for packets on port {port} for {duration}s ===")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', port))
    sock.settimeout(1.0)

    start_time = time.time()
    packet_count = 0

    while time.time() - start_time < duration:
        try:
            data, addr = sock.recvfrom(1024)
            packet_count += 1

            if len(data) >= 8:
                # Parse packet header
                version, msg_type, player_id, padding, seq_num = struct.unpack('<BBBBI', data[:8])

                msg_type_name = MessageType(msg_type).name if msg_type < 7 else f"UNKNOWN({msg_type})"

                print(f"\n[Packet #{packet_count}] from {addr[0]}:{addr[1]}")
                print(f"  Version: {version}")
                print(f"  Type: {msg_type_name}")
                print(f"  Player ID: {player_id}")
                print(f"  Sequence: {seq_num}")
                print(f"  Size: {len(data)} bytes")

        except socket.timeout:
            continue
        except Exception as e:
            print(f"Error: {e}")

    sock.close()
    print(f"\n✓ Received {packet_count} packets")


def main():
    parser = argparse.ArgumentParser(
        description='Test UDP packets for Kart Mania multiplayer',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Test lobby sequence
  python test_multiplayer.py --target 192.168.1.100 --test lobby

  # Test race updates
  python test_multiplayer.py --target 192.168.1.100 --test race

  # Test item usage
  python test_multiplayer.py --target 192.168.1.100 --test items

  # Listen for incoming packets
  python test_multiplayer.py --listen --port 8888

  # Broadcast to all devices on network
  python test_multiplayer.py --target 192.168.1.255 --test lobby
        """
    )

    parser.add_argument('--target', type=str, default='192.168.1.255',
                       help='Target IP address (default: 192.168.1.255 for broadcast)')
    parser.add_argument('--port', type=int, default=8888,
                       help='UDP port (default: 8888)')
    parser.add_argument('--player-id', type=int, default=7,
                       help='Player ID to simulate (0-7, default: 7)')
    parser.add_argument('--test', choices=['lobby', 'race', 'items', 'all'],
                       help='Test sequence to run')
    parser.add_argument('--listen', action='store_true',
                       help='Listen for incoming packets instead of sending')
    parser.add_argument('--duration', type=int, default=10,
                       help='How long to listen for packets in seconds (default: 10)')

    args = parser.parse_args()

    if args.listen:
        listen_for_packets(args.port, args.duration)
    elif args.test:
        if args.test in ['lobby', 'all']:
            test_lobby_sequence(args.target, args.port, args.player_id)

        if args.test in ['race', 'all']:
            test_race_sequence(args.target, args.port, args.player_id)

        if args.test in ['items', 'all']:
            test_item_usage(args.target, args.port, args.player_id)
    else:
        parser.print_help()


if __name__ == '__main__':
    main()
