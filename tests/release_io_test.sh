#!/bin/sh
# Release builds must do zero file I/O of their own: runs the release executable headless for N frames under
# strace and fails if, after main() starts, it touches any path outside the operating system's own files.
#   tests/release_io_test.sh <executable> <ascii|2d|3d> [frames]      (ctest runs it: release-zero-io)
# main() marks its start with prctl(PR_GET_NAME) (src/main.cpp), so the dynamic loader's opens before it don't
# count. Allowed afterwards, because they are the OS's business rather than the game's (each is listed):
#   /dev /proc /sys            devices (sound card, GPU, tty), process info
#   shared libraries           (/usr)/lib*/..., /etc/ld.so.*: libraries the audio/GL stacks dlopen
#   locale                     /usr/lib/locale, gconv: setlocale(LC_CTYPE, "") for UTF-8 output (ASCII mode)
#   ALSA configuration         /usr/share/alsa, /etc/alsa, /etc/asound.conf, ~/.asoundrc, ~/.config/alsa
#   PulseAudio/PipeWire client /etc/pulse, ~/.config/pulse, /run/user, machine-id
#   X11 / Wayland              /tmp/.X11-unix, ~/.Xauthority, ~/.Xdefaults*, /usr/share/X11 and ~/.XCompose,
#                              ~/.compose-cache, /var/cache/libx11 (XOpenIM: keyboard compose tables), cursor themes
#                              (/usr/share/icons, /usr/share/pixmaps, ~/.icons, ~/.local/share/icons), XDG_RUNTIME_DIR
#   the executable itself      readlink(/proc/self/exe) and its path (SDL_GetBasePath, mesa's drirc app matching)
#   GL drivers (mesa, glvnd)   /usr/share/glvnd, /usr/share/drirc.d, /etc/drirc, ~/.drirc, mesa's shader cache
#   bicycle-mem://...          fluidsynth stat()s a SoundFont's name before handing it to bicycle's memory loader;
#                              it isn't a file (ENOENT) and nothing is read
# Anything else (the game folder, scripts/, fonts/, sf2/, .bpak, config or save files, relative paths) fails.
set -eu
exe=$1
mode=$2
frames=${3:-120}
command -v strace >/dev/null || { echo "release-zero-io: strace not installed; skipping"; exit 77; }
dir=$(mktemp -d)
trap '[ -n "${KEEP_TRACE:-}" ] && cp "$dir/trace.txt" "$KEEP_TRACE"; rm -rf "$dir"' EXIT
# Run from an empty folder: a release build must not need anything next to it.
cp "$exe" "$dir/game"
cd "$dir"
trace="open,openat,openat2,creat,stat,lstat,newfstatat,statx,access,faccessat,faccessat2,readlink,readlinkat,execve,prctl"
export BICYCLE_MAX_FRAMES="$frames" TERM="${TERM:-xterm-256color}"
case "$TERM" in dumb|"") TERM=xterm-256color ;; esac
run="strace -f -qq -o $dir/trace.txt -e trace=$trace ./game"
case "$mode" in
  ascii)
    command -v script >/dev/null || { echo "release-zero-io: needs script(1) for a pty; skipping"; exit 77; }
    timeout 60 script -qec "$run" /dev/null >"$dir/out.txt" 2>&1 || { echo "release-zero-io: the game failed"; tail -c 2000 "$dir/out.txt"; exit 1; } ;;
  2d|3d)
    if [ -z "${DISPLAY:-}" ]; then
      command -v xvfb-run >/dev/null || { echo "release-zero-io: no DISPLAY and no xvfb-run; skipping"; exit 77; }
      run="xvfb-run -a -s '-screen 0 1280x720x24' $run"
    fi
    LIBGL_ALWAYS_SOFTWARE=1 timeout 120 sh -c "$run" >"$dir/out.txt" 2>&1 || { echo "release-zero-io: the game failed"; tail -c 2000 "$dir/out.txt"; exit 1; } ;;
  *) echo "mode must be ascii, 2d or 3d"; exit 2 ;;
esac
grep -q PR_GET_NAME "$dir/trace.txt" || { echo "release-zero-io: no start-of-main marker (not a BICYCLE_RELEASE build?)"; exit 1; }
home=${HOME:-/nonexistent}
# Every path the game touched after main() started, once each.
awk '/PR_GET_NAME/ { on = 1; next } on' "$dir/trace.txt" | grep -v 'resumed>' |
  sed -n 's/^[0-9]* *[a-z0-9_]*(\(AT_FDCWD, \)\{0,1\}"\([^"]*\)".*/\2/p' | sort -u >"$dir/paths.txt"
allowed='^(/dev/|/proc/|/sys/|/(usr/)?lib(32|64|x32)?/|/usr/local/lib|/etc/ld\.so\.|/usr/lib/locale|/usr/share/locale|/usr/share/i18n|/usr/share/alsa|/etc/alsa|/usr/etc/(alsa|asound)|/etc/asound\.conf|/var/lib/alsa|/etc/pulse|/run/user/|/run/pulse|/etc/machine-id|/var/lib/dbus/machine-id|/tmp/\.X11-unix|/tmp/\.ICE-unix|/usr/share/X11|/usr/share/icons|/usr/share/pixmaps|/var/cache/libx11|/var/cache/fontconfig|/usr/share/glvnd|/usr/share/drirc\.d|/etc/drirc|/usr/share/vulkan|/etc/vulkan|/usr/share/egl|/etc/egl|bicycle-mem://)'
homeAllowed="^$home/(\.asoundrc|\.config/alsa|\.config/pulse|\.pulse|\.Xauthority|\.Xdefaults[^/]*|\.XCompose|\.compose-cache|\.icons|\.local/share/icons|\.drirc|\.cache/mesa_shader_cache|\.cache/mesa_shader_cache_db|\.cache)(/|\$)"
bad=$(grep -Ev "$allowed" "$dir/paths.txt" | grep -Ev "$homeAllowed" | grep -Ev "^(/|/proc|/run|/tmp|/usr|/usr/share|/etc|/run/user|/home|$home|$dir|$dir/game)\$" || true)
n=$(wc -l <"$dir/paths.txt")
echo "release-zero-io ($mode, $frames frames): $n paths touched after main(), all OS/driver files:"
sed 's/^/  /' "$dir/paths.txt" | sed -E 's#/dev/(snd/controlC|aloadC)[0-9]+#/dev/\1N#' | sort -u
if [ -n "$bad" ]; then
  echo "release-zero-io: FAILED: the game touched files of its own:"
  echo "$bad" | sed 's/^/  /'
  exit 1
fi
echo "release-zero-io: OK"
