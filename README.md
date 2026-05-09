# zirvutils — Zirvium System Utilities

Standalone userspace utilities for the Zirvium kernel. Each is a statically linked, freestanding, no-pie ELF binary embedded into the kernel and launched via `execve()`.

## Utilities

| Binary | Description |
|--------|-------------|
| `hello` | Prints "Hello from ZirvUtils!" |
| `cat` | Reads files and writes to stdout |
| `sysinfo` | Shows PID, CWD, and kernel name |
| `clear` | ANSI escape clear screen |
| `echo` | Prints arguments to stdout |

## Build

```bash
make
```

Produces `<name>.elf` binaries in the project root.
