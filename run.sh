#!/bin/bash

# ShareMap Configuration Web Server - Run Script

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}ShareMap Configuration Web Server${NC}"
echo "=================================="

# Check Python version
PYTHON_CMD=""
if command -v python3 &> /dev/null; then
    PYTHON_CMD="python3"
elif command -v python &> /dev/null; then
    PYTHON_CMD="python"
else
    echo -e "${RED}Error: Python not found. Please install Python 3.8+${NC}"
    exit 1
fi

PYTHON_VERSION=$($PYTHON_CMD --version 2>&1 | cut -d' ' -f2)
echo -e "Python version: ${PYTHON_VERSION}"

# Check if we're in a virtual environment
if [[ -z "$VIRTUAL_ENV" ]]; then
    echo -e "${YELLOW}Note: Not running in a virtual environment${NC}"
    echo "Consider creating one with: python3 -m venv venv && source venv/bin/activate"
fi

# Install dependencies
echo ""
echo "Installing dependencies..."
$PYTHON_CMD -m pip install -r requirements.txt -q

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to install dependencies${NC}"
    exit 1
fi

echo -e "${GREEN}Dependencies installed successfully${NC}"

# Parse arguments
PORT=8080
DEBUG=""
START_RECEIVER=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --port)
            PORT="$2"
            shift 2
            ;;
        --debug)
            DEBUG="--debug"
            shift
            ;;
        --with-receiver)
            START_RECEIVER=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--port PORT] [--debug] [--with-receiver]"
            exit 1
            ;;
    esac
done

# Start UDP receiver in background if requested
if [ "$START_RECEIVER" = true ]; then
    echo ""
    echo -e "${YELLOW}Starting UDP receiver on port 5000...${NC}"
    $PYTHON_CMD udp_receiver.py --port 5000 &
    RECEIVER_PID=$!
    echo "UDP Receiver PID: $RECEIVER_PID"
    
    # Trap to clean up receiver on exit
    trap "echo 'Stopping UDP receiver...'; kill $RECEIVER_PID 2>/dev/null" EXIT
fi

# Start the web server
echo ""
echo -e "${GREEN}Starting web server on port ${PORT}...${NC}"
echo "Access the configuration interface at: http://localhost:${PORT}"
echo ""
echo "Press Ctrl+C to stop the server"
echo "=================================="

$PYTHON_CMD app.py --port $PORT $DEBUG
