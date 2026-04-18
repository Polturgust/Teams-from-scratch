# MyTeams

A collaborative communication server and CLI client — like Microsoft Teams, built from scratch over TCP.

## Requirements

- GCC/G++ with C++17
- `libuuid-dev` (`apt install uuid-dev`)
- The logging library must be present at `libs/myteams/` (provided)

## Compilation

```bash
make        # builds myteams_server and myteams_cli
make clean  # removes object files
make fclean # removes objects + binaries
make re     # fclean + all
```

## Usage

### Server

```bash
./myteams_server <port>
./myteams_server 4242
```

The server saves its state on `Ctrl-C` and reloads it on next startup.

### Client

```bash
./myteams_cli <ip> <port>
./myteams_cli 127.0.0.1 4242
```

## Commands

All arguments must be wrapped in double quotes.

| Command | Description |
|---|---|
| `/help` | Show command list |
| `/login "name"` | Log in (creates user if new) |
| `/logout` | Disconnect |
| `/users` | List all users |
| `/user "uuid"` | Get details about a user |
| `/send "uuid" "message"` | Send a private message |
| `/messages "uuid"` | Show conversation history with a user |
| `/subscribe "team_uuid"` | Subscribe to a team |
| `/subscribed` | List your subscribed teams |
| `/subscribed "team_uuid"` | List members of a team |
| `/unsubscribe "team_uuid"` | Unsubscribe from a team |
| `/use` | Reset context |
| `/use "team_uuid"` | Set context to a team |
| `/use "team_uuid" "channel_uuid"` | Set context to a channel |
| `/use "team_uuid" "channel_uuid" "thread_uuid"` | Set context to a thread |
| `/create ...` | Create a resource (see below) |
| `/list` | List sub-resources in current context |
| `/info` | Show details of current context |

### `/create` — context-dependent

| Context | Command | Creates |
|---|---|---|
| No context | `/create "name" "description"` | Team |
| Team | `/create "name" "description"` | Channel |
| Team + Channel | `/create "title" "body"` | Thread |
| Team + Channel + Thread | `/create "body"` | Reply |

## Example Session

```bash
# Terminal 1 — server
./myteams_server 4242

# Terminal 2 — Alice
./myteams_cli 127.0.0.1 4242
/login "alice"
/create "SquadAlpha" "Our team"
/use "<team_uuid>"
/create "general" "General channel"
/list

# Terminal 3 — Bob
./myteams_cli 127.0.0.1 4242
/login "bob"
/list
/subscribe "<team_uuid>"
/send "<alice_uuid>" "Hey Alice!"
```

## Project Structure

```
.
├── Makefile
├── README.md
├── docs/
│   └── RFC.md              # Protocol specification (MTP/1.0)
├── include/
│   ├── data.hpp            # Shared data structures
│   └── protocole.hpp       # Command/response/event codes
├── libs/
│   └── myteams/            # Provided logging library
├── src/
│   ├── server/
│   │   ├── main.cpp
│   │   ├── Server.cpp / Server.hpp
│   │   ├── Server_dispatch.cpp
│   │   ├── Server_queue_raw.cpp
│   │   ├── mtp.hpp
│   │   ├── mtp_detail.hpp
│   │   └── handlers/
│   │       ├── mtp_auth.cpp
│   │       ├── mtp_users.cpp
│   │       ├── mtp_messages.cpp
│   │       ├── mtp_subscriptions.cpp
│   │       ├── mtp_context.cpp
│   │       ├── mtp_create.cpp
│   │       ├── mtp_list.cpp
│   │       └── mtp_info.cpp
│   └── client/
│       ├── main.cpp
│       ├── Client.hpp
│       ├── Client.cpp
│       ├── Constructor.cpp
│       ├── Network.cpp
│       ├── Packet.cpp
│       ├── stdin_handling.cpp
│       ├── Command_parser.cpp
│       ├── Command_dispatch.cpp
│       ├── Command_help.cpp
│       ├── Individual.cpp
│       ├── Server_response.cpp
│       ├── helpers.hpp
│       └── helpers.cpp
└── tests/
    └── integration_test.sh
```

## Tests

```bash
chmod +x tests/integration_test.sh
./tests/integration_test.sh          # port 4242
./tests/integration_test.sh 9090     # custom port
```

The script covers: multi-client login, team/channel/thread/reply creation, private messaging, subscriber isolation, persistence after restart, parse errors, and unauthenticated access.

## Protocol

See [`doc/RFC.md`](doc/RFC.md) for the full MTP/1.0 specification.