#!/bin/bash

# Function to check if a port is available
check_port() {
    if ! lsof -i :$1 > /dev/null 2>&1; then
        return 0  # Port is available
    fi
    return 1     # Port is in use
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

# Base port (will try this and the next 4 ports)
BASE_PORT=1234

# Find an available port
PORT=$(find_available_port $BASE_PORT)
echo "Using port: $PORT"

# Compile the server and client
make clean
make

# Launch the server in a new terminal
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    osascript -e "tell app \"Terminal\" to do script \"cd $(pwd) && ./server $PORT\""
else
    # Linux with xterm
    xterm -hold -e "./Serveur/server $PORT" &
fi

# Wait for the server to start
sleep 2

# Define an array of pseudo names
PSEUDOS=("player1" "player2" "player3" "player4")

# Launch the clients in new terminals with unique pseudos
for i in {0..3}; do
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        osascript -e "tell app \"Terminal\" to do script \"cd $(pwd) && ./client localhost $PORT ${PSEUDOS[$i]}\""
    else
        # Linux with xterm
        xterm -hold -e "./Client/client localhost $PORT ${PSEUDOS[$i]}" &
    fi
    sleep 1
done

echo "Server and clients launched successfully on port $PORT!"