# Elevator

COP4610 Project 2. This project modifies the Linux kernel (6.16.x) to add three system calls
(`start_elevator`, `issue_request`, `stop_elevator`) and implements an `elevator` kernel module
that schedules a pet elevator across 5 floors. The elevator runs in a kthread, protects shared
data with a mutex, stores waiting and riding pets in kmalloc'd linked lists, and reports its live
state through `/proc/elevator`. The repo also includes system-call tracing with strace (Part 1)
and a `my_timer` kernel module that exposes `/proc/timer` (Part 2).

## Group Members

- **Gabriel Valladares-Ruiz**
- **Olivia Anderson**
- **Gannon Wooley**

## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: Write `empty.c` and `part1.c` (exactly five more system calls than
  `empty`), generate `empty.trace` and `part1.trace` with strace, and write the Makefile.
- **Assigned to**: Olivia Anderson, Gannon Wooley

### Part 2: Timer Kernel Module
- **Responsibilities**: Implement the `my_timer` module using `ktime_get_real_ts64()`, create
  and remove `/proc/timer`, and print the current time and elapsed time on each read.
- **Assigned to**: Gabriel Valladares-Ruiz, Olivia Anderson

### Part 3a: Adding System Calls
- **Responsibilities**: Add `start_elevator` (548), `issue_request` (549), and `stop_elevator`
  (550) by creating `syscalls/syscalls.c` and `syscalls/Makefile` and editing
  `syscall_64.tbl`, `syscalls.h`, and the kernel Makefile.
- **Assigned to**: Olivia Anderson, Gannon Wooley

### Part 3b: Kernel Compilation
- **Responsibilities**: Configure, compile, and install the modified 6.16.x kernel, then verify
  it with `uname -r` and the provided system-call tests.
- **Assigned to**: Olivia Anderson, Gannon Wooley

### Part 3c: Threads
- **Responsibilities**: Create the kthread that drives the elevator, including the 2.0 second
  delay between floors and the 1.0 second delay when loading and unloading.
- **Assigned to**: Gabriel Valladares-Ruiz, Gannon Wooley

### Part 3d: Linked List
- **Responsibilities**: Allocate pets with `kmalloc`, store waiting pets per floor and riding
  pets in the elevator using linked lists, and free all memory when the module is removed.
- **Assigned to**: Gabriel Valladares-Ruiz, Olivia Anderson

### Part 3e: Mutexes
- **Responsibilities**: Protect all shared floor and elevator data with a mutex across the
  kthread, the system calls, and the `/proc/elevator` read.
- **Assigned to**: Olivia Anderson, Gannon Wooley

### Part 3f: Scheduling Algorithm
- **Responsibilities**: Decide elevator movement and loading, board pets in FIFO order, enforce
  the 5-pet and 50 lb limits, and unload all riders before going OFFLINE on stop.
- **Assigned to**: Gabriel Valladares-Ruiz, Gannon Wooley

## File Listing

```text
cop4610-elevator-kernel-module/
├── part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
│   └── Makefile
├── part2/
│   ├── src/
│   │   └── my_timer.c
│   └── Makefile
├── part3/
│   ├── src/
│   │   ├── elevator.h
│   │   ├── elevator_main.c
│   │   ├── elevator_proc.c
│   │   ├── elevator_thread.c
│   │   └── pets.c
│   ├── Makefile
│   └── syscalls.c
├── division_of_labor.md
└── README.md
```

## How to Compile & Execute

Run each compilation section from the repository root. On the shared VM, each member
uses their own clone under `~/YOURNAME/cop4610-elevator-kernel-module`. The installed
kernel and loaded modules are shared, so coordinate module testing with the group.

### Requirements

- **Compiler**: `gcc`
- **Kernel**: Linux 6.16.x, compiled with the system calls from Part 3
- **Tools**: `make`, `strace`, kernel headers for the running kernel, `sudo` access

## Part 1

### Compilation

```bash
cd part1
make
```
This builds the `empty` and `part1` executables.

### Execution

```bash
strace -o empty.trace ./empty
strace -o part1.trace ./part1
```
The committed traces contain 30 system calls in `empty.trace` and 35 in `part1.trace`.
The five extra calls are `getpid`, `getppid`, `getuid`, `getgid`, and `write`.

## Part 2

### Compilation

```bash
cd part2
make
```
This builds the kernel module `my_timer.ko`.

### Execution

```bash
sudo insmod my_timer.ko
cat /proc/timer
sleep 1
cat /proc/timer
sudo rmmod my_timer
```
Each read prints the current time and, after the first read, the elapsed time since the last
read.

## Part 3

### Compilation

```bash
cd part3
make
```
This builds the kernel module `elevator.ko`. The shared VM already runs Linux 6.16.12
with syscalls 548-550 installed; rebuild the module after source changes. The
`part3/syscalls.c` file is the kernel-side syscall source, not an object linked into
`elevator.ko`. A separate machine needs the corresponding kernel integration and
installation before it can use the module.

### Execution

The following commands assume the provided producer and consumer are already built
in `~/elevator-test` on the shared VM.

From the clone's `part3` directory, load the module:

```bash
sudo insmod elevator.ko
cd ~/elevator-test
./consumer --start
./producer 20
watch -n 1 cat /proc/elevator
```

Press **Ctrl+C** to leave `watch`; this does not stop the elevator. To test a graceful
stop, issue the stop while riders are still aboard:

