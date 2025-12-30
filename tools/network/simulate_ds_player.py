#!/usr/bin/env python3
"""
Simulates a Nintendo DS player for Kart Mania multiplayer testing
Acts like a real DS: joins lobby, sends heartbeats, can press SELECT, sends car updates
"""

import socket
import struct
import time
import argparse
import threading
import sys
import select
from enum import IntEnum

# Protocol Constants
PROTOCOL_VERSION = 1
GAME_PORT = 8888
MAX_MULTIPLAYER_PLAYERS = 8

class MessageType(IntEnum):
    MSG_LOBBY_JOIN = 0
    MSG_LOBBY_UPDATE = 1
    MSG_READY = 2
    MSG_CAR_UPDATE = 3
    MSG_ITEM_PLACED = 4
    MSG_ITEM_BOX_PICKUP = 5
    MSG_DISCONNECT = 6

class Item(IntEnum):
    ITEM_NONE = 0
    ITEM_BANANA = 1
    ITEM_GREEN_SHELL = 2
    ITEM_RED_SHELL = 3
    ITEM_BOOST = 4

def get_broadcast_address():
    """Auto-detect the broadcast address for the active network interface"""
    try:
        # Try to find our local IP by connecting to a remote address
        # (doesn't actually send data, just determines routing)
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        local_ip = s.getsockname()[0]
        s.close()

        # Calculate broadcast address based on /24 network
        # (assumes standard 255.255.255.0 subnet mask)
        ip_parts = local_ip.split('.')
        if len(ip_parts) == 4:
            broadcast = f"{ip_parts[0]}.{ip_parts[1]}.{ip_parts[2]}.255"
            print(f"📡 Detected network: {local_ip} -> broadcast: {broadcast}")
            return broadcast
        else:
            print(f"⚠️  Invalid IP format: {local_ip}")
            return '127.0.0.1'

    except Exception as e:
        print(f"⚠️  Network detection failed: {e}")
        print(f"⚠️  Using localhost - specify --target manually for network broadcast")
        return '127.0.0.1'

