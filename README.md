# Awale Game - Client/Server Implementation

A networked implementation of the traditional African board game Awale (also known as Oware) with additional social features like friend lists, private messaging, and spectator mode (for more info : https://fr.wikipedia.org/wiki/Awal%C3%A9)

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

N.B : the exec 'awale' is only to test the game in solo without client/server interactions.

## Launch Options

### Manual Launch
1. Start the server
```bash
go in the repo "Serveur"
./server 2000
```
./server <port>

2. Start multiple clients in different terminals:
```bash
go in the repo "Client"
./client 127.0.0.1 2000 test
```
./client <address> <port> <pseudo>
- `address`: Server IP address (e.g., 127.0.0.1 for local testing)
- `port`: Server port number (between 1024-65535)
- `pseudo`: Your nickname (2-15 characters, alphanumeric and underscore only)

### Using Shell Script
```bash
./launch_game.sh
```
This script will automatically start the server and four client instances.

```bash
./cleanup_port.sh
```
This script will clean all ports (If that doesn't work, we recommend running the programs manually.)

## User Manual 

### Main Menu Options (After connection)

1. **Send message to all**
   - Broadcast a message to all connected users
   - Enter your message and press Enter
   - Multi-line messages supported (end with empty line)

2. **Send private message**
   - Specify the receiver's pseudo
   - Write your muti-line message and press Enter to send

3. **List connected users**
   - Shows all online users and their current number of gained points

4. **Bio options**
   - View and update your player biography
   - Set personal information visible to other players

5. **Play awale vs someone**
   - Challenge another player to an Awale match
   - Specify if you want to set the game to be private or public
   - If the game was set to private the user has the option of making his friends spectators( press Enter),specify manually the players he wants as spectators using the foramt `<pseudo>:<pseudo>:..`
   - Enter opponent's username when prompted
   
6. **All games in progression**
   - View list of ongoing Awale matches
   - The user can directly choose a game to watch 

7. **See the save games**
   - Access history of your completed games
   - Press 1 to select the game you see
   
8. **Spectate a game**
   - Watch ongoing matches between other players
   - Enter player pseudo to spectate
   - View moves in real-time

9. **Blocked users**
   - Manage your list of blocked users
   - Block/unblock specific players

10. **Friend list**
    - View and manage your friends list
    - Add/remove friends

11. **Clear screen**
    - Clears the terminal
    - Refreshes the menu display
     
12. **Not implemented yet**
    - to be implemented later
  
13. **Quit**
    - Safely exits the application

### During Game

1. **Game Controls**
   - Enter a number (0 - 5) or (6 - 11) to select a pit for your move
   - Your pits are always displayed at the bottom

2. **Available Commands**
   - `quit`: Forfeit the current game
   - `mp:<pseudo>:<message>`: Send private message during game

### Special Features

1. **Time out (30 sec) during demand if the demand is not accepted**
2. **Unicity of pseudo**
3. **Good Managagement of exit ctrl c**
4. **Reutilisabity of the code**
5. **Error Management**
6. **Persistance is done for bio, point, friend and blocked people**

N.B : Some actions block messages of the server while not not done but these actions are atomic and not problematic (public messsage, ...).

## Credits

Developed by:
- Noam CATHERINE
- Abderramane BOUZIANE
(Project as part on courses in network programming)



Social Features

Friend list management
Private messaging
Player blocking
Custom biography
Game spectating

Game Features

ELO ranking system
Game history
Match replays
Private/Public games
Spectator mode