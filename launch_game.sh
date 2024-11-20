#!/bin/bash

# Function to check if a port is already in use
check_port() {
    if lsof -i :$1 > /dev/null; then
        echo "Port $1 is already in use"
        exit 1
    fi
}

# Port for the server (change if necessary)
PORT=1234

# Check if the port is already in use
check_port $PORT

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

# Launch the clients in new terminals
for i in {1..4}; do
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        osascript -e "tell app \"Terminal\" to do script \"cd $(pwd) && ./client localhost $PORT\""
    else
        # Linux with xterm
        xterm -hold -e "./Client/client localhost $PORT" &
    fi
    sleep 1
done

echo "Server and clients launched successfully!"