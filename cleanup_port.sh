#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

# Port to clean up and change if necessary
PORT=1234

echo "Cleaning up port $PORT..."

# Kill server and client processes
pkill -f "./server $PORT"
pkill -f "./client localhost $PORT"

# Find and kill processes using the port
if command -v lsof >/dev/null; then
    PIDS=$(lsof -ti :$PORT)
    if [ ! -z "$PIDS" ]; then
        echo -e "${RED}Killing processes:${NC} $PIDS"
        kill -9 $PIDS 2>/dev/null
    fi
fi

# Wait for the port to be freed
sleep 1

# Check if the port is still in use
if lsof -i :$PORT >/dev/null 2>&1; then
    echo -e "${RED}Port $PORT is still in use${NC}"
else
    echo -e "${GREEN}Port $PORT successfully freed!${NC}"
fi

# Kill xterm on Linux
if [[ "$OSTYPE" != "darwin"* ]]; then
    pkill xterm
fi