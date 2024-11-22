#!/bin/bash

# Define colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

# Function to print colored messages
print_message() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Function to kill processes by port
kill_port_processes() {
    local port=$1
    local pids=$(lsof -ti :$port 2>/dev/null)

    if [ ! -z "$pids" ]; then
        print_message "$RED" "Killing processes on port $port: $pids"
        kill -9 $pids 2>/dev/null
        return 0
    fi
    return 1
}

# Function to check if a port is in use
check_port() {
    local port=$1
    if lsof -i :$port >/dev/null 2>&1; then
        return 0 # Port is in use
    fi
    return 1 # Port is free
}

# Function to kill specific application processes
kill_app_processes() {
    local port=$1

    # Kill server and client processes for this specific port
    pkill -f "./server $port"
    pkill -f "./client localhost $port"
}

# Function to cleanup a range of ports
cleanup_port_range() {
    local base_port=$1
    local num_ports=$2

    print_message "$GREEN" "Cleaning up ports $base_port through $((base_port + num_ports - 1))..."

    # Kill xterm on Linux (only once, not per port)
    if [[ "$OSTYPE" != "darwin"* ]]; then
        pkill xterm
    fi

    # Clean each port in the range
    for ((i = 0; i < num_ports; i++)); do
        local current_port=$((base_port + i))

        # Kill application specific processes
        kill_app_processes $current_port

        # Kill any remaining processes using the port
        if command -v lsof >/dev/null; then
            kill_port_processes $current_port
        fi
    done

    # Wait for ports to be freed
    sleep 1

    # Verify port status
    local all_ports_free=true
    for ((i = 0; i < num_ports; i++)); do
        local current_port=$((base_port + i))
        if check_port $current_port; then
            print_message "$RED" "Port $current_port is still in use"
            all_ports_free=false
        else
            print_message "$GREEN" "Port $current_port successfully freed!"
        fi
    done

    if $all_ports_free; then
        print_message "$GREEN" "All ports successfully cleaned!"
        return 0
    else
        print_message "$RED" "Some ports are still in use"
        return 1
    fi
}

# Main execution starts here
main() {
    local BASE_PORT=1234
    local NUM_PORTS=5 # Will clean ports 1234 through 1238
    cleanup_port_range $BASE_PORT $NUM_PORTS
}

# Execute main function
main
