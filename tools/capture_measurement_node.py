#!/usr/bin/env python3
"""Capture a controlled 30-minute raw HX711 run from the WROOM node."""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
import re
import statistics
import sys

try:
    import serial
except ImportError:
    print(
        "pyserial is required. Install with: python -m pip install pyserial",
        file=sys.stderr,
    )
    raise SystemExit(2)


BAUD_RATE = 115200
CONNECT_SAMPLE_COUNT = 20
RUN_DURATION_MS = 30 * 60 * 1000
UINT32_MASK = 0xFFFFFFFF

CSV_HEADER = [
    "session_id",
    "load_cell_id",
    "hx711_id",
    "cassette_id",
    "gpio_dt",
    "gpio_sck",
    "applied_mass_g",
    "sample_kind",
    "sample_index",
    "target_elapsed_ms",
    "actual_elapsed_ms",
    "device_millis",
    "raw_count",
    "reference_raw",
    "delta_count",
    "host_timestamp_utc",
]


@dataclass(frozen=True)
class ChannelChoice:
    key: str
    firmware_side: str
    dt_pin: int
    sck_pin: int


@dataclass(frozen=True)
class NodeSample:
    device_millis: int
    raw_count: int
    host_timestamp_utc: str


CHANNELS = {
    "A": ChannelChoice("A", "LEFT", 4, 5),
    "B": ChannelChoice("B", "RIGHT", 6, 7),
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Capture an identified WROOM/HX711/load-cell combination at "
            "0 s, 10 s, 30 s and every 30 s through 30 minutes."
        )
    )
    parser.add_argument("port", help="Serial port, for example COM3")
    parser.add_argument(
        "--baud",
        type=int,
        default=BAUD_RATE,
        help=f"Serial baud rate (default: {BAUD_RATE})",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Optional output CSV path",
    )
    return parser.parse_args()


def required_input(prompt: str) -> str:
    while True:
        value = input(prompt).strip()
        if value:
            return value
        print("A value is required.")


def cassette_input() -> str:
    value = input("Cassette number [NONE for bench test]: ").strip()
    if not value or value.casefold() in {"none", "no", "n/a", "na"}:
        return "NONE"
    return value


def channel_input() -> ChannelChoice:
    print("WROOM GPIO set:")
    print("  A - DT GPIO4 / SCK GPIO5 (default)")
    print("  B - DT GPIO6 / SCK GPIO7")

    while True:
        value = input("GPIO set [A]: ").strip().upper() or "A"
        if value in CHANNELS:
            return CHANNELS[value]
        print("Enter A or B.")


def mass_input() -> str:
    while True:
        value = input(
            "Applied test mass in grams [0 for no added load]: "
        ).strip() or "0"
        try:
            mass = float(value.replace(",", "."))
        except ValueError:
            print("Enter a number, for example 0, 21 or 999.8.")
            continue

        if mass < 0:
            print("Mass cannot be negative.")
            continue

        return f"{mass:.3f}"


def safe_name(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_-]+", "-", value.strip())
    return cleaned.strip("-_") or "UNKNOWN"


def default_output(
    load_cell_id: str,
    hx711_id: str,
    cassette_id: str,
    channel: ChannelChoice,
    stamp: str,
) -> Path:
    return (
        Path("calibration")
        / "diagnostics"
        / (
            f"wroom_LC-{safe_name(load_cell_id)}_"
            f"HX-{safe_name(hx711_id)}_"
            f"CAS-{safe_name(cassette_id)}_"
            f"GPIO{channel.dt_pin}-{channel.sck_pin}_{stamp}.csv"
        )
    )


def timed_targets_ms() -> list[int]:
    return [0, 10_000, *range(30_000, RUN_DURATION_MS + 1, 30_000)]


def elapsed_ms(current: int, start: int) -> int:
    return (current - start) & UINT32_MASK


def parse_node_sample(line: str, channel: ChannelChoice) -> NodeSample | None:
    if not line.startswith("AL_NODE,DATA,"):
        return None

    parts = line.split(",")
    if len(parts) != 6 or parts[3] != channel.firmware_side:
        return None

    try:
        device_millis = int(parts[2])
        raw_count = int(parts[4])
    except ValueError:
        return None

    return NodeSample(
        device_millis=device_millis,
        raw_count=raw_count,
        host_timestamp_utc=datetime.now(timezone.utc).isoformat(
            timespec="milliseconds"
        ),
    )


def write_row(
    writer: csv.writer,
    metadata: list[str | int],
    sample_kind: str,
    sample_index: int,
    target_ms: int,
    actual_ms: int,
    sample: NodeSample,
    reference_raw: int,
) -> None:
    writer.writerow(
        [
            *metadata,
            sample_kind,
            sample_index,
            target_ms,
            actual_ms,
            sample.device_millis,
            sample.raw_count,
            reference_raw,
            sample.raw_count - reference_raw,
            sample.host_timestamp_utc,
        ]
    )


def open_device(port: str, baud: int) -> serial.Serial:
    device = serial.Serial()
    device.port = port
    device.baudrate = baud
    device.timeout = 1.0
    device.dtr = False
    device.rts = False
    device.open()
    device.reset_input_buffer()
    return device


