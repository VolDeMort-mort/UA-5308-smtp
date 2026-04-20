#!/bin/bash

SMTP_HOST="localhost"
SMTP_PORT="25000"
IMAP_HOST="localhost"
IMAP_PORT="2553"
CLIENT="../build/mua/smtp_mua_console"
QUIET=0

if [ "$1" = "-q" ]; then
    QUIET=1
fi

if [ ! -f "$CLIENT" ]; then
    echo "ERROR: client not found at $CLIENT"
    exit 1
fi

run_client() {
    echo "$1" | $CLIENT | grep -E "\[OK\]|\[ERR\]|\[FAIL\]|id=|Mails fetched|Mail body|subject=|from="
}

log() {
    if [ $QUIET -eq 0 ]; then
        echo "$1"
    fi
}

PASS=0
FAIL=0

check() {
    local description=$1
    local output=$2
    local expected=$3

    if echo "$output" | grep -q "$expected"; then
        log "  [PASS] $description"
        PASS=$((PASS + 1))
    else
        log "  [FAIL] $description"
        log "         expected: $expected"
        FAIL=$((FAIL + 1))
    fi
}

# ─── Step 1: Alice sends ──────────────────────────────────────────────────────
log ""
log "=== Step 1: Alice sends email to Bob ==="
SEND_OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT alice pass
send alice bob \"Stress test subject\" \"Hello Bob from stress test\"
quit")
log "$SEND_OUTPUT"
check "Email sent successfully" "$SEND_OUTPUT" "OK"

# ─── Step 2: Bob fetches inbox ────────────────────────────────────────────────
log ""
log "=== Step 2: Bob fetches inbox ==="
FETCH_OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT bob pass
fetch INBOX
quit")
log "$FETCH_OUTPUT"
check "Inbox fetched successfully" "$FETCH_OUTPUT" "Mails fetched"
check "Test email is in inbox"     "$FETCH_OUTPUT" "Stress test subject"

# ─── Step 3: Bob reads the email ─────────────────────────────────────────────
SEQ_ID=$(echo "$FETCH_OUTPUT" | grep "Stress test subject" | grep -o 'id=[0-9]*' | head -1 | cut -d= -f2)
log ""
log "=== Step 3: Bob reads email (seqId=$SEQ_ID) ==="

if [ -n "$SEQ_ID" ]; then
    READ_OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT bob pass
fetch INBOX
fetchmail $SEQ_ID
quit")
    log "$READ_OUTPUT"
    check "Email body correct" "$READ_OUTPUT" "Hello Bob from stress test"
else
    log "  [FAIL] Could not find test email"
    FAIL=$((FAIL + 1))
fi

# ─── Step 4: Bob deletes the email ───────────────────────────────────────────
log ""
log "=== Step 4: Bob deletes test email ==="

if [ -n "$SEQ_ID" ]; then
    DELETE_OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT bob pass
deletemail $SEQ_ID
quit")
    log "$DELETE_OUTPUT"
    check "Email deleted" "$DELETE_OUTPUT" "OK"
else
    log "  [FAIL] Could not delete, no sequence ID"
    FAIL=$((FAIL + 1))
fi

# ─── Results ──────────────────────────────────────────────────────────────────
log ""
log "================================"
TOTAL=$((PASS + FAIL))
if [ $FAIL -eq 0 ]; then
    echo "PASSED ($PASS/$TOTAL checks passed)"
else
    echo "FAILED ($FAIL/$TOTAL checks failed)"
fi