```bash
./consumer --stop
watch -n 1 cat /proc/elevator
```

Wait for `OFFLINE`, zero riders, and zero load, then press **Ctrl+C** and unload:

```bash
sudo rmmod elevator
```

A stop prevents further boarding while the elevator delivers its current riders.
Waiting pets may remain queued when it reaches `OFFLINE`. Before unloading, another
`./consumer --start` starts at floor 1 and allows those waiting requests to be served.
Module removal frees any remaining queued pets.

`/proc/elevator` shows the elevator state, current floor, load, the pets on board, the pets
waiting on each floor, and the number of pets serviced.

### Scheduling

- Unload pets at their destination before boarding from the current floor's queue.
- Board from the head of each queue in FIFO order. If the next pet would exceed
  5 riders or 50 lbs, leave it and the pets behind it queued.
- Display `LOADING` and wait 1 second after a loading/unloading visit.
- Keep the current direction while work remains ahead; reverse when needed.
  Move one floor at a time with a 2-second delay, without wrapping between floors 5 and 1.
- Use `IDLE` when active with no work. On stop, ignore waiting queues, drain riders,
  then enter `OFFLINE`.
- Hold `elevator_lock` for shared data access and release it before all thread delays.
  Free each delivered pet and increment `total_serviced`.

### Verification recorded

- Gannon built and ran the scheduler on the shared Linux 6.16.12 VM with the provided
  producer/consumer tools. A captured running state showed `UP` at floor 4, 4 riders,
  exactly 50 lbs, and 16 pets serviced. Gannon also reported that the stop test completed
  with an empty elevator in `OFFLINE`.
- The scheduler was additionally checked with a userspace simulation using kernel API
  shims: 12 targeted scenarios and 200 randomized runs covering 40,000 pets passed.
  These assistant-run checks covered FIFO boarding, both capacity limits, floor bounds,
  shared-data locking, idle/offline behavior, and stopping during loading and travel.
  They do not replace kernel execution tests; the simulation harness is not included in
  this repository.
- A teammate's independent test of the pulled scheduler and the final combined demo
  remain to be recorded.

## Development Log

Each member records their contributions here.

### Gabriel Valladares-Ruiz

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |

### Olivia Anderson

| Date       | Work Completed / Notes                                 |
|------------|--------------------------------------------------------|
| 2026-10-06 | Created GitHub repository and division of labor; README |
| 2026-10-26 | Set up lab VM 427: SSH access, installed openssh-server; downloaded 6.16.12 |
| 2026-10-26 | Added syscalls 548-550: syscalls/, syscall_64.tbl, syscalls.h, Makefile    |
| 2026-10-26 | Configured kernel (localmodconfig, disabled cert keys); started build      |
| 2026-10-26 | Completed Part 1: empty.c, part1.c, strace output, Makefile                |
| 2026-10-29 | Installed 6.16.12 kernel; verified syscalls 548-550 with syscheck test |
| 2026-10-29 | Completed Part 2: my_timer module with /proc/timer                     |
| 2026-10-29 | Part 3 stage 1: pet lists, syscalls, /proc/elevator, kthread lifecycle |

### Gannon Wooley

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-10-29 | Implemented the Part 3 scheduler in `elevator_thread.c` with ChatGPT assistance: FIFO boarding, 5-pet/50-lb limits, unloading and memory cleanup, direction selection, delays outside the mutex, IDLE behavior, and graceful stop. |
| 2026-10-29 | Built the module on the shared Linux 6.16.12 VM and tested with producer/consumer and `/proc/elevator`; observed movement and a 50-lb load, then confirmed the stop test worked. |
| 2026-11-02 | Pushed the scheduler in commit [ea04bd2](https://github.com/olivialanderson/cop4610-elevator-kernel-module/commit/ea04bd270b4ede9618beb731e4519b28a0207535). |
| 2026-11-02 | Updated the README with scheduler behavior, recorded tests, the current file listing, shared-VM run instructions, and assistance disclosure. |

## Meetings

Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |

## Known Limitations

- Both `/proc` read handlers currently return EOF after the first successful read on
  an open file. Reads with small buffers can therefore truncate the displayed data.
- Syscall callback removal is not synchronized with syscalls already in progress.
  Concurrent module removal and producer/consumer calls can race. Stop request
  generation and let active calls finish before unloading; callback lifetime
  synchronization remains a code-review follow-up.
- Kernel integration changes outside `part3/syscalls.c` (the syscall table, kernel
  header declarations, and kernel Makefiles) are installed on the shared VM but are
  not tracked in this repository.

## Considerations

- Kernel configured with `make localmodconfig` so only modules the VM uses are built.
- Kernel build also needed `libdw-dev` (provides `dwarf.h`), which is not in the slide list.
- The VM shipped with kernel 7.0.0; we built 6.16.x per the handout and boot it explicitly.
- Run `make clean` in the relevant part before staging changes; do not commit generated
  modules, object files, or executables.
- AI use: Claude (Anthropic) helped with setup steps, the syscall wrappers, and Part 1 code.
  The team reviewed and tested all output; transcripts are saved.
- ChatGPT (OpenAI) assisted Gannon with the Part 3 scheduler implementation, setup and
  Git troubleshooting, source review, userspace simulation checks, and these README
  updates. Gannon copied the scheduler into his clone, built and ran it on the VM,
  and pushed the code. The verification section distinguishes the VM observations
  from assistant-run simulations.
