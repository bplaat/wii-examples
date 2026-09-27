#!/bin/sh
set -eu

if [ "$#" -lt 1 ]; then
    echo "Cargo did not pass an executable to the Dolphin runner" >&2
    exit 1
fi

elf="$1"
shift
if [ ! -f "$elf" ]; then
    echo "Executable not found: $elf" >&2
    exit 1
fi
dol="$elf.dol"
: "${DEVKITPRO:?DEVKITPRO must point to devkitPro}"
"$DEVKITPRO/tools/bin/elf2dol" "$elf" "$dol"
dol_dir=$(CDPATH= cd -- "$(dirname -- "$dol")" && pwd)
dol="$dol_dir/$(basename -- "$dol")"

if [ -n "${DOLPHIN:-}" ]; then
    dolphin=$DOLPHIN
elif command -v dolphin-emu >/dev/null 2>&1; then
    dolphin=$(command -v dolphin-emu)
elif command -v dolphin >/dev/null 2>&1; then
    dolphin=$(command -v dolphin)
elif [ -x /Applications/Dolphin.app/Contents/MacOS/Dolphin ]; then
    dolphin=/Applications/Dolphin.app/Contents/MacOS/Dolphin
elif [ -x "$HOME/Applications/Dolphin.app/Contents/MacOS/Dolphin" ]; then
    dolphin="$HOME/Applications/Dolphin.app/Contents/MacOS/Dolphin"
else
    echo "Dolphin not found; set DOLPHIN to its executable path" >&2
    exit 1
fi

exec "$dolphin" "--exec=$dol" "$@"
