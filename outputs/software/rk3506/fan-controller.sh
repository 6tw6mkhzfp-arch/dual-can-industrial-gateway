#!/bin/sh

GPIO_NUM=33
GPIO_DIR=/sys/class/gpio/gpio${GPIO_NUM}
GREEN_GPIO_NUM=34
GREEN_GPIO_DIR=/sys/class/gpio/gpio${GREEN_GPIO_NUM}
CAN_LOG=/var/log/usbcan_gateway.log
STATE_FILE=/run/fan-controller.state
EVENT_LOG=/var/log/fan-controller.log

if [ ! -d "$GPIO_DIR" ]; then
    echo "$GPIO_NUM" >/sys/class/gpio/export || exit 1
    sleep 1
fi

if [ ! -d "$GREEN_GPIO_DIR" ]; then
    echo "$GREEN_GPIO_NUM" >/sys/class/gpio/export || exit 1
    sleep 1
fi

# Relay channel 1 is configured for HIGH-level trigger.
echo high >"$GPIO_DIR/direction" || exit 1
echo low >"$GREEN_GPIO_DIR/direction" || exit 1
current=1
green_current=0
echo "ON STARTUP" >"$STATE_FILE"
echo "$(date '+%F %T') fan=ON reason=startup" >>"$EVENT_LOG"

set_fan()
{
    desired=$1
    reason=$2
    [ "$desired" = "$current" ] && return
    echo "$desired" >"$GPIO_DIR/value" || return
    current=$desired
    if [ "$desired" = 1 ]; then
        echo "ON $reason" >"$STATE_FILE"
        echo "$(date '+%F %T') fan=ON reason=$reason" >>"$EVENT_LOG"
    else
        echo "OFF $reason" >"$STATE_FILE"
        echo "$(date '+%F %T') fan=OFF reason=$reason" >>"$EVENT_LOG"
    fi
}

set_green()
{
    desired=$1
    reason=$2
    [ "$desired" = "$green_current" ] && return
    echo "$desired" >"$GREEN_GPIO_DIR/value" || return
    green_current=$desired
    if [ "$desired" = 1 ]; then
        echo "$(date '+%F %T') green=ON reason=$reason" >>"$EVENT_LOG"
    else
        echo "$(date '+%F %T') green=OFF reason=$reason" >>"$EVENT_LOG"
    fi
}

cleanup()
{
    # Deliberately leave the cooling fan off during an orderly shutdown.
    echo 0 >"$GPIO_DIR/value" 2>/dev/null || true
    echo 0 >"$GREEN_GPIO_DIR/value" 2>/dev/null || true
    echo "OFF stopped" >"$STATE_FILE"
    exit 0
}
trap cleanup TERM INT

last_line=
last_seen=0
alarm=0

while :; do
    now=$(date +%s)
    line=$(tail -n 80 "$CAN_LOG" 2>/dev/null |
        grep ' id=200 .* can=[12]' | tail -n 1)
    if [ -n "$line" ] && [ "$line" != "$last_line" ]; then
        last_line=$line
        last_seen=$now
        first_byte=$(echo "$line" |
            sed -n 's/.* data=\([0-9a-fA-F][0-9a-fA-F]\).*/\1/p')
        if [ -n "$first_byte" ] && [ "$first_byte" != 00 ]; then
            alarm=1
        else
            alarm=0
        fi
    fi

    if [ "$last_seen" -eq 0 ] || [ $((now - last_seen)) -gt 3 ]; then
        set_fan 1 can_offline
        set_green 0 can_offline
    elif [ "$alarm" -eq 1 ]; then
        set_fan 1 can_alarm
        set_green 0 can_alarm
    else
        set_fan 0 normal
        set_green 1 normal
    fi
    sleep 1
done