def main() -> int:
    args = parse_args()

    print("ArrowLab WROOM raw hardware logger")
    print("Enter the identity physically marked on each component.")
    load_cell_id = required_input("Load-cell number: ")
    hx711_id = required_input("HX711 number: ")
    cassette_id = cassette_input()
    channel = channel_input()
    applied_mass_g = mass_input()

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    session_id = f"WROOM-{stamp}"
    output = args.output or default_output(
        load_cell_id,
        hx711_id,
        cassette_id,
        channel,
        stamp,
    )

    print()
    print("Test configuration:")
    print(f"  Load cell : {load_cell_id}")
    print(f"  HX711     : {hx711_id}")
    print(f"  Cassette  : {cassette_id}")
    print(f"  GPIO      : DT {channel.dt_pin} / SCK {channel.sck_pin}")
    print(f"  Test mass : {applied_mass_g} g")
    print(f"  CSV       : {output}")
    print()
    print("Connect the identified assembly to the stated GPIO set.")
    print("Arrange the load and leave the setup completely untouched.")
    input("Press Enter to connect and begin recording automatically...")

    output.parent.mkdir(parents=True, exist_ok=True)
    targets = timed_targets_ms()
    metadata: list[str | int] = [
        session_id,
        load_cell_id,
        hx711_id,
        cassette_id,
        channel.dt_pin,
        channel.sck_pin,
        applied_mass_g,
    ]

    device: serial.Serial | None = None
    csv_file = None
    row_count = 0

    try:
        print(f"Opening {args.port} @ {args.baud}...")
        device = open_device(args.port, args.baud)
        print(
            "Serial connected. Waiting for the first valid HX711 reading "
            f"on GPIO{channel.dt_pin}/{channel.sck_pin}..."
        )

        csv_file = output.open("w", newline="", encoding="utf-8")
        writer = csv.writer(csv_file)
        writer.writerow(CSV_HEADER)
        csv_file.flush()

        connect_samples: list[NodeSample] = []
        first_sample: NodeSample | None = None

        while len(connect_samples) < CONNECT_SAMPLE_COUNT:
            raw_line = device.readline()
            if not raw_line:
                continue

            line = raw_line.decode("utf-8", errors="replace").strip()
            sample = parse_node_sample(line, channel)
            if sample is None:
                continue

            if first_sample is None:
                first_sample = sample
                print("HX711 detected. The 30-minute clock is running.")

            connect_samples.append(sample)

        assert first_sample is not None
        reference_raw = round(
            statistics.fmean(sample.raw_count for sample in connect_samples)
        )

        for index, sample in enumerate(connect_samples, start=1):
            write_row(
                writer,
                metadata,
                "CONNECT",
                index,
                0,
                elapsed_ms(sample.device_millis, first_sample.device_millis),
                sample,
                reference_raw,
            )
            row_count += 1

        write_row(
            writer,
            metadata,
            "TIMED",
            0,
            0,
            0,
            first_sample,
            reference_raw,
        )
        row_count += 1
        csv_file.flush()

        connect_range = (
            max(sample.raw_count for sample in connect_samples)
            - min(sample.raw_count for sample in connect_samples)
        )
        print(
            f"Connection burst saved: {CONNECT_SAMPLE_COUNT} readings, "
            f"mean={reference_raw}, range={connect_range} counts."
        )
        print(
            f"TIMED 00/61  target=   0.0s  raw={first_sample.raw_count}  "
            f"delta={first_sample.raw_count - reference_raw:+d}"
        )

        target_index = 1
        while target_index < len(targets):
            raw_line = device.readline()
            if not raw_line:
                continue

            line = raw_line.decode("utf-8", errors="replace").strip()
            sample = parse_node_sample(line, channel)
            if sample is None:
                continue

            actual = elapsed_ms(
                sample.device_millis,
                first_sample.device_millis,
            )
            target = targets[target_index]
            if actual < target:
                continue

            write_row(
                writer,
                metadata,
                "TIMED",
                target_index,
                target,
                actual,
                sample,
                reference_raw,
            )
            row_count += 1
            csv_file.flush()

            print(
                f"TIMED {target_index:02d}/61  "
                f"target={target / 1000:6.1f}s  "
                f"actual={actual / 1000:6.1f}s  "
                f"raw={sample.raw_count}  "
                f"delta={sample.raw_count - reference_raw:+d}"
            )
            target_index += 1

        print()
        print(f"Run complete. {row_count} rows saved.")
        print(f"CSV: {output}")
        return 0

    except KeyboardInterrupt:
        print()
        print(f"Capture stopped by user. {row_count} rows retained.")
        print(f"Partial CSV: {output}")
        return 130
    except serial.SerialException as exc:
        print(f"Serial error: {exc}", file=sys.stderr)
        print(f"Partial CSV retained: {output}", file=sys.stderr)
        return 1
    finally:
        if csv_file is not None:
            csv_file.flush()
            csv_file.close()
        if device is not None and device.is_open:
            device.close()


if __name__ == "__main__":
    raise SystemExit(main())
