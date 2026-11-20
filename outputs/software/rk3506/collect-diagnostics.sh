#!/bin/sh
# Read-only diagnostics. No CAN transmit, installation, or interface changes.
# Usage: sh collect-diagnostics.sh > rk-usb-before.txt 2>&1
# Repeat after plugging the USB-CAN into a carrier USB-A port.
section() { printf '\n=== %s ===\n' "$1"; }
section system
uname -a
if [ -r /etc/os-release ]; then cat /etc/os-release; fi
if command -v getconf >/dev/null 2>&1; then
    getconf LONG_BIT
    getconf GNU_LIBC_VERSION
fi
section userspace
if command -v file >/dev/null 2>&1; then
    file -L /bin/sh /bin/busybox
fi
if command -v readelf >/dev/null 2>&1; then
    readelf -h /bin/sh
    readelf -A /bin/sh
    readelf -l /bin/sh
fi
for f in /lib/ld*.so* /lib/libc.so* /lib/libuClibc*; do
    [ -e "$f" ] && ls -l "$f"
done
section usb
if command -v lsusb >/dev/null 2>&1; then
    lsusb
    lsusb -t
fi
for d in /sys/bus/usb/devices/*; do
    [ -r "$d/idVendor" ] || continue
    printf '%s ' "${d##*/}"
    for a in idVendor idProduct manufacturer product; do
        if [ -r "$d/$a" ]; then
            printf '%s=' "$a"
            tr '\n' ' ' < "$d/$a"
            printf ' '
        fi
    done
    printf '\n'
done
section usb_interface_drivers
for d in /sys/bus/usb/devices/*:*; do
    [ -d "$d" ] || continue
    printf '%s ' "${d##*/}"
    if [ -e "$d/driver" ]; then
        readlink -f "$d/driver"
    else
        printf 'driver=none\n'
    fi
done
section serial_devices
for d in /sys/class/tty/ttyUSB* /sys/class/tty/ttyACM*; do
    [ -e "$d" ] || continue
    printf '%s ' "${d##*/}"
    if [ -e "$d/device" ]; then readlink -f "$d/device"; else printf '\n'; fi
done
section network
if command -v ip >/dev/null 2>&1; then
    ip -details -statistics link show
fi
for d in /sys/class/net/*; do
    [ -e "$d" ] || continue
    printf '\ninterface=%s\n' "${d##*/}"
    [ -r "$d/type" ] && cat "$d/type"
    [ -e "$d/device" ] && readlink -f "$d/device"
    [ -e "$d/device/driver" ] && readlink -f "$d/device/driver"
done
section kernel_config
if [ -r /proc/config.gz ] && command -v zcat >/dev/null 2>&1; then
    zcat /proc/config.gz | grep -E 'CONFIG_(CAN|USB_CHIPIDEA|USB_DWC|USB_EHCI|USB_OHCI|USB_XHCI)'
fi
section recent_kernel_messages
dmesg | tail -n 100
section done
printf 'Missing commands/permission errors mean evidence unavailable, not hardware failure.\n'
