#!/bin/bash

SMTP_HOST="localhost"
SMTP_PORT="25000"
IMAP_HOST="localhost"
IMAP_PORT="2553"
CLIENT="../build/mua/smtp_mua_console"
QUIET=0
COUNT=1
ATTACHMENT_SIZE_KB=10
ATTACHMENT_FILE="./test_attachment.bin"

while [[ $# -gt 0 ]]; do
    case $1 in
        -q) QUIET=1 ;;
        -n) COUNT=$2; shift ;;
        -s) ATTACHMENT_SIZE_KB=$2; shift ;;
        *)  COUNT=$1 ;;
    esac
    shift
done

if [ ! -f "$CLIENT" ]; then
    echo "ERROR: client not found at $CLIENT"
    exit 1
fi

log() {
    if [ $QUIET -eq 0 ]; then
        echo "$1"
    fi
}

run_client() {
    echo "$1" | $CLIENT | grep -E "\[OK\]|\[ERR\]|\[FAIL\]|id=|Mails fetched|Attachment saved"
}

now_ms() {
    echo $(($(date +%s%3N)))
}

# file generating

EXPECTED_BYTES=$(( ATTACHMENT_SIZE_KB * 1024 ))
ACTUAL_BYTES=0
if [ -f "$ATTACHMENT_FILE" ]; then
    ACTUAL_BYTES=$(wc -c < "$ATTACHMENT_FILE")
fi

if [ ! -f "$ATTACHMENT_FILE" ] || [ "$ACTUAL_BYTES" -ne "$EXPECTED_BYTES" ]; then
    log "Generating ${ATTACHMENT_SIZE_KB}KB test attachment: $ATTACHMENT_FILE"
    head -c "$EXPECTED_BYTES" /dev/urandom > "$ATTACHMENT_FILE"
    if [ ! -s "$ATTACHMENT_FILE" ]; then
        echo "ERROR: failed to generate attachment file"
        exit 1
    fi
    log "Done."
else
    log "Using existing ${ATTACHMENT_SIZE_KB}KB attachment: $ATTACHMENT_FILE"
fi

# resolve to absolute path
ATTACHMENT_ABS=$(realpath "$ATTACHMENT_FILE")

# ─── Setup results dir ────────────────────────────────────────────────────────

RESULTS_DIR=$(mktemp -d)
SEND_TIMES="$RESULTS_DIR/send_times.txt"
FETCH_TIMES="$RESULTS_DIR/fetch_times.txt"
GETATT_TIMES="$RESULTS_DIR/getatt_times.txt"
DELETE_TIMES="$RESULTS_DIR/delete_times.txt"
ERRORS="$RESULTS_DIR/errors.txt"

touch "$SEND_TIMES" "$FETCH_TIMES" "$GETATT_TIMES" "$DELETE_TIMES" "$ERRORS"

calc_stats() {
    local file=$1
    local label=$2
    if [ ! -s "$file" ]; then
        echo "  $label: no data"
        return
    fi
    awk -v label="$label" '
    BEGIN { min=999999; max=0; sum=0; count=0 }
    {
        val=$1
        times[count]=val
        sum+=val
        count++
        if(val<min) min=val
        if(val>max) max=val
    }
    END {
        avg=sum/count
        n=asort(times)
        p95_idx=int(n*0.95)
        if(p95_idx<1) p95_idx=1
        p95=times[p95_idx]
        printf "  %-12s count=%-5d avg=%-6dms min=%-6dms max=%-6dms p95=%dms\n", label, count, avg, min, max, p95
    }' "$file"
}

# ─── Phase 1: All users send email with attachment simultaneously ──────────────

log ""
log "=== Phase 1: $COUNT users sending emails with ${ATTACHMENT_SIZE_KB}KB attachment ==="
PHASE1_START=$(now_ms)

for i in $(seq 1 $COUNT); do
    (
        next=$(( i % COUNT + 1 ))
        sender="stress_user$i"
        receiver="stress_user$next"
        subject="Attachment mail from $i to $next"

        start=$(now_ms)
        OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT $sender pass
send $sender $receiver \"$subject\" \"Check the file\" \"$ATTACHMENT_ABS\"
quit")
        end=$(now_ms)
        duration=$(( end - start ))

        if echo "$OUTPUT" | grep -q "\[OK\]"; then
            echo "$duration" >> "$SEND_TIMES"
            log "  sender $i: PASS (${duration}ms)"
        else
            echo "send $sender FAIL: $OUTPUT" >> "$ERRORS"
            log "  sender $i: FAIL (${duration}ms)"
        fi
    ) &
done

wait
PHASE1_END=$(now_ms)
PHASE1_TOTAL=$(( PHASE1_END - PHASE1_START ))
log "--- Phase 1 done in ${PHASE1_TOTAL}ms ---"

# ─── Phase 2: All users fetch inbox and download attachment simultaneously ─────

log ""
log "=== Phase 2: $COUNT users fetching and downloading attachment ==="
PHASE2_START=$(now_ms)

