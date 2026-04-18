# RFC MyTeams Protocol (MTP/1.0)

**Status:** Final — **Version:** 1.0 — **Date:** 2025

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Message Format](#2-message-format)
3. [Command Codes — Client → Server](#3-command-codes--client--server)
4. [Response Codes — Server → Client](#4-response-codes--server--client)
5. [Push Events](#5-push-events)
6. [Serialization Rules](#6-serialization-rules)
7. [CMD_USE — Context Management](#7-cmd_use--context-management)
8. [CMD_CREATE — Context-Aware Creation](#8-cmd_create--context-aware-creation)
9. [CMD_LIST and CMD_INFO](#9-cmd_list-and-cmd_info)
10. [CMD_SUBSCRIBED — Dual-Mode Query](#10-cmd_subscribed--dual-mode-query)
11. [Push Event Distribution Rules](#11-push-event-distribution-rules)
12. [Connection Lifecycle](#12-connection-lifecycle)
13. [Security Rules](#13-security-rules)
14. [Sequence Diagrams](#14-sequence-diagrams)

---

## 1. Introduction

MTP/1.0 is the binary protocol used between `myteams_cli` and `myteams_server` over TCP.
It uses a **fixed 6-byte header** followed by a variable payload.
All integers are in **network byte order (big-endian)**.

---

## 2. Message Format

Every message in both directions follows this layout:

```
+------------------+------------------+--------------------+
|  command (2 B)   | payload_size (4B) |   payload (N B)    |
+------------------+------------------+--------------------+
|    uint16_t      |    uint32_t      |   variable         |
+------------------+------------------+--------------------+
```

- Total header size: **6 bytes**.
- `payload_size` may be 0.
- TCP is a stream — both sides buffer incoming bytes and only process a message once `header + payload` are fully received.

---

## 3. Command Codes — Client → Server

| Code   | Name              | Payload                                                        |
|--------|-------------------|----------------------------------------------------------------|
| `0x01` | `CMD_LOGIN`       | `name` (32 B, null-padded)                                     |
| `0x02` | `CMD_LOGOUT`      | *(empty)*                                                      |
| `0x03` | `CMD_USERS`       | *(empty)*                                                      |
| `0x04` | `CMD_USER`        | `user_uuid` (36 B)                                             |
| `0x05` | `CMD_SEND`        | `receiver_uuid` (36 B) + `body` (512 B, null-padded)           |
| `0x06` | `CMD_MESSAGES`    | `user_uuid` (36 B)                                             |
| `0x07` | `CMD_SUBSCRIBE`   | `team_uuid` (36 B)                                             |
| `0x08` | `CMD_SUBSCRIBED`  | `flag` (1 B) + `team_uuid` (36 B, only if `flag = 0x01`)      |
| `0x09` | `CMD_UNSUBSCRIBE` | `team_uuid` (36 B)                                             |
| `0x0A` | `CMD_USE`         | `level` (1 B) + UUIDs depending on level — see §7              |
| `0x0B` | `CMD_CREATE`      | Depends on USE context — see §8                                |
| `0x0C` | `CMD_LIST`        | *(empty — server uses stored USE context)*                     |
| `0x0D` | `CMD_INFO`        | *(empty — server uses stored USE context)*                     |

> All commands except `CMD_LOGIN` require authentication. Unauthenticated commands return `ERR_UNAUTHORIZED`.

---

## 4. Response Codes — Server → Client

### 4.1 Success Responses

| Code   | Name                   | Payload                                                                              |
|--------|------------------------|--------------------------------------------------------------------------------------|
| `0x10` | `RES_LOGIN_OK`         | `user_uuid` (36) + `name` (32)                                                       |
| `0x11` | `RES_LOGOUT_OK`        | `user_uuid` (36) + `name` (32)                                                       |
| `0x12` | `RES_USERS_LIST`       | `count` (4) + [`user_uuid` (36) + `name` (32) + `status` (1)]*                      |
| `0x13` | `RES_USER_INFO`        | `user_uuid` (36) + `name` (32) + `status` (1)                                       |
| `0x14` | `RES_SEND_OK`          | *(empty)*                                                                             |
| `0x15` | `RES_MESSAGES_LIST`    | `count` (4) + [`sender_uuid` (36) + `receiver_uuid` (36) + `timestamp` (8) + `body` (512)]* |
| `0x16` | `RES_SUBSCRIBE_OK`     | `team_uuid` (36)                                                                     |
| `0x17` | `RES_SUBSCRIBED_TEAMS` | `count` (4) + [`team_uuid` (36) + `name` (32) + `desc` (255)]*                      |
| `0x18` | `RES_SUBSCRIBED_USERS` | `count` (4) + [`user_uuid` (36) + `name` (32) + `status` (1)]*                      |
| `0x19` | `RES_UNSUBSCRIBE_OK`   | `team_uuid` (36)                                                                     |
| `0x1A` | `RES_TEAM_CREATED`     | `team_uuid` (36) + `name` (32) + `desc` (255)                                       |
| `0x1B` | `RES_CHANNEL_CREATED`  | `channel_uuid` (36) + `name` (32) + `desc` (255)                                    |
| `0x1C` | `RES_THREAD_CREATED`   | `thread_uuid` (36) + `creator_uuid` (36) + `timestamp` (8) + `title` (32) + `body` (512) |
| `0x1D` | `RES_REPLY_CREATED`    | `thread_uuid` (36) + `creator_uuid` (36) + `timestamp` (8) + `body` (512)           |
| `0x1E` | `RES_LIST_TEAMS`       | `count` (4) + [`team_uuid` (36) + `name` (32) + `desc` (255)]*                      |
| `0x1F` | `RES_LIST_CHANNELS`    | `count` (4) + [`channel_uuid` (36) + `name` (32) + `desc` (255)]*                   |
| `0x20` | `RES_LIST_THREADS`     | `count` (4) + [`thread_uuid` (36) + `creator_uuid` (36) + `timestamp` (8) + `title` (32) + `body` (512)]* |
| `0x21` | `RES_LIST_REPLIES`     | `count` (4) + [`thread_uuid` (36) + `creator_uuid` (36) + `timestamp` (8) + `body` (512)]* |
| `0x22` | `RES_TEAM_INFO`        | `team_uuid` (36) + `name` (32) + `desc` (255)                                       |
| `0x23` | `RES_CHANNEL_INFO`     | `channel_uuid` (36) + `name` (32) + `desc` (255)                                    |
| `0x24` | `RES_THREAD_INFO`      | `thread_uuid` (36) + `creator_uuid` (36) + `timestamp` (8) + `title` (32) + `body` (512) |
| `0x25` | `RES_USER_DETAILS`     | `user_uuid` (36) + `name` (32)                                                       |

### 4.2 Error Responses

| Code   | Name                   | Payload             | Meaning                                    |
|--------|------------------------|---------------------|--------------------------------------------|
| `0xE0` | `ERR_UNAUTHORIZED`     | *(empty)*           | Not logged in, or insufficient permissions |
| `0xE1` | `ERR_UNKNOWN_TEAM`     | `team_uuid` (36)    | Team not found                             |
| `0xE2` | `ERR_UNKNOWN_CHANNEL`  | `channel_uuid` (36) | Channel not found                          |
| `0xE3` | `ERR_UNKNOWN_THREAD`   | `thread_uuid` (36)  | Thread not found                           |
| `0xE4` | `ERR_UNKNOWN_USER`     | `user_uuid` (36)    | User not found                             |
| `0xE5` | `ERR_ALREADY_EXISTS`   | *(empty)*           | Resource name already exists               |
| `0xE6` | `ERR_NOT_SUBSCRIBED`   | *(empty)*           | Action requires team subscription          |
| `0xE7` | `ERR_INVALID_COMMAND`  | *(empty)*           | Malformed or unexpected command            |

---

## 5. Push Events

Push events are **server-initiated** — sent without a prior client request.

| Code   | Name                    | Payload                                                                          |
|--------|-------------------------|----------------------------------------------------------------------------------|
| `0xF0` | `EVT_MESSAGE_RECEIVED`  | `sender_uuid` (36) + `body` (512)                                                |
| `0xF1` | `EVT_TEAM_CREATED`      | `team_uuid` (36) + `name` (32) + `desc` (255)                                   |
| `0xF2` | `EVT_CHANNEL_CREATED`   | `channel_uuid` (36) + `name` (32) + `desc` (255)                                |
| `0xF3` | `EVT_THREAD_CREATED`    | `thread_uuid` (36) + `creator_uuid` (36) + `timestamp` (8) + `title` (32) + `body` (512) |
| `0xF4` | `EVT_REPLY_CREATED`     | `thread_uuid` (36) + `creator_uuid` (36) + `timestamp` (8) + `body` (512)       |
| `0xF5` | `EVT_USER_LOGGED_IN`    | `user_uuid` (36) + `name` (32)                                                   |
| `0xF6` | `EVT_USER_LOGGED_OUT`   | `user_uuid` (36) + `name` (32)                                                   |

---

## 6. Serialization Rules

All fixed-size string fields are **null-padded** to their defined length.

| Field          | Wire size | Encoding                                                          |
|----------------|-----------|-------------------------------------------------------------------|
| UUID           | 36 B      | ASCII `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`, no null terminator  |
| `name`/`title` | 32 B      | UTF-8, null-padded                                                |
| `description`  | 255 B     | UTF-8, null-padded                                                |
| `body`         | 512 B     | UTF-8, null-padded                                                |
| `timestamp`    | 8 B       | `int64_t`, big-endian (manual `htonll`)                           |
| `status`       | 1 B       | `0x00` = offline, `0x01` = online                                 |
| `count`        | 4 B       | `uint32_t`, big-endian                                            |

---

## 7. CMD_USE — Context Management

| Level   | Value  | Payload                                                           | Server Response          |
|---------|--------|-------------------------------------------------------------------|--------------------------|
| Reset   | `0x00` | `level` (1 B)                                                     | `RES_USER_DETAILS`       |
| Team    | `0x01` | `level` + `team_uuid` (36 B)                                     | `RES_TEAM_INFO`          |
| Channel | `0x02` | `level` + `team_uuid` (36 B) + `channel_uuid` (36 B)            | `RES_CHANNEL_INFO`       |
| Thread  | `0x03` | `level` + `team_uuid` (36 B) + `channel_uuid` (36 B) + `thread_uuid` (36 B) | `RES_THREAD_INFO` |

The context is stored per client in `server_data_t::client_contexts[fd]` and used by `CMD_CREATE`, `CMD_LIST`, `CMD_INFO`.

---

## 8. CMD_CREATE — Context-Aware Creation

| Context       | Payload                     | Response              |
|---------------|-----------------------------|-----------------------|
| No USE        | `name` (32) + `desc` (255)  | `RES_TEAM_CREATED`    |
| Team          | `name` (32) + `desc` (255)  | `RES_CHANNEL_CREATED` |
| Team+Channel  | `title` (32) + `body` (512) | `RES_THREAD_CREATED`  |
| Team+Chan+Thread | `body` (512)             | `RES_REPLY_CREATED`   |

---

## 9. CMD_LIST and CMD_INFO

No payload. The server uses the stored USE context.

| Context      | CMD_LIST            | CMD_INFO           |
|--------------|---------------------|--------------------|
| No USE       | `RES_LIST_TEAMS`    | `RES_USER_DETAILS` |
| Team         | `RES_LIST_CHANNELS` | `RES_TEAM_INFO`    |
| Channel      | `RES_LIST_THREADS`  | `RES_CHANNEL_INFO` |
| Thread       | `RES_LIST_REPLIES`  | `RES_THREAD_INFO`  |

---

## 10. CMD_SUBSCRIBED — Dual-Mode Query

| `flag` | Extra payload      | Response               |
|--------|--------------------|------------------------|
| `0x00` | *(none)*           | `RES_SUBSCRIBED_TEAMS` |
| `0x01` | `team_uuid` (36 B) | `RES_SUBSCRIBED_USERS` |

---

## 11. Push Event Distribution Rules

| Event                  | Recipients                              |
|------------------------|-----------------------------------------|
| `EVT_USER_LOGGED_IN`   | All logged-in clients                   |
| `EVT_USER_LOGGED_OUT`  | All logged-in clients                   |
| `EVT_TEAM_CREATED`     | All logged-in clients                   |
| `EVT_MESSAGE_RECEIVED` | Recipient only (if connected)           |
| `EVT_CHANNEL_CREATED`  | All subscribers of the parent team      |
| `EVT_THREAD_CREATED`   | All subscribers of the parent team      |
| `EVT_REPLY_CREATED`    | All subscribers of the parent team      |

---

## 12. Connection Lifecycle

1. Client opens a TCP connection to `ip:port`.
2. Connection stays open until `CMD_LOGOUT` or close.
3. Server detects disconnection via `poll()` (`POLLHUP` or `read()` = 0).
4. On unexpected disconnection the server logs the user out and broadcasts `EVT_USER_LOGGED_OUT`.
5. Server uses `poll()` for all I/O — no `fork()`, no threads.
6. On `SIGINT` the server saves its full state to disk before exiting.
7. On startup the server loads the save file if present.

---

## 13. Security Rules

1. Any command before `CMD_LOGIN` → `ERR_UNAUTHORIZED`.
2. `CMD_CREATE` in a team without subscription → `ERR_NOT_SUBSCRIBED`.
3. Non-subscribers never receive `EVT_CHANNEL_CREATED`, `EVT_THREAD_CREATED`, `EVT_REPLY_CREATED`.
4. Private messages are delivered only to the intended recipient.

---

## 14. Sequence Diagrams

### Login / Logout

```
Client                          Server
  |-- CMD_LOGIN [name] -------->|
  |<- RES_LOGIN_OK [uuid,name] -|
  |<- EVT_USER_LOGGED_IN -------|  (broadcast)

  |-- CMD_LOGOUT -------------->|
  |<- RES_LOGOUT_OK ------------|
  |<- EVT_USER_LOGGED_OUT ------|  (broadcast)
```

### Create team → channel → thread → reply

```
Client                          Server
  |-- CMD_CREATE [name,desc] -->|  (no context → team)
  |<- RES_TEAM_CREATED ---------|
  |<- EVT_TEAM_CREATED ---------|  (all logged clients)

  |-- CMD_USE [0x01, team] ---->|
  |<- RES_TEAM_INFO ------------|
  |-- CMD_CREATE [name,desc] -->|  (team context → channel)
  |<- RES_CHANNEL_CREATED ------|
  |<- EVT_CHANNEL_CREATED ------|  (team subscribers)

  |-- CMD_USE [0x02,t,c] ------>|
  |<- RES_CHANNEL_INFO ---------|
  |-- CMD_CREATE [title,body] ->|  (channel context → thread)
  |<- RES_THREAD_CREATED -------|
  |<- EVT_THREAD_CREATED -------|  (team subscribers)

  |-- CMD_USE [0x03,t,c,th] --->|
  |<- RES_THREAD_INFO ----------|
  |-- CMD_CREATE [body] ------->|  (thread context → reply)
  |<- RES_REPLY_CREATED --------|
  |<- EVT_REPLY_CREATED --------|  (team subscribers)
```

### Private message

```
Client A                        Server                      Client B
  |-- CMD_SEND [uuid_B,body] -->|                               |
  |<- RES_SEND_OK --------------|                               |
  |                             |-- EVT_MESSAGE_RECEIVED ------>|
```