#!/usr/bin/env python3
"""
ShareMap Configuration Web Server

A Flask-based web application for configuring hardware radios.
Provides a dynamic form interface based on schema.yaml and sends
configurations via UDP.

Author: Take-Home Assignment Implementation
"""

import os
import sys
import socket
import json
import yaml
from flask import Flask, render_template, request, jsonify, redirect, url_for, flash
from urllib.parse import urlparse
import sharemap_lib

# Configuration
THIS_DIR = os.path.abspath(os.path.dirname(__file__))
SCHEMA_PATH = os.path.join(THIS_DIR, 'schema.yaml')
DEFAULT_UDP_HOST = 'localhost'
DEFAULT_UDP_PORT = 5000

app = Flask(__name__)
app.secret_key = os.urandom(24)


def load_schema():
    """Load and parse the schema.yaml file."""
    with open(SCHEMA_PATH, 'r') as f:
        return yaml.safe_load(f)


def get_field_groups(config_schema):
    """
    Organize fields into logical groups based on their names.
    Returns a dictionary of group_name -> list of (field_name, field_details)
    """
    groups = {
        'PSK CC TX': [],
        'PSK CC RX': [],
        'DVB-S2 TX': [],
        'GFSK TX': [],
        'Anylink': []
    }
    
    for field_name, details in config_schema.items():
        if field_name.startswith('psk_cc_tx'):
            groups['PSK CC TX'].append((field_name, details))
        elif field_name.startswith('psk_cc_rx'):
            groups['PSK CC RX'].append((field_name, details))
        elif field_name.startswith('dvbs2_tx'):
            groups['DVB-S2 TX'].append((field_name, details))
        elif field_name.startswith('gfsk_tx'):
            groups['GFSK TX'].append((field_name, details))
        elif field_name.startswith('anylink'):
            groups['Anylink'].append((field_name, details))
    
    return groups


def get_default_values(config_schema):
    """Extract default values from schema."""
    defaults = {}
    for field_name, details in config_schema.items():
        if 'default' in details:
            value = details['default']
            # Convert scientific notation strings to floats for numeric fields
            field_type = details.get('type', 'string')
            if field_type in ('f32', 'f64') and isinstance(value, str):
                try:
                    value = float(value)
                except (ValueError, TypeError):
                    pass
            defaults[field_name] = value
        else:
            # Provide sensible defaults based on type
            field_type = details.get('type', 'string')
            if field_type == 'boolean':
                defaults[field_name] = False
            elif field_type in ('u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64'):
                defaults[field_name] = 0
            elif field_type in ('f32', 'f64'):
                defaults[field_name] = 0.0
            else:
                defaults[field_name] = ''
    return defaults


def convert_form_value(value, field_type):
    """Convert form string value to appropriate Python type."""
    if field_type == 'boolean':
        return value.lower() in ('true', 'on', '1', 'yes')
    elif field_type in ('u8', 'u16', 'u32', 'u64'):
        return int(float(value))  # Handle scientific notation
    elif field_type in ('i8', 'i16', 'i32', 'i64'):
        return int(float(value))
    elif field_type in ('f32', 'f64'):
        return float(value)
    else:
        return str(value)


def validate_field(field_name, value, details):
    """
    Validate a field value against schema constraints.
    Returns (is_valid, error_message)
    """
    errors = []
    field_type = details.get('type', 'string')
    
    # Check min/max for numeric types
    if field_type in ('u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64', 'f32', 'f64'):
        try:
            # Handle both numeric and string representations (including scientific notation)
            if isinstance(value, (int, float)):
                num_value = float(value)
            else:
                num_value = float(str(value))
            
            # Convert min/max to float (they may be strings with scientific notation)
            if 'min' in details:
                min_val = float(details['min']) if isinstance(details['min'], str) else float(details['min'])
                if num_value < min_val:
                    errors.append(f"Value must be >= {details['min']}")
            if 'max' in details:
                max_val = float(details['max']) if isinstance(details['max'], str) else float(details['max'])
                if num_value > max_val:
                    errors.append(f"Value must be <= {details['max']}")
        except (ValueError, TypeError) as e:
            errors.append("Invalid numeric value")
    
    # Check options for string types
    if field_type == 'string' and 'options' in details:
        if value not in details['options']:
            errors.append(f"Value must be one of: {', '.join(str(o) for o in details['options'])}")
    
    # Check string length (max 64 bytes for sharemap strings)
    if field_type == 'string' and len(value.encode('utf-8')) > 63:
        errors.append("String too long (max 63 characters)")
    
    return (len(errors) == 0, errors)


