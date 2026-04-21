# MyTeams Documentation

## Overview

MyTeams is a C++17 client/server application that implements a collaborative
messaging platform over TCP. The server manages users, teams, channels, threads,
replies, and private messages. The client provides an interactive command-line
interface.

This documentation is generated with Doxygen from project headers, sources,
and protocol notes.

## Architecture

- Server core: poll-based event loop and packet dispatching.
- Business layer: command handlers grouped in the mtp namespace.
- Data model: shared plain structs in include/data.hpp.
- Protocol: command, response, error, and event codes in include/protocole.hpp.
- Client: command parser, request builder, and server response handlers.

## Wire Protocol

The protocol uses a fixed 6-byte header:

- 2 bytes command/response code (network byte order)
- 4 bytes payload size (network byte order)

See doc/RFC.md for the complete payload formats and behavior rules.

## Entry Points

- myteams_server: src/server/main.cpp
- myteams_cli: src/client/main.cpp

## Build

- make
- make clean
- make fclean
- make re

## Documentation Build

- doxygen Doxyfile

Generated HTML output location:

- doc/doxygen/html/index.html
