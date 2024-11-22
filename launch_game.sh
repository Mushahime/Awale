#!/bin/bash

# Function to check if a port is available
check_port() {
    if ! lsof -i :$1 > /dev/null 2>&1; then
        return 0 # Port is available
    fi
    return 1 # Port is in use
}

# Function to find next available port
find_available_port() {
    local base_port=$1
    local max_attempts=5
    local port
    for i in $(seq 0 $((max_attempts-1))); do
        port=$((base_port + i))
        if check_port $port; then
            echo $port
            return 0
        fi
    done
    echo "No available ports found in range $base_port-$((base_port + max_attempts - 1))" >&2
    exit 1
}

# Function to check if the server is running and accepting connections
check_server() {
    local port=$1
    local max_attempts=10
    local attempt=1
    
    while [ $attempt -le $max_attempts ]; do
        if nc -z localhost $port 2>/dev/null; then
            return 0
        fi
        sleep 1
        ((attempt++))
    done
    return 1
}

# Function to cleanup processes on error
cleanup() {
    echo "Error occurred, cleaning up..."
    # Kill any running server or client processes
    pkill -f "./server $PORT"
    pkill -f "./client localhost $PORT"
    exit 1
}

# Set trap for cleanup on script termination
trap cleanup EXIT INT TERM

# Base port (will try this and the next 4 ports)
BASE_PORT=1234

# Find an available port
PORT=$(find_available_port $BASE_PORT)
echo "Using port: $PORT"

# Compile the server and client
make clean
if ! make; then
    echo "Compilation failed!"
    exit 1
fi

# Launch the server in a new terminal
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    if ! osascript -e "tell app \"Terminal\" to do script \"cd $(pwd) && ./server $PORT\""; then
        echo "Failed to launch server"
        exit 1
    fi
else
    # Linux with xterm
    xterm -hold -e "./Serveur/server $PORT" &
    if [ $? -ne 0 ]; then
        echo "Failed to launch server"
        exit 1
    fi
fi

# Wait for the server to start and verify it's accepting connections
echo "Waiting for server to start..."
if ! check_server $PORT; then
    echo "Server failed to start or accept connections!"
    cleanup
    exit 1
fi

# Define an array of pseudo names
PSEUDOS=("player1" "player2" "player3" "player4")

# Launch the clients in new terminals with unique pseudos
for i in {0..3}; do
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        if ! osascript -e "tell app \"Terminal\" to do script \"cd $(pwd) && ./client localhost $PORT ${PSEUDOS[$i]}\""; then
            echo "Failed to launch client ${PSEUDOS[$i]}"
            cleanup
            exit 1
        fi
    else
        # Linux with xterm
        xterm -hold -e "./Client/client localhost $PORT ${PSEUDOS[$i]}" &
        if [ $? -ne 0 ]; then
            echo "Failed to launch client ${PSEUDOS[$i]}"
            cleanup
            exit 1
        fi
    fi
    
    # Wait a bit before launching the next client
    sleep 1
done

# Remove trap as everything started successfully
trap - EXIT INT TERM

echo "Server and clients launched successfully on port $PORT!"