# ShareMap Configuration Web Server

A Flask-based web application for configuring hardware radios via a dynamic form interface. The server reads configuration schema from `schema.yaml`, generates a user-friendly form, validates inputs, and sends configurations via UDP.

## 📁 Project Structure

```
sharemap_web_server/
├── app.py                 # Main Flask application
├── sharemap_lib.py        # Sharemap serialization library (provided)
├── schema.yaml           # Enhanced configuration schema with validation
├── udp_receiver.py       # UDP test receiver for verification
├── requirements.txt      # Python dependencies
├── run.sh               # Convenience script to run the server
├── Dockerfile           # Docker configuration
├── docker-compose.yml   # Docker Compose configuration
├── templates/
│   └── index.html       # Main form template
└── static/
    ├── style.css        # Stylesheet
    └── script.js        # Client-side JavaScript
```

## 🚀 Quick Start

### Option 1: Using the Run Script (Recommended)

```bash
# Make the script executable (if not already)
chmod +x run.sh

# Run with default settings
./run.sh

# Run with UDP receiver for testing
./run.sh --with-receiver

# Run in debug mode on custom port
./run.sh --port 9000 --debug
```

### Option 2: Manual Setup

```bash
# Create virtual environment (recommended)
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Start the web server
python app.py --port 8080

# In a separate terminal, start the UDP receiver for testing
python udp_receiver.py --port 5000
```

### Option 3: Using Docker

```bash
# Build and run with Docker Compose
docker-compose up --build

# Run with UDP receiver for testing
docker-compose --profile testing up --build
```

## 🌐 Accessing the Interface

Once running, open your browser to:
- **Web Interface**: http://localhost:8080

## 📝 Features

### Dynamic Form Generation
- Forms are automatically generated from `schema.yaml`
- Fields are organized into logical groups (PSK CC TX, PSK CC RX, DVB-S2 TX, GFSK TX, Anylink)
- Appropriate input types for each field (toggles for booleans, dropdowns for enums, numeric inputs with validation)

### Validation
- **Client-side**: Immediate feedback on invalid inputs
- **Server-side**: Full validation before sending
- **Range checking**: Min/max values enforced for numeric fields
- **Mutual exclusion**: Prevents enabling conflicting channels (e.g., STX1 and STX2)
- **Type validation**: Ensures correct data types

### Schema Enhancements
The enhanced `schema.yaml` includes:
- Default values for all fields
- Min/max ranges for numeric fields
- Step values for fine-grained control
- Unit annotations (Hz, dB, seconds, etc.)
- Dropdown options for enum fields
- Mutual exclusion constraints

### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Main configuration form |
| `/submit` | POST | Submit form and send UDP |
| `/api/schema` | GET | Get schema as JSON |
| `/api/defaults` | GET | Get default values |
| `/api/validate` | POST | Validate configuration (JSON body) |
| `/api/send` | POST | Send configuration via API |
| `/export` | GET | Download current defaults as YAML |

### Example API Usage

```bash
# Validate a configuration
curl -X POST http://localhost:8080/api/validate \
  -H "Content-Type: application/json" \
  -d '{"psk_cc_tx_force_on": false, "psk_cc_tx_idle_timeout_s": 10, ...}'

# Send a configuration via API
curl -X POST http://localhost:8080/api/send \
  -H "Content-Type: application/json" \
  -d '{
    "config": {...},
    "host": "localhost",
    "port": 5000
  }'
```

## 🔧 Configuration

### UDP Target
The form includes fields to specify the UDP target:
- **Host**: IP address or hostname (default: localhost)
- **Port**: UDP port number (default: 5000)

### Server Settings
Command-line options for `app.py`:
- `--host`: Host to bind to (default: 0.0.0.0)
- `--port`: Port to listen on (default: 8080)
- `--debug`: Enable Flask debug mode

## 🧪 Testing

### End-to-End Test

1. Start the UDP receiver:
   ```bash
   python udp_receiver.py --port 5000
   ```

2. Start the web server:
   ```bash
   python app.py --port 8080
   ```

3. Open http://localhost:8080 in your browser

4. Fill in the form (or use defaults) and click "Send Configuration"

5. Verify the received data in the UDP receiver terminal

### Validation Test

Click the "Validate" button to check all fields without sending. The validation summary will show any errors.

### Unit Test (Manual)

```python
# Test sharemap serialization
import sharemap_lib
import yaml

with open('schema.yaml') as f:
    schema = yaml.safe_load(f)

sharemap = sharemap_lib.Sharemap(schema['config'])

# Create test config
config = {
    'psk_cc_tx_force_on': False,
    'psk_cc_tx_idle_timeout_s': 10,
    # ... other fields
}

# Pack and unpack
packed = sharemap.pack(config)
unpacked = sharemap.unpack(packed)

assert config == {k: v for k, v in unpacked.items() 
                  if k not in ('source_id', 'schema_hash', 'unix_timestamp_ns')}
```

## 📊 Schema Enhancements

The enhanced schema adds the following validation properties:

| Property | Description | Example |
|----------|-------------|---------|
| `default` | Default value | `10` |
| `min` | Minimum allowed value | `0` |
| `max` | Maximum allowed value | `3600` |
| `step` | Value increment | `0.25` |
| `unit` | Display unit | `Hz`, `dB`, `seconds` |
| `options` | Allowed values for enums | `["BPSK", "QPSK"]` |
| `mutex_with` | Mutually exclusive field | `psk_cc_tx_fe_stx2_enable` |

## 🎨 UI Features

- **Responsive design**: Works on desktop and mobile
- **Dark mode**: Automatically adapts to system preference
- **Collapsible groups**: Click group headers to collapse/expand
- **Real-time validation**: Errors shown immediately
- **Flash messages**: Success/error notifications
- **Export**: Download configuration as YAML file

## 🔒 Security Notes

- The server binds to all interfaces (0.0.0.0) by default for convenience
- For production, consider using a reverse proxy (nginx) with HTTPS
- No authentication is implemented - add as needed for production use

## 📚 Original Files Reference

The implementation uses the provided `sharemap_lib.py` for packet serialization. The schema structure follows the original `schema.yaml` format with enhancements for validation.

## 🐛 Troubleshooting

**Port already in use**:
```bash
# Find and kill process using the port
lsof -i :8080
kill <PID>
```

**Module not found**:
```bash
# Ensure you're in the correct directory
cd sharemap_web_server
pip install -r requirements.txt
```

**UDP not receiving**:
- Verify firewall settings
- Check that both server and receiver are on the same network
- Try using `127.0.0.1` instead of `localhost`

---

*Implementation completed for ShareMap Configuration Take-Home Assignment*