for i in $(seq 1 $COUNT); do
    (
        user="stress_user$i"
        prev=$(( (i - 2 + COUNT) % COUNT + 1 ))
        expected_subject="Attachment mail from $prev to $i"

        start=$(now_ms)
        FETCH_OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT $user pass
fetch INBOX
quit")
        end=$(now_ms)
        fetch_duration=$(( end - start ))

        if ! echo "$FETCH_OUTPUT" | grep -q "Mails fetched"; then
            echo "fetch $user FAIL: $FETCH_OUTPUT" >> "$ERRORS"
            log "  reader $i: FAIL fetch (${fetch_duration}ms)"
            exit 0
        fi

        echo "$fetch_duration" >> "$FETCH_TIMES"

        SEQ_ID=$(echo "$FETCH_OUTPUT" | grep "$expected_subject" | grep -o 'id=[0-9]*' | head -1 | cut -d= -f2)

        if [ -z "$SEQ_ID" ]; then
            echo "fetch $user FAIL: expected '$expected_subject' not found" >> "$ERRORS"
            log "  reader $i: FAIL (expected mail not found)"
            exit 0
        fi

        start=$(now_ms)
        GETATT_OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT $user pass
getatt $SEQ_ID 1
quit")
        end=$(now_ms)
        getatt_duration=$(( end - start ))

        if echo "$GETATT_OUTPUT" | grep -q "\[OK\]"; then
            echo "$getatt_duration" >> "$GETATT_TIMES"
            log "  reader $i: PASS fetch+getatt (fetch=${fetch_duration}ms, getatt=${getatt_duration}ms, seqId=$SEQ_ID)"
        else
            echo "getatt $user seqId=$SEQ_ID FAIL: $GETATT_OUTPUT" >> "$ERRORS"
            log "  reader $i: FAIL getatt (${getatt_duration}ms)"
        fi
    ) &
done

wait
PHASE2_END=$(now_ms)
PHASE2_TOTAL=$(( PHASE2_END - PHASE2_START ))
log "--- Phase 2 done in ${PHASE2_TOTAL}ms ---"

# ─── Phase 3: All users delete their mail simultaneously ──────────────────────

log ""
log "=== Phase 3: $COUNT users deleting emails simultaneously ==="
PHASE3_START=$(now_ms)

for i in $(seq 1 $COUNT); do
    (
        user="stress_user$i"
        prev=$(( (i - 2 + COUNT) % COUNT + 1 ))
        expected_subject="Attachment mail from $prev to $i"

        FETCH_OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT $user pass
fetch INBOX
quit")
        SEQ_ID=$(echo "$FETCH_OUTPUT" | grep "$expected_subject" | grep -o 'id=[0-9]*' | head -1 | cut -d= -f2)

        if [ -z "$SEQ_ID" ]; then
            echo "delete $user FAIL: expected '$expected_subject' not found" >> "$ERRORS"
            log "  deleter $i: FAIL (mail not found)"
            exit 0
        fi

        start=$(now_ms)
        DELETE_OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT $user pass
deletemail $SEQ_ID
quit")
        end=$(now_ms)
        duration=$(( end - start ))

        if echo "$DELETE_OUTPUT" | grep -q "\[OK\]"; then
            echo "$duration" >> "$DELETE_TIMES"
            log "  deleter $i: PASS (${duration}ms)"
        else
            echo "delete $user FAIL: $DELETE_OUTPUT" >> "$ERRORS"
            log "  deleter $i: FAIL (${duration}ms)"
        fi
    ) &
done

wait
PHASE3_END=$(now_ms)
PHASE3_TOTAL=$(( PHASE3_END - PHASE3_START ))
log "--- Phase 3 done in ${PHASE3_TOTAL}ms ---"

# ─── Phase 4: Cleanup downloaded attachments ──────────────────────────────────

log ""
log "=== Phase 4: Cleaning up downloaded attachments ==="
if [ -d ".tmp/attachments" ]; then
    rm -rf .tmp/attachments
    log "Removed .tmp/attachments/"
else
    log ".tmp/attachments/ not found, nothing to clean"
fi

# ─── Results ──────────────────────────────────────────────────────────────────

SEND_OK=$(wc -l < "$SEND_TIMES")
FETCH_OK=$(wc -l < "$FETCH_TIMES")
GETATT_OK=$(wc -l < "$GETATT_TIMES")
DELETE_OK=$(wc -l < "$DELETE_TIMES")
ERROR_COUNT=$(wc -l < "$ERRORS")

TOTAL_OPS=$(( COUNT * 4 ))
TOTAL_OK=$(( SEND_OK + FETCH_OK + GETATT_OK + DELETE_OK ))

echo ""
echo "================================"
echo "ATTACHMENT STRESS TEST ($COUNT users, ${ATTACHMENT_SIZE_KB}KB file)"
echo "================================"
echo "Operations:  $TOTAL_OK/$TOTAL_OPS succeeded"
echo "Errors:      $ERROR_COUNT"
echo ""
echo "Timing:"
calc_stats "$SEND_TIMES"   "send"
calc_stats "$FETCH_TIMES"  "fetch"
calc_stats "$GETATT_TIMES" "getatt"
calc_stats "$DELETE_TIMES" "delete"
echo ""
echo "Phase durations:"
echo "  Phase 1 (send):   ${PHASE1_TOTAL}ms"
echo "  Phase 2 (fetch):  ${PHASE2_TOTAL}ms"
echo "  Phase 3 (delete): ${PHASE3_TOTAL}ms"
echo "  Total:            $(( PHASE1_TOTAL + PHASE2_TOTAL + PHASE3_TOTAL ))ms"
echo ""

if [ $ERROR_COUNT -gt 0 ]; then
    echo "Errors:"
    cat "$ERRORS"
    echo ""
    echo "RESULT: FAILED"
else
    echo "RESULT: PASSED"
fi

rm -rf "$RESULTS_DIR"