def validate_mutex_constraints(config, config_schema):
    """
    Validate mutex (mutual exclusion) constraints between fields.
    Returns list of error messages.
    """
    errors = []
    checked = set()
    
    for field_name, details in config_schema.items():
        if 'mutex_with' in details and field_name not in checked:
            mutex_field = details['mutex_with']
            checked.add(field_name)
            checked.add(mutex_field)
            
            # Check if both are enabled
            val1 = config.get(field_name, False)
            val2 = config.get(mutex_field, False)
            
            if val1 and val2:
                errors.append(f"'{field_name}' and '{mutex_field}' cannot both be enabled")
    
    return errors


def send_udp_config(config, host, port):
    """
    Pack and send configuration via UDP.
    Returns (success, message)
    """
    try:
        # Load schema and create sharemap
        schema = load_schema()
        config_sharemap = sharemap_lib.Sharemap(schema['config'])
        
        # Pack the configuration
        buff = config_sharemap.pack(config)
        
        # Create UDP socket and send
        sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
        sock.sendto(buff, (host, port))
        sock.close()
        
        return True, f"Configuration sent successfully to {host}:{port} ({len(buff)} bytes)"
    
    except Exception as e:
        return False, f"Error sending configuration: {str(e)}"


@app.route('/')
def index():
    """Main page with configuration form."""
    schema = load_schema()
    config_schema = schema['config']
    groups = get_field_groups(config_schema)
    defaults = get_default_values(config_schema)
    
    return render_template('index.html', 
                         groups=groups, 
                         defaults=defaults,
                         config_schema=config_schema)


@app.route('/api/schema', methods=['GET'])
def get_schema():
    """API endpoint to get the schema as JSON."""
    schema = load_schema()
    return jsonify(schema['config'])


@app.route('/api/defaults', methods=['GET'])
def get_defaults():
    """API endpoint to get default values."""
    schema = load_schema()
    defaults = get_default_values(schema['config'])
    return jsonify(defaults)


@app.route('/api/validate', methods=['POST'])
def validate_config():
    """
    API endpoint to validate a configuration.
    Expects JSON body with configuration values.
    """
    schema = load_schema()
    config_schema = schema['config']
    
    data = request.get_json()
    if not data:
        return jsonify({'valid': False, 'errors': {'_general': ['No data provided']}}), 400
    
    errors = {}
    
    # Validate each field
    for field_name, details in config_schema.items():
        if field_name in data:
            is_valid, field_errors = validate_field(field_name, data[field_name], details)
            if not field_errors:
                pass
            else:
                errors[field_name] = field_errors
        else:
            errors[field_name] = ['Field is required']
    
    # Validate mutex constraints
    mutex_errors = validate_mutex_constraints(data, config_schema)
    if mutex_errors:
        errors['_mutex'] = mutex_errors
    
    return jsonify({
        'valid': len(errors) == 0,
        'errors': errors
    })


