# Shared Protocol

Common message format definitions shared by all CPUs:
- `Master CPU`
- `Input CPU`
- `Display CPU`

Keep protocol versioned and backward-compatible.

Current `MSG_STATUS_SNAPSHOT` payload (`kStatusPayloadLen = 18`):
- `speed_mph`
- `soc_pct`
- `power_used_pct` (signed)
- `power_asked_pct`
- `indicator_bits`
- `light_bits`
- `wiper_mode`
- `gear`
- `trip_total_tenths_mi` (u16)
- `trip_since_charge_tenths_mi` (u16)
- `input_online`
- `display_online_from_master`
- `ready`
- `last_command`
- `input_mask` (u16)

## Arduino Build Note

Arduino sketches cannot include headers outside their sketch folder tree.

Canonical source:
- `shared/protocol/protocol_v1.h`

Build copies used by sketches:
- `firmware/master_cpu/protocol_v1.h`
- `firmware/input_cpu/protocol_v1.h`
- `firmware/display_cpu/protocol_v1.h`

When protocol changes, update the canonical file and copy it into each firmware folder.
