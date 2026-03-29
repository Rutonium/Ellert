# Display CPU ESP32-S3 LGFX Lab

Isolated LovyanGFX bring-up target for JC3248W535 (AXS15231B).

Purpose:
- Validate panel + bus configuration without project UI/protocol complexity.
- Run deterministic display gates before enabling LVGL/project features.
- This target is lab-only; runtime display firmware remains `firmware/display_cpu_esp32s3/`.

## Gates

- Gate A (`A_SOLID`): full-screen black/white/red/green/blue sequence.
- Gate B (`B_CHECKER`): static checkerboard + edge markers.
- Gate C (`C_SOAK`): moving stress frame for long-run stability.

Switch gate in sketch:
- `kGateMode` in `display_cpu_esp32s3_lgfx_lab.ino`.

## Compile + Flash

```bash
arduino-cli lib install "LovyanGFX"
./tools/flash_display_lgfx_lab.sh
```

To return immediately to known-good runtime display firmware:

```bash
./tools/flash_display_stable.sh
```
