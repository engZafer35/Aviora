"""
STM32 CubeIDE ön derleme düzeni: HAL/CMSIS ve Core kaynaklarını
Aviora kökündeki Driver/StmCubeIDE altına taşır; boşalan Project/.../Drivers dizinini siler;
Startup'ı CubeIDE proje köküne alır.
"""
from __future__ import annotations

import shutil
import sys
from pathlib import Path


def _repo_root(script_dir: Path) -> Path:
    """precompile_setup.py: .../Aviora/Customers/Stm407Eva -> Aviora kökü."""
    return script_dir.parent.parent


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    repo_root = _repo_root(script_dir)

    driver_dest = repo_root / "Driver" / "StmCubeIDE"
    project_root = repo_root / "Project" / "StmCubeIde_Stm407Eva"

    drivers_src = project_root / "Drivers"
    core_src = project_root / "Core"

    if driver_dest.exists():
        print(f"\033[93m- !! WARNING: {driver_dest} exists and will be deleted!!\033[0m")
        shutil.rmtree(driver_dest)

    driver_dest.mkdir(parents=True, exist_ok=False)
    print(f"\033[92m- Driver/StmCubeIDE created\033[0m")

    moves_to_driver: list[Path] = [
        drivers_src / "CMSIS",
        drivers_src / "STM32F4xx_HAL_Driver",
        core_src / "Inc",
        core_src / "Src",
    ]

    for src in moves_to_driver:
        if not src.exists():
            print(f"\033[91m- !!ERROR: {src} does not exist!!\033[0m")
            return 1
        dest = driver_dest / src.name

        print(f"\033[94m- Moving: {src} -> {dest}\033[0m")        
        shutil.move(str(src), str(dest))

    if drivers_src.exists():
        try:
            drivers_src.rmdir()
            print(f"\033[92m- {drivers_src} deleted\033[0m")
        except OSError as e:
            print(
                f"\033[91m- !!ERROR: {drivers_src} could not be deleted: {e}\033[0m",
                file=sys.stderr,
            )

    startup_src = core_src / "Startup"
    startup_dest = project_root / "Startup"
    if startup_src.exists():
        if startup_dest.exists():
            print(f"\033[91m- !!ERROR: {startup_dest} already exists and must be deleted!!\033[0m")
            return 1
        else:
            print(f"\033[92m- Moving: {startup_src} -> {startup_dest}\033[0m")
            shutil.move(str(startup_src), str(startup_dest))
            core_src.rmdir()
    else:
        if startup_dest.exists():
            print(f"\033[92m- {startup_dest} already moved\033[0m")
            core_src.rmdir()
        else:
            print(f"\033[91m- !!ERROR: {startup_src} does not exist!!\033[0m")
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
