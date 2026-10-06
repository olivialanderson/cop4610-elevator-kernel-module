# Elevator

COP4610 Project 2. This project modifies the Linux kernel (6.16.x) to add three system calls
(`start_elevator`, `issue_request`, `stop_elevator`) and implements an `elevator` kernel module
that schedules a pet elevator across 5 floors. The elevator runs in a kthread, protects shared
data with a mutex, stores waiting and riding pets in kmalloc'd linked lists, and reports its live
state through `/proc/elevator`. The repo also includes system-call tracing with strace (Part 1)
and a `my_timer` kernel module that exposes `/proc/timer` (Part 2).

## Group Members
- **Gabriel Valladares-Ruiz**: [fsu email]
- **Olivia Anderson**: [fsu email]
- **Gannon Wooley**: [fsu email]

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
```
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
│   │   └── elevator.c
│   ├── Makefile
│   └── syscalls.c
├── division_of_labor.md
├── Makefile
└── README.md
```

# How to Compile & Execute

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
`part1.trace` should show exactly five more system calls than `empty.trace`.

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
This builds the kernel module `elevator.ko`. The kernel must already be compiled and installed
with the three elevator system calls.

### Execution
```bash
sudo insmod elevator.ko
./consumer --start
./producer [number_of_pets]
watch -n 1 cat /proc/elevator
./consumer --stop
sudo rmmod elevator
```
`/proc/elevator` shows the elevator state, current floor, load, the pets on board, the pets
waiting on each floor, and the number of pets serviced.

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

### Gannon Wooley

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |

## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |

## Bugs
- None known yet.

## Considerations
- [Add any design decisions or notes for the grader here.]
