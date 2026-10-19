#!/usr/bin/env python3
# Terminal key events end to end: the ASCII build in a pty gets synthetic bytes (a key's auto-repeat, an arrow's
# escape sequence, SGR mouse, kitty keyboard protocol events) and every onInput event Lua sees is logged
# (BICYCLE_INPUT_LOG) in order, through Trigger::sendInput.
#   tests/tty_input_test.py <ascii executable>   (ctest: tty-input)
import os, pty, select, sys, tempfile, time

exe = sys.argv[1]
log = tempfile.NamedTemporaryFile(prefix="tty-input-", suffix=".log", delete=False).name
X, D, MOUSE_L = 24, 4, 49  # LogicalKey numbers (include/c/Input.h)
schedule = [(1.0, b"x")]                                          # X pressed...
schedule += [(1.5 + i * 0.033, b"x") for i in range(30)]          # ...auto-repeat after a 500 ms delay, ~1 s
schedule += [(3.0, b"\x1b[C")]                                    # right arrow: D (not "ESC [ C" = C)
schedule += [(4.0, b"\x1b[<0;10;5M"), (4.3, b"\x1b[<0;10;5m")]    # SGR mouse: left button down, up
schedule += [(5.0, b"\x1b[?11u"),                                 # kitty: the terminal answers the query
             (5.2, b"\x1b[120u"), (6.0, b"\x1b[120;1:2u"), (6.2, b"\x1b[120;1:3u")]  # x press, repeat, release
env = dict(os.environ, BICYCLE_INPUT_LOG=log, BICYCLE_MAX_FRAMES="230", TERM=os.environ.get("TERM") or "xterm-256color")
if env["TERM"] in ("", "dumb"):
    env["TERM"] = "xterm-256color"
pid, fd = pty.fork()
if pid == 0:
    os.execve(exe, [exe, "test"], env)
start = time.time()
out = b""
i = 0
status = None
while True:
    now = time.time() - start
    while i < len(schedule) and schedule[i][0] <= now:
        os.write(fd, schedule[i][1])
        i += 1
    r, _, _ = select.select([fd], [], [], 0.005)
    if r:
        try:
            out += os.read(fd, 65536)
        except OSError:
            pass
    p, st = os.waitpid(pid, os.WNOHANG)
    if p:
        status = st
        break
    if now > 60:
        os.kill(pid, 9)
        print("tty-input: the game hung")
        sys.exit(1)
events = []
for line in open(log):
    k, s = line.split()[1:3]
    if int(k) in (X, D, MOUSE_L):
        events.append("%s%s" % ("+" if s == "down" else "-", {X: "X", D: "D", MOUSE_L: "MouseL"}[int(k)]))
os.unlink(log)
# A hold is a tap (released right away) and, from its first auto-repeat, a press held until the repeats stop.
want = ["+X", "-X", "+X", "-X", "+D", "-D", "+MouseL", "-MouseL", "+X", "-X"]
if i < len(schedule):
    print("tty-input: the game quit before all input was sent (%.1fs)" % (time.time() - start))
    print(out[-2000:].decode(errors="replace"))
    sys.exit(1)
if events != want:
    print("tty-input: events %s, want %s" % (" ".join(events), " ".join(want)))
    sys.exit(1)
print("tty-input: OK:", " ".join(events))
