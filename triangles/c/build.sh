#!/bin/sh
set -eu
cd "$(dirname "$0")"

: "${DEVKITPRO:?Set DEVKITPRO to your devkitPro installation}"
: "${DEVKITPPC:?Set DEVKITPPC to your devkitPPC installation}"

PATH="$DEVKITPRO/tools/bin:$DEVKITPPC/bin:$PATH"
export PATH

name=triangles
target=target
libogc="$DEVKITPRO/libogc2/wii"
portlibs="$DEVKITPRO/portlibs/ppc"
mkdir -p "$target"

flags="-std=c23 -g -O2 -Wall -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float"
includes="-Isrc -I$target -I$libogc/include -I$portlibs/include"
for source in src/*.c; do
    object="$target/$(basename "${source%.c}").o"
    powerpc-eabi-gcc $flags $includes -MMD -MP -MF "$target/$(basename "${source%.c}").d" -c "$source" -o "$object"
done

powerpc-eabi-gcc -g -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float \
    "$target"/*.o -Wl,-Map,"$target/$name.elf.map" \
    -L"$libogc/lib" -L"$portlibs/lib" -lwiiuse -lbte -logc -lm \
    -o "$target/$name.elf"
elf2dol "$target/$name.elf" "$target/$name.dol"

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

exec "$dolphin" "--exec=$(pwd -P)/$target/$name.dol"
