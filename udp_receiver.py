#!/usr/bin/env python3
"""
UDP Test Receiver for ShareMap Configuration

This script receives and unpacks UDP packets sent by the ShareMap Configuration
web server, displaying the decoded configuration values for verification.

Usage:
    python udp_receiver.py [--port PORT]
"""

import socket
import sys
import os
import yaml
import argparse
from datetime import datetime

# Add the current directory to path to import sharemap_lib
THIS_DIR = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, THIS_DIR)

import sharemap_lib


def format_value(value, field_type):
    """Format a value for display based on its type."""
    if field_type in ('f32', 'f64'):
        if abs(value) >= 1e6 or (abs(value) < 1e-3 and value != 0):
            return f"{value:.4e}"
        return f"{value:.4f}"
    elif field_type == 'boolean':
        return '✓ True' if value else '✗ False'
    elif field_type in ('u64', 'i64'):
        return f"{value:,}"
    return str(value)


def print_config(config, schema):
    """Pretty print the decoded configuration."""
    print("\n" + "=" * 60)
    print(f"Configuration Received at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 60)
    
    # Group by prefix
    current_group = None
    
    for key, value in config.items():
        # Skip internal fields
        if key in ('source_id', 'schema_hash', 'unix_timestamp_ns'):
            continue
        
        # Determine group
        if key.startswith('psk_cc_tx'):
            group = 'PSK CC TX'
        elif key.startswith('psk_cc_rx'):
            group = 'PSK CC RX'
        elif key.startswith('dvbs2_tx'):
            group = 'DVB-S2 TX'
        elif key.startswith('gfsk_tx'):
            group = 'GFSK TX'
        elif key.startswith('anylink'):
            group = 'Anylink'
        else:
            group = 'Other'
        
        if group != current_group:
            print(f"\n--- {group} ---")
            current_group = group
        
        # Get field type from schema if available
        field_type = schema.get(key, {}).get('type', 'string')
        formatted = format_value(value, field_type)
        
        # Get unit if available
        unit = schema.get(key, {}).get('unit', '')
        unit_str = f" {unit}" if unit else ""
        
        print(f"  {key}: {formatted}{unit_str}")
    
    # Print internal fields
    print("\n--- Internal Fields ---")
    print(f"  source_id: {config.get('source_id', 'N/A')}")
    print(f"  schema_hash: {hex(config.get('schema_hash', 0))}")
    ts = config.get('unix_timestamp_ns', 0)
    if ts:
        ts_sec = ts / 1e9
        dt = datetime.fromtimestamp(ts_sec)
        print(f"  timestamp: {dt.strftime('%Y-%m-%d %H:%M:%S.%f')}")
    
    print("=" * 60)


def main(port, schema_path):
    """Main receiver loop."""
    # Load schema
    print(f"Loading schema from: {schema_path}")
    with open(schema_path, 'r') as f:
        schema = yaml.safe_load(f)
    
    config_schema = schema['config']
    config_sharemap = sharemap_lib.Sharemap(config_schema)
    
    # Get expected packet size
    expected_size = config_sharemap._packed_size
    
    print(f"\n{'=' * 60}")
    print(f"ShareMap UDP Test Receiver")
    print(f"{'=' * 60}")
    print(f"Listening on port: {port}")
    print(f"Expected packet size: {expected_size} bytes")
    print(f"Schema hash: {hex(config_sharemap.get_hash())}")
    print(f"\nWaiting for packets... (Press Ctrl+C to stop)")
    print(f"{'=' * 60}\n")
    
    # Create UDP socket
    sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
    sock.bind(('0.0.0.0', port))
    
    packet_count = 0
    
    try:
        while True:
            data, addr = sock.recvfrom(4096)
            packet_count += 1
            
            print(f"\n📡 Packet #{packet_count} received from {addr[0]}:{addr[1]}")
            print(f"   Size: {len(data)} bytes")
            
            if len(data) != expected_size:
                print(f"   ⚠️  WARNING: Expected {expected_size} bytes, got {len(data)}")
            
            try:
                config = config_sharemap.unpack(data)
                print_config(config, config_schema)
            except Exception as e:
                print(f"   ❌ ERROR unpacking data: {str(e)}")
                print(f"   Raw data (hex): {data[:64].hex()}...")
    
    except KeyboardInterrupt:
        print(f"\n\nReceiver stopped. Total packets received: {packet_count}")
    finally:
        sock.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='ShareMap UDP Test Receiver')
    parser.add_argument('--port', type=int, default=5000, help='UDP port to listen on (default: 5000)')
    parser.add_argument('--schema', type=str, default=os.path.join(THIS_DIR, 'schema.yaml'),
                       help='Path to schema.yaml file')
    
    args = parser.parse_args()
    main(args.port, args.schema)
