#!/usr/bin/env bash
# Post-CubeMX patches for Driver/StmCubeIDE (run from repo root or any cwd).
# Windows: Project/Pre-Compile-cubemx-steps.ps1
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AVIORA_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

MAIN_H="${AVIORA_ROOT}/Driver/StmCubeIDE/Inc/main.h"
SRC_DIR="${AVIORA_ROOT}/Driver/StmCubeIDE/Src"
MAIN_C="${SRC_DIR}/main.c"
MAIN_STM="${SRC_DIR}/main_stm.c"
FREERTOS_SRC="${AVIORA_ROOT}/Middleware/MiddComm/Midd_OS/inc/FreeRTOSConfig_notInclude"
FREERTOS_DST="${AVIORA_ROOT}/Driver/StmCubeIDE/Inc/FreeRTOSConfig.h"

INCLUDE_LINE='#include "McuSmallHelperFunc.h"'
USER_INCLUDES_MARK='/* USER CODE BEGIN Includes */'

die() {
  echo "pre-compile-cubemx-steps: $*" >&2
  exit 1
}

# 1) Add McuSmallHelperFunc.h under USER CODE BEGIN Includes
patch_main_h() {
  [ -f "${MAIN_H}" ] || die "missing ${MAIN_H}"

  if grep -qF "${INCLUDE_LINE}" "${MAIN_H}"; then
    echo "main.h: ${INCLUDE_LINE} already present"
    return
  fi

  if ! grep -qF "${USER_INCLUDES_MARK}" "${MAIN_H}"; then
    die "main.h: marker not found: ${USER_INCLUDES_MARK}"
  fi

  awk -v inc="${INCLUDE_LINE}" -v mark="${USER_INCLUDES_MARK}" '
    $0 == mark && !done { print; print inc; done = 1; next }
    { print }
  ' "${MAIN_H}" > "${MAIN_H}.tmp" && mv "${MAIN_H}.tmp" "${MAIN_H}"
  echo "main.h: added ${INCLUDE_LINE}"
}

# 2) Rename main.c -> main_stm.c when CubeMX regenerated main.c
rename_main_c() {
  if [ ! -f "${MAIN_C}" ]; then
    if [ -f "${MAIN_STM}" ]; then
      echo "Src: main.c absent, main_stm.c ok"
    else
      die "missing ${MAIN_C} and ${MAIN_STM}"
    fi
    return
  fi

  if [ -f "${MAIN_STM}" ]; then
    die "both ${MAIN_C} and ${MAIN_STM} exist; resolve manually"
  fi

  mv "${MAIN_C}" "${MAIN_STM}"
  echo "Src: renamed main.c -> main_stm.c"
}

# 3–4) Disable CubeMX main and HAL tick callback (Aviora provides its own)
patch_main_stm_c() {
  [ -f "${MAIN_STM}" ] || die "missing ${MAIN_STM}"

  if grep -q 'HAL_TIM_PeriodElapsedCallback_notused' "${MAIN_STM}"; then
    echo "main_stm.c: HAL_TIM_PeriodElapsedCallback already patched"
  else
    sed -i 's/\bHAL_TIM_PeriodElapsedCallback\b/HAL_TIM_PeriodElapsedCallback_notused/g' "${MAIN_STM}"
    echo "main_stm.c: renamed HAL_TIM_PeriodElapsedCallback"
  fi

  if grep -q 'int main_notused(void)' "${MAIN_STM}"; then
    echo "main_stm.c: main already patched"
  else
    sed -i 's/int main(void)/int main_notused(void)/g' "${MAIN_STM}"
    if ! grep -q 'int main_notused(void)' "${MAIN_STM}"; then
      die "main_stm.c: int main(void) not found"
    fi
    echo "main_stm.c: renamed int main(void)"
  fi
}

# 5) Replace CubeMX FreeRTOSConfig.h with Aviora canonical copy
install_freertos_config() {
  [ -f "${FREERTOS_SRC}" ] || die "missing ${FREERTOS_SRC}"

  if [ -f "${FREERTOS_DST}" ] && cmp -s "${FREERTOS_SRC}" "${FREERTOS_DST}"; then
    echo "FreeRTOSConfig.h: already matches FreeRTOSConfig_notInclude"
    return
  fi

  rm -f "${FREERTOS_DST}"
  cp -f "${FREERTOS_SRC}" "${FREERTOS_DST}"
  echo "FreeRTOSConfig.h: copied from FreeRTOSConfig_notInclude"
}

main() {
  patch_main_h
  rename_main_c
  patch_main_stm_c
  install_freertos_config
  echo "pre-compile-cubemx-steps: done"
}

main "$@"
