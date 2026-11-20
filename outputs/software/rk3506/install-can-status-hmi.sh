#!/bin/sh
set -eu

BASE=/userdata/usbcan_gateway
BIN="$BASE/can-status-hmi"
RUNNER="$BASE/run-can-status-hmi.sh"
URL=${1:-http://192.168.31.249:8000/can-status-hmi}
RUN_URL=${2:-http://192.168.31.249:8000/run-can-status-hmi.sh}

mkdir -p "$BASE"
wget -O "$BIN.new" "$URL"
chmod 755 "$BIN.new"

if ! "$BIN.new" --self-test >/tmp/can-status-hmi-self-test.log 2>&1; then
    echo "HMI self-test failed; existing binary was not replaced." >&2
    cat /tmp/can-status-hmi-self-test.log >&2 || true
    rm -f "$BIN.new"
    exit 1
fi

wget -O "$RUNNER.new" "$RUN_URL"
chmod 755 "$RUNNER.new"

if [ -f "$BIN" ]; then
    cp "$BIN" "$BIN.bak"
fi
if [ -f "$RUNNER" ]; then
    cp "$RUNNER" "$RUNNER.bak"
fi
mv "$BIN.new" "$BIN"
mv "$RUNNER.new" "$RUNNER"
echo "Installed: $BIN"
echo "Manual test under Weston:"
echo "  sh $RUNNER"
echo "Rollback:"
echo "  test -f $BIN.bak && mv $BIN.bak $BIN"
