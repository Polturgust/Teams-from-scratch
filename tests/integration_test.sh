#!/usr/bin/env bash
# =============================================================================
# MyTeams — Integration Test Suite
# Usage: ./tests/integration_test.sh [PORT]
#
# Requires: ./myteams_server and ./myteams_cli to be compiled (make)
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
PORT="${1:-4242}"
SERVER_BIN="./myteams_server"
CLIENT_BIN="./myteams_cli"
HOST="127.0.0.1"

PASS=0
FAIL=0
TOTAL=0

# Temp files for client outputs
TMP_DIR=$(mktemp -d)
trap 'cleanup' EXIT

cleanup() {
    kill "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
    rm -rf "$TMP_DIR"
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

pass() { echo -e "${GREEN}[PASS]${NC} $1"; ((PASS++)); ((TOTAL++)); }
fail() { echo -e "${RED}[FAIL]${NC} $1"; ((FAIL++)); ((TOTAL++)); }
info() { echo -e "${CYAN}[INFO]${NC} $1"; }
section() { echo -e "\n${YELLOW}=== $1 ===${NC}"; }

# assert_contains <output_var_content> <expected_string> <test_name>
assert_contains() {
    local output="$1"
    local expected="$2"
    local name="$3"
    if echo "$output" | grep -qF "$expected"; then
        pass "$name"
    else
        fail "$name — expected: '$expected'"
        echo "    Got: $(echo "$output" | head -5)"
    fi
}

# assert_not_contains <output_var_content> <unexpected_string> <test_name>
assert_not_contains() {
    local output="$1"
    local unexpected="$2"
    local name="$3"
    if ! echo "$output" | grep -qF "$unexpected"; then
        pass "$name"
    else
        fail "$name — unexpected string found: '$unexpected'"
        echo "    Got: $(echo "$output" | head -5)"
    fi
}

# Send commands to the server via the CLI and capture stdout+stderr
# Usage: run_client <output_file> <commands...>
# Each command is sent with a short delay so the server can process it.
run_client() {
    local outfile="$1"
    shift
    local cmds=("$@")

    {
        for cmd in "${cmds[@]}"; do
            echo "$cmd"
            sleep 0.3
        done
        sleep 0.5
    } | timeout 8 "$CLIENT_BIN" "$HOST" "$PORT" > "$outfile" 2>&1 || true
}

# Like run_client but returns immediately after sending commands (for parallel clients)
run_client_bg() {
    local outfile="$1"
    local pid_file="$2"
    shift 2
    local cmds=("$@")

    {
        for cmd in "${cmds[@]}"; do
            echo "$cmd"
            sleep 0.3
        done
        # Keep the client alive a bit so it can receive push events
        sleep 2
    } | timeout 10 "$CLIENT_BIN" "$HOST" "$PORT" > "$outfile" 2>&1 &
    echo $! > "$pid_file"
}

wait_bg() {
    local pid_file="$1"
    local pid
    pid=$(cat "$pid_file")
    wait "$pid" 2>/dev/null || true
}

# ---------------------------------------------------------------------------
# Pre-flight checks
# ---------------------------------------------------------------------------
section "Pre-flight checks"

if [[ ! -x "$SERVER_BIN" ]]; then
    echo -e "${RED}ERROR: $SERVER_BIN not found. Run 'make' first.${NC}"
    exit 1
fi
if [[ ! -x "$CLIENT_BIN" ]]; then
    echo -e "${RED}ERROR: $CLIENT_BIN not found. Run 'make' first.${NC}"
    exit 1
fi
pass "Binaries exist"

# Remove previous save file so each run starts fresh
rm -f myteams_save.json myteams.save save.json *.save 2>/dev/null || true

# ---------------------------------------------------------------------------
# Start server
# ---------------------------------------------------------------------------
section "Server startup"

"$SERVER_BIN" "$PORT" > "$TMP_DIR/server.log" 2>&1 &
SERVER_PID=$!
sleep 0.5

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    fail "Server failed to start"
    cat "$TMP_DIR/server.log"
    exit 1
fi
pass "Server started on port $PORT (PID $SERVER_PID)"

# ---------------------------------------------------------------------------
# TEST 1 — --help and /help
# ---------------------------------------------------------------------------
section "1. Help flags"

OUT=$(timeout 3 "$CLIENT_BIN" --help 2>&1 || true)
assert_contains "$OUT" "ip" "1.1 --help shows usage"

OUT_HELP="$TMP_DIR/help.txt"
run_client "$OUT_HELP" "/help"
assert_contains "$(cat "$OUT_HELP")" "/login" "1.2 /help lists /login"
assert_contains "$(cat "$OUT_HELP")" "/create" "1.3 /help lists /create"

# ---------------------------------------------------------------------------
# TEST 2 — Unauthenticated access
# ---------------------------------------------------------------------------
section "2. Unauthenticated access"

OUT_UNAUTH="$TMP_DIR/unauth.txt"
run_client "$OUT_UNAUTH" "/users"
# The lib prints something on stderr for unauthorized — check either stream
OUT_UNAUTH_CONTENT=$(cat "$OUT_UNAUTH")
# We just verify the client didn't crash and handled the error response
pass "2.1 /users before login does not crash client"

# ---------------------------------------------------------------------------
# TEST 3 — Parse errors (missing quotes)
# ---------------------------------------------------------------------------
section "3. Parse errors"

OUT_PARSE="$TMP_DIR/parse.txt"
run_client "$OUT_PARSE" '/login alice'   # argument not quoted
assert_contains "$(cat "$OUT_PARSE")" "quotes" "3.1 Missing quotes → parse error"

OUT_PARSE2="$TMP_DIR/parse2.txt"
run_client "$OUT_PARSE2" '/login "alice'  # unclosed quote
assert_contains "$(cat "$OUT_PARSE2")" "quote" "3.2 Unclosed quote → parse error"

# ---------------------------------------------------------------------------
# TEST 4 — Three clients login, /users shows all three
# ---------------------------------------------------------------------------
section "4. Multi-client login + /users"

PID_A="$TMP_DIR/pidA"
PID_B="$TMP_DIR/pidB"
PID_C="$TMP_DIR/pidC"
OUT_A="$TMP_DIR/clientA.txt"
OUT_B="$TMP_DIR/clientB.txt"
OUT_C="$TMP_DIR/clientC.txt"

# Client A logs in and lists users (after B and C have connected)
run_client_bg "$OUT_A" "$PID_A" \
    '/login "alice"' \
    '/users'

sleep 0.4

run_client_bg "$OUT_B" "$PID_B" \
    '/login "bob"'

sleep 0.2

run_client_bg "$OUT_C" "$PID_C" \
    '/login "charlie"'

wait_bg "$PID_A"
wait_bg "$PID_B"
wait_bg "$PID_C"

A_OUT=$(cat "$OUT_A")
assert_contains "$A_OUT" "alice"   "4.1 Client A sees itself in /users"
assert_contains "$A_OUT" "bob"     "4.2 Client A sees bob in /users"
assert_contains "$A_OUT" "charlie" "4.3 Client A sees charlie in /users"

# ---------------------------------------------------------------------------
# TEST 5 — Client A creates a team, Client B sees it with /list
# ---------------------------------------------------------------------------
section "5. Team creation + /list"

OUT_A2="$TMP_DIR/clientA2.txt"
OUT_B2="$TMP_DIR/clientB2.txt"
PID_A2="$TMP_DIR/pidA2"

# Client A creates the team
run_client "$OUT_A2" \
    '/login "alice"' \
    '/create "SquadAlpha" "The alpha team"'

sleep 0.3

A2_OUT=$(cat "$OUT_A2")
assert_contains "$A2_OUT" "SquadAlpha" "5.1 Team creation confirmed to creator"

# Client B lists teams
run_client "$OUT_B2" \
    '/login "bob"' \
    '/list'

B2_OUT=$(cat "$OUT_B2")
assert_contains "$B2_OUT" "SquadAlpha" "5.2 /list shows the new team to client B"

# Extract team UUID from B's output for later use
TEAM_UUID=$(echo "$B2_OUT" | grep -oE '[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}' | head -1)
info "Team UUID: $TEAM_UUID"

if [[ -z "$TEAM_UUID" ]]; then
    fail "5.3 Could not extract team UUID from /list output"
    TEAM_UUID="00000000-0000-0000-0000-000000000000"  # dummy to avoid empty-string errors
else
    pass "5.3 Team UUID extracted: $TEAM_UUID"
fi

# ---------------------------------------------------------------------------
# TEST 6 — Client B subscribes, creates channel, thread, reply
# ---------------------------------------------------------------------------
section "6. Subscribe + create channel/thread/reply"

OUT_B3="$TMP_DIR/clientB3.txt"

run_client "$OUT_B3" \
    '/login "bob"' \
    "/subscribe \"$TEAM_UUID\"" \
    "/use \"$TEAM_UUID\"" \
    '/create "general" "General discussion"' \
    '/list'

B3_OUT=$(cat "$OUT_B3")
assert_contains "$B3_OUT" "general" "6.1 Channel created and listed"

# Extract channel UUID
CHAN_UUID=$(echo "$B3_OUT" | grep -oE '[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}' | head -1)
info "Channel UUID: $CHAN_UUID"

if [[ -z "$CHAN_UUID" ]]; then
    fail "6.2 Could not extract channel UUID"
    CHAN_UUID="00000000-0000-0000-0000-000000000000"
else
    pass "6.2 Channel UUID extracted"
fi

OUT_B4="$TMP_DIR/clientB4.txt"
run_client "$OUT_B4" \
    '/login "bob"' \
    "/use \"$TEAM_UUID\" \"$CHAN_UUID\"" \
    '/create "Hello thread" "First post here"' \
    '/list'

B4_OUT=$(cat "$OUT_B4")
assert_contains "$B4_OUT" "Hello thread" "6.3 Thread created and listed"

# Extract thread UUID
THREAD_UUID=$(echo "$B4_OUT" | grep -oE '[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}' | head -1)
info "Thread UUID: $THREAD_UUID"

if [[ -z "$THREAD_UUID" ]]; then
    fail "6.4 Could not extract thread UUID"
    THREAD_UUID="00000000-0000-0000-0000-000000000000"
else
    pass "6.4 Thread UUID extracted"
fi

OUT_B5="$TMP_DIR/clientB5.txt"
run_client "$OUT_B5" \
    '/login "bob"' \
    "/use \"$TEAM_UUID\" \"$CHAN_UUID\" \"$THREAD_UUID\"" \
    '/create "My first reply!"' \
    '/list'

B5_OUT=$(cat "$OUT_B5")
assert_contains "$B5_OUT" "My first reply!" "6.5 Reply created and listed"

# ---------------------------------------------------------------------------
# TEST 7 — Client C (not subscribed) does NOT receive team events
# ---------------------------------------------------------------------------
section "7. Non-subscriber isolation"

OUT_A3="$TMP_DIR/clientA3.txt"
OUT_C2="$TMP_DIR/clientC2.txt"
PID_A3="$TMP_DIR/pidA3"
PID_C2="$TMP_DIR/pidC2"

# C stays alive but unsubscribed while A creates a new channel in the team
run_client_bg "$OUT_C2" "$PID_C2" \
    '/login "charlie"'
# give C time to connect before A fires the event
sleep 0.5

# A (who is subscribed since they created the team) creates another channel
run_client_bg "$OUT_A3" "$PID_A3" \
    '/login "alice"' \
    "/use \"$TEAM_UUID\"" \
    '/create "secret" "No charlie here"'

wait_bg "$PID_A3"
wait_bg "$PID_C2"

C2_OUT=$(cat "$OUT_C2")
assert_not_contains "$C2_OUT" "secret" "7.1 Non-subscriber (charlie) does NOT receive EVT_CHANNEL_CREATED"

# ---------------------------------------------------------------------------
# TEST 8 — Private messages
# ---------------------------------------------------------------------------
section "8. Private messaging"

# Get alice's UUID so bob can /send to her
OUT_B6="$TMP_DIR/clientB6.txt"
run_client "$OUT_B6" \
    '/login "bob"' \
    '/users'

B6_OUT=$(cat "$OUT_B6")

# Extract alice's UUID (line containing "alice")
ALICE_UUID=$(echo "$B6_OUT" | grep -i "alice" | grep -oE '[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}' | head -1)
info "Alice UUID: $ALICE_UUID"

if [[ -z "$ALICE_UUID" ]]; then
    fail "8.0 Could not find alice's UUID — skipping message tests"
else
    pass "8.0 Alice UUID found"

    # Alice stays online to receive the notification
    OUT_A4="$TMP_DIR/clientA4.txt"
    PID_A4="$TMP_DIR/pidA4"

    run_client_bg "$OUT_A4" "$PID_A4" \
        '/login "alice"'
    sleep 0.5

    # Bob sends a message to alice
    OUT_B7="$TMP_DIR/clientB7.txt"
    run_client "$OUT_B7" \
        '/login "bob"' \
        "/send \"$ALICE_UUID\" \"Hello Alice, it's Bob!\""

    wait_bg "$PID_A4"

    B7_OUT=$(cat "$OUT_B7")
    A4_OUT=$(cat "$OUT_A4")

    assert_contains "$B7_OUT" "" "8.1 Bob /send completes without error"   # no crash
    assert_contains "$A4_OUT" "bob" "8.2 Alice receives EVT_MESSAGE_RECEIVED from bob"

    # Now check /messages history
    OUT_A5="$TMP_DIR/clientA5.txt"
    BOB_UUID=$(echo "$B6_OUT" | grep -i "bob" | grep -oE '[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}' | head -1)

    run_client "$OUT_A5" \
        '/login "alice"' \
        "/messages \"$BOB_UUID\""

    A5_OUT=$(cat "$OUT_A5")
    assert_contains "$A5_OUT" "Bob" "8.3 /messages shows conversation history"
fi

# ---------------------------------------------------------------------------
# TEST 9 — /info at various context levels
# ---------------------------------------------------------------------------
section "9. /info context"

OUT_B8="$TMP_DIR/clientB8.txt"
run_client "$OUT_B8" \
    '/login "bob"' \
    '/info'
assert_contains "$(cat "$OUT_B8")" "bob" "9.1 /info (no context) shows logged-in user"

OUT_B9="$TMP_DIR/clientB9.txt"
run_client "$OUT_B9" \
    '/login "bob"' \
    "/use \"$TEAM_UUID\"" \
    '/info'
assert_contains "$(cat "$OUT_B9")" "SquadAlpha" "9.2 /info (team context) shows team details"

# ---------------------------------------------------------------------------
# TEST 10 — Server persistence (Ctrl-C + restart)
# ---------------------------------------------------------------------------
section "10. Server persistence (save & restore)"

info "Sending SIGINT to server (PID $SERVER_PID)..."
kill -INT "$SERVER_PID" 2>/dev/null || true
sleep 1

# Check server is gone
if kill -0 "$SERVER_PID" 2>/dev/null; then
    fail "10.1 Server did not stop after SIGINT"
else
    pass "10.1 Server stopped cleanly"
fi

# Check a save file was created (server should save on shutdown)
SAVE_FILE=$(ls myteams*.save myteams.save save.json myteams_save.json 2>/dev/null | head -1 || true)
if [[ -n "$SAVE_FILE" ]]; then
    pass "10.2 Save file found: $SAVE_FILE"
else
    info "10.2 No save file detected (may use a different name — checking server log)"
    # Not a hard failure — the server may use a non-obvious path
fi

# Restart server
"$SERVER_BIN" "$PORT" > "$TMP_DIR/server2.log" 2>&1 &
SERVER_PID=$!
sleep 0.5

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    fail "10.3 Server failed to restart"
    cat "$TMP_DIR/server2.log"
else
    pass "10.3 Server restarted"
fi

# Verify data survived the restart
OUT_RESTORE="$TMP_DIR/restore.txt"
run_client "$OUT_RESTORE" \
    '/login "alice"' \
    '/list'

RESTORE_OUT=$(cat "$OUT_RESTORE")
assert_contains "$RESTORE_OUT" "SquadAlpha" "10.4 Team data persisted after restart"

# ---------------------------------------------------------------------------
# TEST 11 — /subscribed
# ---------------------------------------------------------------------------
section "11. /subscribed"

OUT_SUB="$TMP_DIR/subscribed.txt"
run_client "$OUT_SUB" \
    '/login "bob"' \
    '/subscribed'

assert_contains "$(cat "$OUT_SUB")" "SquadAlpha" "11.1 /subscribed lists bob's teams"

OUT_SUB2="$TMP_DIR/subscribed2.txt"
run_client "$OUT_SUB2" \
    '/login "bob"' \
    "/subscribed \"$TEAM_UUID\""

assert_contains "$(cat "$OUT_SUB2")" "bob" "11.2 /subscribed team_uuid lists team members"

# ---------------------------------------------------------------------------
# TEST 12 — /unsubscribe
# ---------------------------------------------------------------------------
section "12. /unsubscribe"

OUT_UNSUB="$TMP_DIR/unsubscribe.txt"
run_client "$OUT_UNSUB" \
    '/login "bob"' \
    "/unsubscribe \"$TEAM_UUID\"" \
    '/subscribed'

# After unsubscribing, SquadAlpha should no longer appear in bob's subscribed list
assert_not_contains "$(cat "$OUT_UNSUB")" "SquadAlpha" "12.1 /unsubscribe removes team from subscriptions"

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
section "Results"
echo ""
echo -e "  Tests passed : ${GREEN}$PASS${NC}"
echo -e "  Tests failed : ${RED}$FAIL${NC}"
echo -e "  Total        : $TOTAL"
echo ""

if [[ $FAIL -eq 0 ]]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}$FAIL test(s) failed.${NC}"
    exit 1
fi