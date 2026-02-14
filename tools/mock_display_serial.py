#!/usr/bin/env python3
"""
Simple PC mock display for Ellert firmware serial diagnostics.

Expected firmware line format:
DIAG|D|v0,v1,...,v65|A|a0,a1,...,a11
"""

import argparse
import sys
import time

try:
    import serial  # type: ignore
except Exception:
    print("Missing dependency: pyserial")
    print("Install with: pip install pyserial")
    sys.exit(1)


def parse_diag(line: str):
    parts = line.strip().split("|")
    if len(parts) != 5:
        return None
    if parts[0] != "DIAG" or parts[1] != "D" or parts[3] != "A":
        return None

    try:
        dvals = [int(x) for x in parts[2].split(",") if x != ""]
        avals = [int(x) for x in parts[4].split(",") if x != ""]
    except ValueError:
        return None

    if len(dvals) != 66 or len(avals) != 12:
        return None
    return dvals, avals


def clear():
    sys.stdout.write("\033[2J\033[H")
    sys.stdout.flush()


def render(dvals, avals, port):
    clear()
    print(f"Ellert Mock Display (Serial)  port={port}")
    print(time.strftime("Updated: %Y-%m-%d %H:%M:%S"))
    print()
    print("Digital Pins (D0..D65)")
    rows = 11
    cols = 6
    for r in range(rows):
        line = []
        for c in range(cols):
            pin = c * rows + r
            if pin > 65:
                continue
            state = "H" if dvals[pin] else "L"
            line.append(f"D{pin:02d}:{state}")
        print("  ".join(line))

    print()
    print("Analog Pins (A0..A11)")
    for i, val in enumerate(avals):
        print(f"A{i:02d} (D{54+i:02d}): {val:4d}")

    print()
    print("Ctrl+C to quit")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=1.0)
    args = parser.parse_args()

    print(f"Opening {args.port} @ {args.baud}...")
    ser = serial.Serial(args.port, args.baud, timeout=args.timeout)
    print("Connected. Waiting for DIAG frames... (Ctrl+C to quit)")
    last_status = time.time()
    frames = 0
    try:
        while True:
            raw = ser.readline()
            if not raw:
                now = time.time()
                if now - last_status > 2.0:
                    print("Waiting for data...")
                    last_status = now
                continue
            line = raw.decode(errors="ignore")
            parsed = parse_diag(line)
            if parsed is None:
                continue
            dvals, avals = parsed
            render(dvals, avals, args.port)
            frames += 1
            last_status = time.time()
    except KeyboardInterrupt:
        print("\nExiting.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
