# Shared Protocol

Common message format definitions shared by all CPUs:
- `Master CPU`
- `Input CPU`
- `Display CPU`

Keep protocol versioned and backward-compatible.

## Arduino Build Note

Arduino sketches cannot include headers outside their sketch folder tree.

Canonical source:
- `shared/protocol/protocol_v1.h`

Build copies used by sketches:
- `firmware/master_cpu/protocol_v1.h`
- `firmware/input_cpu/protocol_v1.h`
- `firmware/display_cpu/protocol_v1.h`

When protocol changes, update the canonical file and copy it into each firmware folder.
