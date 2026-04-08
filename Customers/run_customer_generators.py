#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Run every Customers/generate_*.py script with the same --customer argument.

Usage:
  python Customers/run_customer_generators.py --customer ZD_0101

Scripts are executed in alphabetical order (generate_fs_port_config.py,
generate_logger_service.py, generate_protocol_config.py, generate_time_service_code.py, ...).
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def main() -> int:
    out = sys.stdout
    if hasattr(out, "reconfigure"):
        try:
            out.reconfigure(encoding="utf-8", errors="replace")
        except (OSError, ValueError, AttributeError):
            pass

    parser = argparse.ArgumentParser(
        description=(
            "Run all generate_*.py scripts under Customers/ with a single --customer value."
        ),
        epilog=(
            "Example:\n"
            "  python Customers/run_customer_generators.py --customer ZD_0101\n"
        ),
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument(
        "--customer",
        required=True,
        metavar="NAME",
        help="Passed to each generate_*.py (e.g. ZD_0101, LinuxGcc)",
    )
    args = parser.parse_args()

    customers_dir = Path(__file__).resolve().parent
    scripts = sorted(customers_dir.glob("generate_*.py"))

    if not scripts:
        print("No generate_*.py scripts found in", customers_dir, file=sys.stderr)
        return 1

    failed: list[str] = []
    for script in scripts:
        print(f"=== {script.name} ===", flush=True)
        proc = subprocess.run(
            [sys.executable, str(script), "--customer", args.customer],
            cwd=str(customers_dir),
        )
        if proc.returncode != 0:
            failed.append(script.name)
            print(f"FAILED (exit {proc.returncode}): {script.name}", file=sys.stderr)

    if failed:
        print(
            f"\nCompleted with errors. Failed: {', '.join(failed)}",
            file=sys.stderr,
        )
        return 1

    print(f"\nAll {len(scripts)} generator(s) finished successfully.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
