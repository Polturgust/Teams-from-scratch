# RFC MyTeams Protocol (MTP/1.0)

## 1. Introduction

Ce document décrit le protocole de communication entre le client `myteams_cli` et le serveur `myteams_server`. Le protocole fonctionne au-dessus de TCP et utilise un format binaire à header fixe suivi d'un payload.

## 2. Architecture des messages

Chaque message (client → serveur ou serveur → client) suit le même format:

```
+------------------+------------------+--------------------+
|  command (2 oct) | payload_size (4) |   payload (N oct)  |
+------------------+------------------+--------------------+
|    uint16_t      |    uint32_t      |   variable         |
+------------------+------------------+--------------------+
```

- **command** : code de la commande ou de la réponse (voir sections 3 et 4)
- **payload_size** : taille en octets du payload qui suit (peut être 0)
- **payload** : données sérialisées, format dépend de la commande

**Taille totale du header : 6 octets.**

Tous les entiers sont en **network byte order** (big-endian). Utiliser `htons()`/`ntohs()` pour les uint16_t et `htonl()`/`ntohl()` pour les uint32_t.

## 3. Codes de commande (client → serveur)

| Code | Nom              | Payload                                          |
|------|------------------|--------------------------------------------------|
| 0x01 | CMD_LOGIN        | name (32 oct, padded null)                       |
| 0x02 | CMD_LOGOUT       | (vide)                                           |
| 0x03 | CMD_USERS        | (vide)                                           |
| 0x04 | CMD_USER         | user_uuid (36 oct)                               |
| 0x05 | CMD_SEND         | receiver_uuid (36) + body (512, padded null)     |
| 0x06 | CMD_MESSAGES     | user_uuid (36 oct)                               |
| 0x07 | CMD_SUBSCRIBE    | team_uuid (36 oct)                                |
| 0x08 | CMD_SUBSCRIBED   | flag (1 oct) + team_uuid (36 oct) si flag=0x01   |

> CMD_SUBSCRIBED : si flag=0x00 → liste les teams auxquelles le client est subscribed (réponse RES_SUBSCRIBED_TEAMS). Si flag=0x01 + team_uuid → liste les users subscribed à cette team (réponse RES_SUBSCRIBED_USERS).
| 0x09 | CMD_UNSUBSCRIBE  | team_uuid (36 oct)                                |
| 0x0A | CMD_USE          | voir section 5                                   |
| 0x0B | CMD_CREATE       | voir section 6                                   |
| 0x0C | CMD_LIST         | (vide, le serveur utilise le contexte USE)        |
| 0x0D | CMD_INFO         | (vide, le serveur utilise le contexte USE)        |

## 4. Codes de réponse (serveur → client)

### 4.1 Réponses de succès

