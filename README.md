# Lab IHM Server

A C++ TCP chat server built with Qt, SQLite, and libsodium for secure messaging.

## Overview

This project implements a backend server for a chat application as part of the IHM (Interface Homme-Machine) lab. It provides user authentication, message storage, and real-time messaging capabilities over TCP connections using JSON-based communication.

## Features

- User authentication with Argon2id password hashing
- SQLite database for persistent storage
- TCP server handling JSON messages
- Session management for connected clients
- Message history and user management
- Admin user support

## Dependencies

- Qt6 or Qt5 (Core, Sql, Network)
- libsodium (for cryptography)
- CMake (for building)

## Building

1. Ensure you have Qt and libsodium installed on your system.
2. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```
3. Configure with CMake:
   ```bash
   cmake ..
   ```
4. Build the project:
   ```bash
   make
   ```

## Running

Use the provided script:
```bash
./run.sh
```

Or run directly:
```bash
./lab-ihm-server
```

The server will start listening on port 8080.

## Database Setup

The server uses SQLite with a database file `database.sqlite`. The schema includes:
- `users` table: user information with hashed passwords
- `messages` table: message storage with sender/receiver relationships

Default users are created on first run:
- admin (password: admin)
- jordan (password: admin)
- francois (password: admin)

## API

The server communicates via JSON messages over TCP. Supported client message types:
- LOGIN: User authentication
- SEND: Send a message
- DELETE: Delete a message
- ERROR: Error handling

## Testing

You can test the server using telnet:
```bash
telnet localhost 8080
```

## Project Structure

- `main.cpp`: Application entry point and server setup
- `server.cpp/hpp`: Server logic and session management
- `model.cpp/hpp`: Data structures and message handling
- `setup.sql`: Database schema and initial data
- `CMakeLists.txt`: Build configuration
- `run.sh`: Convenience script for building and running