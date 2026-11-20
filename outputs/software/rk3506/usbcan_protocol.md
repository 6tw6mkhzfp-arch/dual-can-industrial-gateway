# 0471:1200 USB-CAN protocol observations

Status: experimentally observed from CANTest 2.69 with the dual-channel adapter. This is not vendor documentation. Captures are in `work/usbcan-heartbeat-all.pcap` and `work/usbcan-init-tx.pcap`.

## USB layout

- Interface: 0
- Interrupt OUT `0x01`, max packet 16
- Interrupt IN `0x81`, max packet 16
- Bulk OUT `0x02`, max packet 64
- Bulk IN `0x82`, max packet 64

The captures used device index 0 at 500 kbit/s in normal mode. CAN2 was
captured first; CAN1 initialization was captured separately on 2026-11-18.

## Channel setup observed

CANTest sent the following requests to interrupt endpoint `0x01`; the device acknowledged them on `0x81`:

| Purpose | OUT payload | IN acknowledgement |
|---|---|---|
| Initialize CAN2 at 500 kbit/s | `03 04 00 10 00 1C` | `03 81 00` |
| Acceptance/filter configuration | `04 0A 00 50 00 00 00 00 FF FF FF FF` | `04 81 00` |
| Start CAN2 | `0E 02 00 10` | `0E 81 00` |

The separately captured CAN1 requests were:

| Purpose | OUT payload | IN acknowledgement |
|---|---|---|
| Initialize CAN1 at 500 kbit/s | `03 04 00 00 00 1C` | `03 81 00` |
| CAN1 acceptance/filter configuration | `04 0A 00 40 00 00 00 00 FF FF FF FF` | `04 81 00` |
| Start CAN1 | `0E 02 00 00` | `0E 81 00` |

The `00 1C` timing pair matches the vendor VCI value `0x1C00` for 500 kbit/s.
The channel selector is now capture-verified as `0x00` for CAN1 and `0x10`
for CAN2. The acceptance/filter selector is `0x40` for CAN1 and `0x50` for
CAN2.

CANTest also queried device information with commands `01`, `12`, `13`, `14`, and `15`; those queries are not required to decode received CAN frames.

## CAN frame record

Bulk transfers contain 18-byte records:

| Offset | Size | Meaning |
|---:|---:|---|
| 0 | 4 | Device timestamp, little endian; zero for host transmit |
| 4 | 4 | Packed CAN descriptor, little endian |
| 8 | 2 | Observed as zero; exact meaning unknown |
| 10 | 8 | CAN data, zero/unused bytes depend on DLC |

For the standard data frames tested, the descriptor is:

```text
descriptor = (standard_id << 21) | (dlc << 8) | 0x1000
```

Observed records:

```text
RX heartbeat, ID 0x100, DLC 8:
<timestamp> 00 18 00 20 00 00 <8 data bytes>

TX request, ID 0x123, DLC 8, data 11..88:
00 00 00 00 00 18 60 24 00 00 11 22 33 44 55 66 77 88

RX reply, ID 0x321, DLC 8, data 11..88:
<timestamp> 00 18 20 64 00 00 11 22 33 44 55 66 77 88
```

Extended frames, RTR, CAN error/status records, buffering across multiple records, timestamp units, bus-off recovery, and hot-unplug behavior are not yet decoded. The prototype must therefore reject unknown descriptor flags rather than silently treating them as normal frames.

