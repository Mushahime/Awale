#!/bin/bash

# Function to check if a port is available
check_port() {
    if ! lsof -i :"$1" >/dev/null 2>&1; then
        return 0 # Port is available
    fi
    return 1 # Port is in use
}

# Function to find the next available port
find_available_port() {
    local base_port=$1
    local max_attempts=5
    local port

    for i in $(seq 0 $((max_attempts - 1))); do
        port=$((base_port + i))
        if check_port "$port"; then
            echo "$port"
            return 0
        fi
    done

    echo "No available ports found in range $base_port-$((base_port + max_attempts - 1))" >&2
    exit 1
}

# Base port (will try this and the next 4 ports)
BASE_PORT=1234

# Find an available port
PORT=$(find_available_port "$BASE_PORT")
echo "Using port: $PORT"

# Compile the server and client
make clean
if ! make; then
    echo "Compilation failed. Exiting." >&2
    exit 1
fi

# Get the current directory
CURRENT_DIR="$(pwd)"

# Define paths to server and client executables
SERVER_PATH="$CURRENT_DIR/Serveur/server"
CLIENT_PATH="$CURRENT_DIR/Client/client"

# Check if server and client executables exist
if [[ ! -x "$SERVER_PATH" ]]; then
    echo "Server executable not found at $SERVER_PATH or is not executable." >&2
    exit 1
fi

if [[ ! -x "$CLIENT_PATH" ]]; then
    echo "Client executable not found at $CLIENT_PATH or is not executable." >&2
    exit 1
fi

# Launch the server in a new terminal
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    osascript -e "tell application \"Terminal\" to do script \"cd '$CURRENT_DIR/Serveur' && ./server $PORT; exec bash\""
else
    # Linux with gnome-terminal
    if command -v gnome-terminal >/dev/null 2>&1; then
        gnome-terminal -- bash -c "cd '$CURRENT_DIR/Serveur' && ./server $PORT; exec bash" &
    else
        echo "gnome-terminal not found. Please install it or modify the script to use your preferred terminal emulator." >&2
        exit 1
    fi
fi

# Wait for the server to start
sleep 2

# Define an array of pseudo names
PSEUDOS=("player1" "player2" "player3" "player4")

# Launch the clients in new terminals with unique pseudos
for i in {0..3}; do
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        osascript -e "tell application \"Terminal\" to do script \"cd '$CURRENT_DIR/Client' && ./client localhost $PORT ${PSEUDOS[$i]}; exec bash\""
    else
        # Linux with gnome-terminal
        if command -v gnome-terminal >/dev/null 2>&1; then
            gnome-terminal -- bash -c "cd '$CURRENT_DIR/Client' && ./client localhost $PORT ${PSEUDOS[$i]}; exec bash" &
        else
            echo "gnome-terminal not found. Please install it or modify the script to use your preferred terminal emulator." >&2
            exit 1
        fi
    fi
    sleep 1
done

echo "Server and clients launched successfully on port $PORT!"