@app.route('/submit', methods=['POST'])
def submit_config():
    """Handle form submission and send configuration via UDP."""
    schema = load_schema()
    config_schema = schema['config']
    
    # Get UDP target from form
    udp_host = request.form.get('udp_host', DEFAULT_UDP_HOST)
    udp_port = int(request.form.get('udp_port', DEFAULT_UDP_PORT))
    
    # Build configuration from form data
    config = {}
    validation_errors = []
    
    for field_name, details in config_schema.items():
        field_type = details.get('type', 'string')
        
        # Handle boolean checkboxes specially
        if field_type == 'boolean':
            value = field_name in request.form
            config[field_name] = value
        else:
            raw_value = request.form.get(field_name, '')
            
            # Validate
            is_valid, errors = validate_field(field_name, raw_value, details)
            if not is_valid:
                validation_errors.append(f"{field_name}: {', '.join(errors)}")
            
            # Convert to appropriate type
            try:
                config[field_name] = convert_form_value(raw_value, field_type)
            except (ValueError, TypeError) as e:
                validation_errors.append(f"{field_name}: Invalid value - {str(e)}")
    
    # Validate mutex constraints
    mutex_errors = validate_mutex_constraints(config, config_schema)
    validation_errors.extend(mutex_errors)
    
    if validation_errors:
        for error in validation_errors:
            flash(error, 'error')
        return redirect(url_for('index'))
    
    # Send via UDP
    success, message = send_udp_config(config, udp_host, udp_port)
    
    if success:
        flash(message, 'success')
    else:
        flash(message, 'error')
    
    return redirect(url_for('index'))


@app.route('/api/send', methods=['POST'])
def api_send_config():
    """
    API endpoint to send configuration.
    Expects JSON body with:
    - config: dict of configuration values
    - host: UDP host (optional, default localhost)
    - port: UDP port (optional, default 5000)
    """
    schema = load_schema()
    config_schema = schema['config']
    
    data = request.get_json()
    if not data or 'config' not in data:
        return jsonify({'success': False, 'error': 'No configuration provided'}), 400
    
    config = data['config']
    host = data.get('host', DEFAULT_UDP_HOST)
    port = data.get('port', DEFAULT_UDP_PORT)
    
    # Validate all fields
    errors = {}
    for field_name, details in config_schema.items():
        if field_name in config:
            is_valid, field_errors = validate_field(field_name, config[field_name], details)
            if field_errors:
                errors[field_name] = field_errors
        else:
            errors[field_name] = ['Field is required']
    
    # Validate mutex constraints  
    mutex_errors = validate_mutex_constraints(config, config_schema)
    if mutex_errors:
        errors['_mutex'] = mutex_errors
    
    if errors:
        return jsonify({'success': False, 'errors': errors}), 400
    
    # Convert types
    converted_config = {}
    for field_name, details in config_schema.items():
        field_type = details.get('type', 'string')
        try:
            converted_config[field_name] = convert_form_value(str(config[field_name]), field_type)
        except (ValueError, TypeError) as e:
            return jsonify({'success': False, 'error': f"Type conversion error for {field_name}: {str(e)}"}), 400
    
    # Send configuration
    success, message = send_udp_config(converted_config, host, port)
    
    return jsonify({
        'success': success,
        'message': message,
        'bytes_sent': len(sharemap_lib.Sharemap(config_schema).pack(converted_config)) if success else 0
    })


@app.route('/export', methods=['GET'])
def export_config():
    """Export current default configuration as YAML."""
    schema = load_schema()
    defaults = get_default_values(schema['config'])
    
    yaml_str = yaml.dump(defaults, default_flow_style=False, sort_keys=False)
    
    return app.response_class(
        response=yaml_str,
        status=200,
        mimetype='application/x-yaml',
        headers={'Content-Disposition': 'attachment; filename=sharemap_config.yaml'}
    )


if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='ShareMap Configuration Web Server')
    parser.add_argument('--host', default='0.0.0.0', help='Host to bind to')
    parser.add_argument('--port', type=int, default=8080, help='Port to listen on')
    parser.add_argument('--debug', action='store_true', help='Enable debug mode')
    
    args = parser.parse_args()
    
    print(f"Starting ShareMap Configuration Server on http://{args.host}:{args.port}")
    print(f"Schema loaded from: {SCHEMA_PATH}")
    
    app.run(host=args.host, port=args.port, debug=args.debug)