| Code | Nom                    | Payload                                          |
|------|------------------------|--------------------------------------------------|
| 0x10 | RES_LOGIN_OK           | user_uuid (36) + name (32)                       |
| 0x11 | RES_LOGOUT_OK          | user_uuid (36) + name (32)                       |
| 0x12 | RES_USERS_LIST         | count (4 oct) + [user_uuid (36) + name (32) + status (1)]* |
| 0x13 | RES_USER_INFO          | user_uuid (36) + name (32) + status (1)          |
| 0x14 | RES_SEND_OK            | (vide)                                           |
| 0x15 | RES_MESSAGES_LIST      | count (4) + [sender_uuid (36) + receiver_uuid (36) + timestamp (8) + body (512)]* |
| 0x16 | RES_SUBSCRIBE_OK       | team_uuid (36)                                   |
| 0x17 | RES_SUBSCRIBED_TEAMS   | count (4) + [team_uuid (36) + name (32) + desc (255)]* |
| 0x18 | RES_SUBSCRIBED_USERS   | count (4) + [user_uuid (36) + name (32) + status (1)]* |
| 0x19 | RES_UNSUBSCRIBE_OK     | team_uuid (36)                                   |
| 0x1A | RES_TEAM_CREATED       | team_uuid (36) + name (32) + desc (255)          |
| 0x1B | RES_CHANNEL_CREATED    | channel_uuid (36) + name (32) + desc (255)       |
| 0x1C | RES_THREAD_CREATED     | thread_uuid (36) + creator_uuid (36) + timestamp (8) + title (32) + body (512) |
| 0x1D | RES_REPLY_CREATED      | thread_uuid (36) + creator_uuid (36) + timestamp (8) + body (512) |
| 0x1E | RES_LIST_TEAMS         | count (4) + [team_uuid (36) + name (32) + desc (255)]* |
| 0x1F | RES_LIST_CHANNELS      | count (4) + [channel_uuid (36) + name (32) + desc (255)]* |
| 0x20 | RES_LIST_THREADS       | count (4) + [thread_uuid (36) + creator_uuid (36) + timestamp (8) + title (32) + body (512)]* |
| 0x21 | RES_LIST_REPLIES       | count (4) + [thread_uuid (36) + creator_uuid (36) + timestamp (8) + body (512)]* |
| 0x22 | RES_TEAM_INFO          | team_uuid (36) + name (32) + desc (255)          |
| 0x23 | RES_CHANNEL_INFO       | channel_uuid (36) + name (32) + desc (255)       |
| 0x24 | RES_THREAD_INFO        | thread_uuid (36) + creator_uuid (36) + timestamp (8) + title (32) + body (512) |
| 0x25 | RES_USER_DETAILS       | user_uuid (36) + name (32)                       |

### 4.2 Réponses d'erreur

| Code | Nom                    | Payload                                          |
|------|------------------------|--------------------------------------------------|
| 0xE0 | ERR_UNAUTHORIZED       | (vide) — pas logged in                           |
| 0xE1 | ERR_UNKNOWN_TEAM       | team_uuid (36)                                   |
| 0xE2 | ERR_UNKNOWN_CHANNEL    | channel_uuid (36)                                |
| 0xE3 | ERR_UNKNOWN_THREAD     | thread_uuid (36)                                 |
| 0xE4 | ERR_UNKNOWN_USER       | user_uuid (36)                                   |
| 0xE5 | ERR_ALREADY_EXISTS     | (vide)                                           |
| 0xE6 | ERR_NOT_SUBSCRIBED     | (vide)                                           |
| 0xE7 | ERR_INVALID_COMMAND    | (vide)                                           |

### 4.3 Événements push (serveur → client, non sollicités)

| Code | Nom                    | Payload                                          |
|------|------------------------|--------------------------------------------------|
| 0xF0 | EVT_MESSAGE_RECEIVED   | sender_uuid (36) + body (512)                    |
| 0xF1 | EVT_TEAM_CREATED       | team_uuid (36) + name (32) + desc (255)          |
| 0xF2 | EVT_CHANNEL_CREATED    | channel_uuid (36) + name (32) + desc (255)       |
| 0xF3 | EVT_THREAD_CREATED     | thread_uuid (36) + creator_uuid (36) + timestamp (8) + title (32) + body (512) |
| 0xF4 | EVT_REPLY_CREATED      | thread_uuid (36) + creator_uuid (36) + timestamp (8) + body (512) |
| 0xF5 | EVT_USER_LOGGED_IN     | user_uuid (36) + name (32)                       |
| 0xF6 | EVT_USER_LOGGED_OUT    | user_uuid (36) + name (32)                       |

## 5. Commande USE (0x0A)

La commande USE définit le contexte courant du client côté serveur. Le payload est variable :

| Contexte          | Payload                                                  |
|-------------------|----------------------------------------------------------|
| Reset (aucun)     | level (1 oct) = 0x00                                     |
| Team              | level (1 oct) = 0x01 + team_uuid (36)                   |
| Team + Channel    | level (1 oct) = 0x02 + team_uuid (36) + channel_uuid (36) |
| Team + Chan + Thread | level (1 oct) = 0x03 + team_uuid (36) + channel_uuid (36) + thread_uuid (36) |

