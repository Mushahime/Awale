# Awale Game - Client/Server Implementation

A networked implementation of the traditional African board game Awale (also known as Oware) with additional social features like friend lists, private messaging, and spectator mode.

## Overview

This project implements a client-server architecture where multiple clients can connect to play Awale against each other. The server handles game logic, user management, and communication between clients, while the client provides the user interface and game visualization.

## Compilation

### Prerequisites
- GCC compiler
- Linux environment
- Make utility

### Building
```bash
make clean
make
```

This will create two executables: `client` and `server`

## Launch Options

### Manual Launch
1. Start the server
```bash
go in the repo "Serveur"
./server 2000
```

2. Start multiple clients in different terminals:
```bash
go in the repo "Serveur"
./client 127.0.0.1 2000
```

### Using Shell Script
```bash
./launch.sh
```
This script will automatically start the server and four client instances.

```bash
./cleanup_port.sh
```
This script will clean all ports (If that doesn't work, we recommend running the programs manually.)

## User Manual

### Connection

- TODO

### Main Menu Options

- TODO

### During Game

- TODO

### Special Features

- TODO

### Nota Bene

- TODO

## Credits

Developed by:
- Noam CATHERINE
- Abderramane BOUZIANE
(Project as part on courses in network programming)

# TODO
Demain :
- Readme/man user, tests
- demo preparation
- remmettre à 24 le score pour le jeu awale
- update code
- deconnection friendlist ??
- action unitaire bloquante
- autres pb

