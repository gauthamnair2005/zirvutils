# zirvutils — MOSIX System Utilities (17 programs)

Standalone userspace utilities for **MOSIX** operating systems. Each is a
statically linked, freestanding, no-pie ELF64 binary embedded into the kernel
and launched via `execve()`.

Part of the [Zirvium](https://github.com/gauthamnair2005/zirvium) reference
MOSIX implementation. See the [MOSIX specification](https://github.com/gauthamnair2005/zirvworld)
for the full standard.

## Utilities

| Binary | Description |
|--------|-------------|
| `hello` | Prints "Hello from ZirvUtils!" and PID |
| `cat` | Reads files via open()/read(), writes to stdout |
| `sysinfo` | Shows PID, CWD, kernel name, hostname |
| `clear` | ANSI escape clear screen |
| `echo` | Prints arguments to stdout |
| `reboot` | System reboot via SYS_REBOOT |
| `shutdown` | System shutdown via SYS_SHUTDOWN |
| `poweroff` | Alias for shutdown |
| `suspend` | Stub (not supported) |
| `ping` | Check VFS path reachability via open() |
| `sleep` | Busy-wait using uptime() |
| `true` | Return exit code 0 |
| `false` | Return exit code 1 |
| `yes` | Infinite "y" output |
| `uname` | System information (-a flag) |
| `hostname` | Get or set system hostname |

## Build

```bash
make
```

Produces 17 `.elf` binaries. All linked against [zirvlibc](https://github.com/gauthamnair2005/zirvlibc).