Le serveur répond selon le niveau :
- level 0x00 (reset) : pas de réponse spécifique, le contexte est simplement effacé. Le serveur répond avec un `RES_USER_DETAILS` contenant les infos du user logged.
- level 0x01 : `RES_TEAM_INFO` ou `ERR_UNKNOWN_TEAM`
- level 0x02 : `RES_CHANNEL_INFO` ou `ERR_UNKNOWN_CHANNEL`
- level 0x03 : `RES_THREAD_INFO` ou `ERR_UNKNOWN_THREAD`

Le serveur stocke le contexte USE par client. Les commandes `/create`, `/list` et `/info` l'utilisent.

Pour `/info` sans contexte USE, le serveur répond avec `RES_USER_DETAILS` (uuid + name du user logged).

## 6. Commande CREATE (0x0B)

Le payload dépend du contexte USE actuel du client :

| Contexte courant        | Payload attendu                        | Réponse                |
|-------------------------|----------------------------------------|------------------------|
| Aucun                   | name (32) + description (255)          | RES_TEAM_CREATED       |
| Team                    | name (32) + description (255)          | RES_CHANNEL_CREATED    |
| Team + Channel          | title (32) + body (512)                | RES_THREAD_CREATED     |
| Team + Channel + Thread | body (512)                             | RES_REPLY_CREATED      |

## 7. Sérialisation des champs

Tous les champs de taille fixe sont **paddés avec des octets null** (`\0`) :

- **UUID** : 36 octets (format `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`, sans null terminal)
- **name / title** : 32 octets (padded null)
- **description** : 255 octets (padded null)
- **body** : 512 octets (padded null)
- **timestamp** : 8 octets, `int64_t` en network byte order (`time_t` cast en int64)
- **status** : 1 octet (0x00 = offline, 0x01 = online)
- **count** : 4 octets, `uint32_t` en network byte order

## 8. Gestion de la connexion TCP

- Le client ouvre une connexion TCP vers le serveur
- La connexion reste ouverte tant que le client ne fait pas `/logout` ou ne ferme pas
- Le serveur détecte la déconnexion via `poll()` (POLLHUP ou read retourne 0)
- Le serveur gère un **buffer de réception** par client pour reconstituer les messages (TCP est un flux, un `read()` peut retourner un message partiel)
- Le serveur gère un **buffer d'envoi** par client et n'écrit que quand `POLLOUT` est signalé

## 9. Diagramme de séquence typique

```
Client                          Server
  |                               |
  |--- CMD_LOGIN [name] --------->|
  |<-- RES_LOGIN_OK [uuid,name] --|
  |<-- EVT_USER_LOGGED_IN --------|  (broadcast aux autres clients)
  |                               |
  |--- CMD_USE [level=1,team] --->|
  |<-- RES_TEAM_INFO -------------|
  |                               |
  |--- CMD_CREATE [name,desc] --->|  (contexte = team → crée channel)
  |<-- RES_CHANNEL_CREATED -------|
  |   (EVT_CHANNEL_CREATED ------>|  broadcast aux subscribers)
  |                               |
  |--- CMD_SEND [uuid,body] ----->|
  |<-- RES_SEND_OK ---------------|
  |   (EVT_MESSAGE_RECEIVED ----->|  au destinataire si connecté)
  |                               |
  |--- CMD_LOGOUT --------------->|
  |<-- RES_LOGOUT_OK -------------|
  |   (EVT_USER_LOGGED_OUT ------>|  broadcast)
```

## 10. Règles de sécurité

1. Un client non-logged (pas de CMD_LOGIN réussi) ne peut exécuter AUCUNE commande → ERR_UNAUTHORIZED
2. Un client non-subscribed à une team ne peut pas CMD_CREATE dans cette team → ERR_NOT_SUBSCRIBED
3. Un client non-subscribed ne reçoit PAS les EVT_* de cette team
4. Les EVT_USER_LOGGED_IN / EVT_USER_LOGGED_OUT sont envoyés à tous les clients logged
5. Les EVT_TEAM_CREATED sont envoyés à tous les clients logged
6. Les EVT_CHANNEL_CREATED, EVT_THREAD_CREATED, EVT_REPLY_CREATED ne sont envoyés qu'aux subscribers de la team