class SimulatedDSPlayer:
    """Simulates a Nintendo DS player"""

    def __init__(self, player_id, target_ip, port=GAME_PORT):
        self.player_id = player_id
        self.target_ip = target_ip
        self.port = port
        self.sequence_number = 0
        self.is_ready = False
        self.in_race = False
        self.running = True

        # Car state (for race mode)
        self.pos_x = 100.0 + (player_id * 20.0)  # Spread players out
        self.pos_y = 100.0 + (player_id * 20.0)
        self.speed = 0.0
        self.angle = 0
        self.lap = 1
        self.item = Item.ITEM_NONE

        # Setup socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        # Bind to receive packets
        try:
            self.sock.bind(('', port))
        except OSError as e:
            print(f"⚠️  Warning: Could not bind to port {port}: {e}")
            print("   (Receive functionality disabled, send-only mode)")

        self.sock.settimeout(0.1)  # Non-blocking with timeout

        # Statistics
        self.packets_sent = 0
        self.packets_received = 0
        self.last_heartbeat = time.time()

    def build_packet(self, msg_type, payload):
        """Build a network packet"""
        header = struct.pack('<BBBBI', PROTOCOL_VERSION, msg_type,
                           self.player_id, 0, self.sequence_number)
        self.sequence_number += 1
        return header + payload

    def send_lobby_join(self):
        """Send MSG_LOBBY_JOIN"""
        payload = struct.pack('<B23x', int(self.is_ready))
        packet = self.build_packet(MessageType.MSG_LOBBY_JOIN, payload)
        self.sock.sendto(packet, (self.target_ip, self.port))
        self.packets_sent += 1
        print(f"📡 Sent LOBBY_JOIN (ready={self.is_ready})")

    def send_lobby_update(self):
        """Send MSG_LOBBY_UPDATE (heartbeat)"""
        payload = struct.pack('<B23x', int(self.is_ready))
        packet = self.build_packet(MessageType.MSG_LOBBY_UPDATE, payload)
        try:
            self.sock.sendto(packet, (self.target_ip, self.port))
            self.packets_sent += 1
            self.last_heartbeat = time.time()
        except OSError as e:
            pass  # Silently ignore network errors during heartbeat

    def send_ready(self):
        """Send MSG_READY"""
        payload = struct.pack('<B23x', int(self.is_ready))
        packet = self.build_packet(MessageType.MSG_READY, payload)
        self.sock.sendto(packet, (self.target_ip, self.port))
        self.packets_sent += 1
        print(f"✅ Sent READY (ready={self.is_ready})")

    def send_car_update(self):
        """Send MSG_CAR_UPDATE"""
        # Convert to Q16.8 fixed-point
        pos_x_q16_8 = int(self.pos_x * 256)
        pos_y_q16_8 = int(self.pos_y * 256)
        speed_q16_8 = int(self.speed * 256)

        payload = struct.pack('<iiiiii', pos_x_q16_8, pos_y_q16_8, speed_q16_8,
                            self.angle, self.lap, self.item)
        packet = self.build_packet(MessageType.MSG_CAR_UPDATE, payload)
        self.sock.sendto(packet, (self.target_ip, self.port))
        self.packets_sent += 1

    def send_item_placed(self, item_type):
        """Send MSG_ITEM_PLACED"""
        pos_x_q16_8 = int(self.pos_x * 256)
        pos_y_q16_8 = int(self.pos_y * 256)
        speed_q16_8 = int(10.0 * 256)  # Item speed

        payload = struct.pack('<iiiii4x', item_type, pos_x_q16_8, pos_y_q16_8,
                            self.angle, speed_q16_8)
        packet = self.build_packet(MessageType.MSG_ITEM_PLACED, payload)
        self.sock.sendto(packet, (self.target_ip, self.port))
        self.packets_sent += 1
        print(f"🎯 Sent ITEM_PLACED ({Item(item_type).name})")

    def send_disconnect(self):
        """Send MSG_DISCONNECT"""
        payload = b'\x00' * 24
        packet = self.build_packet(MessageType.MSG_DISCONNECT, payload)
        try:
            self.sock.sendto(packet, (self.target_ip, self.port))
            self.packets_sent += 1
            print(f"👋 Sent DISCONNECT")
        except OSError:
            print(f"👋 Disconnecting (network unreachable)")

    def update_car_physics(self):
        """Simulate simple car movement"""
        # Move in a circle
        self.angle = (self.angle + 5) % 512
        self.speed = 5.0

        # Convert angle to radians (512 = 2π)
        import math
        angle_rad = (self.angle / 512.0) * 2 * math.pi

        # Update position
        self.pos_x += math.cos(angle_rad) * self.speed
        self.pos_y += math.sin(angle_rad) * self.speed

        # Keep in bounds (rough)
        self.pos_x = max(50, min(950, self.pos_x))
        self.pos_y = max(50, min(950, self.pos_y))

    def receive_packets(self):
        """Receive and display incoming packets"""
        try:
            data, addr = self.sock.recvfrom(1024)
            if len(data) >= 8:
                version, msg_type, player_id, padding, seq_num = struct.unpack('<BBBBI', data[:8])

                # Ignore our own packets
                if player_id == self.player_id:
                    return

                self.packets_received += 1

                msg_type_name = MessageType(msg_type).name if msg_type < 7 else f"UNKNOWN({msg_type})"

                # Only print important messages to reduce spam
                if msg_type in [MessageType.MSG_LOBBY_JOIN, MessageType.MSG_READY, MessageType.MSG_DISCONNECT]:
                    print(f"📥 Received {msg_type_name} from Player {player_id + 1}")
        except socket.timeout:
            pass
        except Exception as e:
            pass

    def run_lobby_mode(self):
        """Lobby mode: send heartbeats, wait for ready"""
        print(f"\n🎮 Player {self.player_id + 1} joining lobby...")
        self.send_lobby_join()

        print("\n" + "="*60)
        print("LOBBY MODE - Commands:")
        print("  [space] or [enter] - Toggle READY status")
        print("  [r] - Start RACE mode (simulate racing)")
        print("  [q] - Quit")
        print("="*60 + "\n")

        last_status_print = time.time()

        while self.running and not self.in_race:
            # Send heartbeat every 1 second
            if time.time() - self.last_heartbeat >= 1.0:
                self.send_lobby_update()

            # Receive packets
            self.receive_packets()

            # Print status every 5 seconds
            if time.time() - last_status_print >= 5.0:
                ready_text = "READY ✅" if self.is_ready else "NOT READY ⏳"
                print(f"Status: {ready_text} | Sent: {self.packets_sent} | Received: {self.packets_received}")
                last_status_print = time.time()

            # Check for keyboard input (non-blocking)
            if sys.platform != 'win32':
                # Unix-like systems
                if select.select([sys.stdin], [], [], 0)[0]:
                    key = sys.stdin.read(1)
                    self.handle_lobby_input(key)

            time.sleep(0.05)

    def handle_lobby_input(self, key):
        """Handle keyboard input in lobby mode"""
        if key in [' ', '\n', '\r']:
            # Toggle ready
            self.is_ready = not self.is_ready
            self.send_ready()
        elif key.lower() == 'r':
            # Start race
            print("\n🏁 Starting RACE mode!")
            self.in_race = True
        elif key.lower() == 'q':
            # Quit
            self.running = False

    def run_race_mode(self):
        """Race mode: send car updates, simulate movement"""
        print("\n" + "="*60)
        print("RACE MODE - Commands:")
        print("  [b] - Drop BANANA")
        print("  [g] - Throw GREEN SHELL")
        print("  [l] - Return to LOBBY")
        print("  [q] - Quit")
        print("="*60 + "\n")

        last_car_update = time.time()
        update_rate = 1.0 / 15.0  # 15 Hz like the real game

        while self.running and self.in_race:
            # Send car updates at 15 Hz
            if time.time() - last_car_update >= update_rate:
                self.update_car_physics()
                self.send_car_update()
                last_car_update = time.time()

            # Receive packets
            self.receive_packets()

            # Check for keyboard input
            if sys.platform != 'win32':
                if select.select([sys.stdin], [], [], 0)[0]:
                    key = sys.stdin.read(1)
                    self.handle_race_input(key)

            time.sleep(0.01)

    def handle_race_input(self, key):
        """Handle keyboard input in race mode"""
        if key.lower() == 'b':
            # Drop banana
            self.send_item_placed(Item.ITEM_BANANA)
        elif key.lower() == 'g':
            # Throw green shell
            self.send_item_placed(Item.ITEM_GREEN_SHELL)
        elif key.lower() == 'l':
            # Return to lobby
            print("\n🏠 Returning to LOBBY mode!")
            self.in_race = False
            self.is_ready = False
            self.send_lobby_join()
        elif key.lower() == 'q':
            # Quit
            self.running = False

    def run(self):
        """Main loop"""
        try:
            # Enable raw terminal mode on Unix for non-blocking input
            if sys.platform != 'win32':
                import termios
                import tty
                old_settings = termios.tcgetattr(sys.stdin)
                try:
                    tty.setcbreak(sys.stdin.fileno())

                    while self.running:
                        if self.in_race:
                            self.run_race_mode()
                        else:
                            self.run_lobby_mode()
                finally:
                    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
            else:
                # Windows: simplified mode
                print("⚠️  Windows detected: keyboard controls limited")
                print("    Use Ctrl+C to quit")
                while self.running:
                    if self.in_race:
                        self.run_race_mode()
                    else:
                        self.run_lobby_mode()

        except KeyboardInterrupt:
            print("\n\n⚠️  Interrupted by user")
        finally:
            self.cleanup()

    def cleanup(self):
        """Cleanup and disconnect"""
        print(f"\n{'='*60}")
        print("Disconnecting...")
        self.send_disconnect()
        self.sock.close()
        print(f"📊 Final Stats:")
        print(f"   Packets Sent: {self.packets_sent}")
        print(f"   Packets Received: {self.packets_received}")
        print(f"{'='*60}")
        print("👋 Goodbye!")


