#!/bin/sh

# Match the vendor S50systemui environment without stopping the existing UI.
[ -r /etc/profile ] && . /etc/profile
export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/run/user/0}
export WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-wayland-0}
export QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-wayland}

i=0
while [ ! -S "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" ] && [ "$i" -lt 50 ]; do
    sleep 0.1
    i=$((i + 1))
done

if [ ! -S "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" ]; then
    echo "Wayland socket not ready: $XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" >&2
    exit 1
fi

exec /userdata/usbcan_gateway/can-status-hmi
