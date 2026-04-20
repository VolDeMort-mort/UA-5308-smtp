#!/bin/bash

SMTP_HOST="localhost"
SMTP_PORT="25000"
IMAP_HOST="localhost"
IMAP_PORT="2553"
CLIENT="../build/mua/smtp_mua_console"
QUIET=0
COUNT=10

while [[ $# -gt 0 ]]; do
    case $1 in
        -q) QUIET=1 ;;
        -n) COUNT=$2; shift ;;
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
    echo "$1" | $CLIENT | grep -E "\[OK\]|\[ERR\]|\[FAIL\]|id=|Mails fetched"
}

now_ms() {
    echo $(($(date +%s%3N)))
}

RESULTS_DIR=$(mktemp -d)
SEND_TIMES="$RESULTS_DIR/send_times.txt"
FETCH_TIMES="$RESULTS_DIR/fetch_times.txt"
DELETE_TIMES="$RESULTS_DIR/delete_times.txt"
ERRORS="$RESULTS_DIR/errors.txt"

touch "$SEND_TIMES" "$FETCH_TIMES" "$DELETE_TIMES" "$ERRORS"

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
        printf "  %-10s count=%-5d avg=%-6dms min=%-6dms max=%-6dms p95=%dms\n", label, count, avg, min, max, p95
    }' "$file"
}

# ─── Phase 1: All users send simultaneously ───────────────────────────────────

log ""
log "=== Phase 1: $COUNT users sending emails simultaneously ==="
PHASE1_START=$(now_ms)

for i in $(seq 1 $COUNT); do
    (
        next=$(( i % COUNT + 1 ))
        sender="stress_user$i"
        receiver="stress_user$next"
        subject="Stress mail from $i to $next"

        start=$(now_ms)
        OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT $sender pass
send $sender $receiver \"$subject\" \"Hello from $sender\"
quit")
        end=$(now_ms)
        duration=$(( (end - start) ))

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
PHASE1_TOTAL=$(( (PHASE1_END - PHASE1_START) ))
log "--- Phase 1 done in ${PHASE1_TOTAL}ms ---"

# ─── Phase 2: All users fetch and read in one connection ──────────────────────

log ""
log "=== Phase 2: $COUNT users fetching and reading simultaneously ==="
PHASE2_START=$(now_ms)

for i in $(seq 1 $COUNT); do
    (
        user="stress_user$i"
        prev=$(( (i - 2 + COUNT) % COUNT + 1 ))
        expected_subject="Stress mail from $prev to $i"

        start=$(now_ms)
        FETCH_OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT $user pass
fetch INBOX
quit")
        end=$(now_ms)
        duration=$(( (end - start) ))

        if ! echo "$FETCH_OUTPUT" | grep -q "Mails fetched"; then
            echo "fetch $user FAIL: $FETCH_OUTPUT" >> "$ERRORS"
            log "  reader $i: FAIL fetch (${duration}ms)"
            exit 0
        fi

        echo "$duration" >> "$FETCH_TIMES"

        SEQ_ID=$(echo "$FETCH_OUTPUT" | grep "$expected_subject" | grep -o 'id=[0-9]*' | head -1 | cut -d= -f2)

        if [ -z "$SEQ_ID" ]; then
            echo "fetchmail $user FAIL: expected '$expected_subject' not found" >> "$ERRORS"
            log "  reader $i: FAIL (expected '$expected_subject' not found)"
            exit 0
        fi

        READ_OUTPUT=$(run_client "connect $SMTP_HOST $SMTP_PORT $IMAP_HOST $IMAP_PORT $user pass
            fetchmail $SEQ_ID
            quit")

        if echo "$READ_OUTPUT" | grep -q "\[OK\]"; then
            log "  reader $i: PASS fetch+read (${duration}ms, seqId=$SEQ_ID)"
        else
            echo "fetchmail $user FAIL: $READ_OUTPUT" >> "$ERRORS"
            log "  reader $i: FAIL fetchmail (${duration}ms)"
        fi
    ) &
done

wait
PHASE2_END=$(now_ms)
PHASE2_TOTAL=$(( (PHASE2_END - PHASE2_START) ))
log "--- Phase 2 done in ${PHASE2_TOTAL}ms ---"

# ─── Phase 3: All users delete their stress mail simultaneously ───────────────

log ""
log "=== Phase 3: $COUNT users deleting emails simultaneously ==="
PHASE3_START=$(now_ms)

for i in $(seq 1 $COUNT); do
    (
        user="stress_user$i"
        prev=$(( (i - 2 + COUNT) % COUNT + 1 ))
        expected_subject="Stress mail from $prev to $i"

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
        duration=$(( (end - start) ))

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
PHASE3_TOTAL=$(( (PHASE3_END - PHASE3_START) ))
log "--- Phase 3 done in ${PHASE3_TOTAL}ms ---"

# ─── Results ──────────────────────────────────────────────────────────────────

SEND_OK=$(wc -l < "$SEND_TIMES")
FETCH_OK=$(wc -l < "$FETCH_TIMES")
DELETE_OK=$(wc -l < "$DELETE_TIMES")
ERROR_COUNT=$(wc -l < "$ERRORS")

TOTAL_OPS=$(( COUNT * 3 ))
TOTAL_OK=$(( SEND_OK + FETCH_OK + DELETE_OK ))

echo ""
echo "================================"
echo "STRESS TEST RESULTS ($COUNT users)"
echo "================================"
echo "Operations:  $TOTAL_OK/$TOTAL_OPS succeeded"
echo "Errors:      $ERROR_COUNT"
echo ""
echo "Timing:"
calc_stats "$SEND_TIMES"   "send"
calc_stats "$FETCH_TIMES"  "fetch+read"
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