def main():
    parser = argparse.ArgumentParser(
        description='Simulate a Nintendo DS player for multiplayer testing',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Simulate Player 7 (broadcast to all devices)
  python simulate_ds_player.py --player-id 7

  # Simulate Player 5 targeting specific DS
  python simulate_ds_player.py --player-id 5 --target 192.168.1.100

  # Use different port
  python simulate_ds_player.py --player-id 3 --port 8888
        """
    )

    parser.add_argument('--player-id', type=int, default=1,
                       help='Player ID to simulate (0-7, default: 1 = Player 2)')
    parser.add_argument('--target', type=str, default=None,
                       help='Target IP address (default: auto-detect broadcast)')
    parser.add_argument('--port', type=int, default=GAME_PORT,
                       help=f'UDP port (default: {GAME_PORT})')
    parser.add_argument('--race', action='store_true',
                       help='Start directly in race mode (skip lobby)')

    args = parser.parse_args()

    if args.player_id < 0 or args.player_id >= MAX_MULTIPLAYER_PLAYERS:
        print(f"❌ Error: player-id must be between 0 and {MAX_MULTIPLAYER_PLAYERS - 1}")
        return 1

    # Auto-detect broadcast address if not specified
    target_ip = args.target
    if target_ip is None:
        target_ip = get_broadcast_address()

    print(f"""
╔════════════════════════════════════════════════════════════╗
║          Kart Mania - DS Player Simulator                 ║
╚════════════════════════════════════════════════════════════╝

Player ID: {args.player_id} (Player {args.player_id + 1})
Target IP: {target_ip}
Port: {args.port}

Starting...
""")

    player = SimulatedDSPlayer(args.player_id, target_ip, args.port)

    if args.race:
        player.in_race = True

    player.run()

    return 0


if __name__ == '__main__':
    sys.exit(main())
