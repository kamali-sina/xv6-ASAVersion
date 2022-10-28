
The Lab project of OS course of University of Tehran at the fall of 2020. This project is written, maintained and edited by Sina Kamali, Amir Mahdi Mohamadian and Alireza Ebrahimi. Not all of the codes in this project belongs to us and the main code is recieved from the MIT-PDOS account.

In three steps we will develope and add new functions to the raw xv6 os. In the first step we added some primitive functualities like ctrl+(c/v/b/x).

# xv6-ASAVersion

xv6-ASAVersion is an operating system cloned from [xv6](https://github.com/mit-pdos/xv6-public) with some added features. Xv6 is a modern reimplementation of Sixth Edition Unix in ANSI C for multiprocessor x86 and RISC-V systems, developed in the summer of 2006 for MIT's operating systems course. xv6-ASAVersion is the result of lab projects of operating systems course at the University of Tehran.

- [xv6-ASAVersion](#xv6-ASAVersion)
  - [Added Features](#added-features)
    - [Part 1 (Introduction to XV6)](#part-1-introduction-to-xv6)
      - [Boot Message](#boot-message)
      - [ctrl + c](#ctrl--c)
      - [ctrl + x](#ctrl--x)
      - [ctrl + v](#ctrl--v)
      - [ctrl + b](#ctrl--b)
      - [ctrl + l](#ctrl--l)
      - [lcm](#lcm)
    - [Part 2 (System Calls)](#part-2-system-calls)
      - [reverse_number(int n)](#reverse_numberint-n)
      - [trace_syscalls(int state)](#trace_syscallsint-state)
      - [get_children(int parent_id)](#get_childrenint-parent_id)
      - [syscalltest](#syscalltest)
  - [How to use](#how-to-use)
    - [Building and Running xv6](#building-and-running-xv6)
      - [Building xv6](#building-xv6)
      - [Running xv6](#running-xv6)

## Added Features

### Part 1 (Introduction to XV6)

#### Boot Message

The operating system's name is added to the boot message.

#### ctrl + c

Copy all the text written on the current console line to the Clipboard.

#### ctrl + x

Cut all the text written on the current console line and copy it to the Clipboard.

#### ctrl + v

Paste the contents of the Clipboard.

#### ctrl + b

Replace the current text on the console with contents of the Clipboard.

#### ctrl + l

Clear the screen (terminal and CGA).

#### lcm

`lcm` is a user-level program written in C language, and it has been added to the operating system user-level programs. The program receives up to eight inputs and calculates their least common multiple. The program stores its output in a text file called `lcm_result.txt`. If the text file already exists, the output will be overwritten.

**Example:**

```shell
$ lcm 3 6 11 9
$ cat lcm_result.txt
198
```

### Part 2 (System Calls)

#### reverse_number(int n)

In this system call, instead of retrieving arguments in the usual way (using stack), we use registers. This system call prints the digits of the given number in reverse on the console.

#### trace_syscalls(int state)

This system call counts the number of times a system call is used by each process and saves it. If the input argument is one, tracing of system calls begins. If the system call's argument is zero, this tracing will no longer be performed and will also clear all the information it has stored.

#### get_children(int parent_id)

This system call takes a PID as an input and returns children's PIDs of the given PID.

#### syscalltest

`syscalltest` is a user-level program written in C language, and it has been added to the operating system user-level programs. In order to be able to test system calls, you need to use this user-level program.

```shell
$ systest
Enter the number of a system call that you would like to test?
1. reverse_number(int number)
2. trace_syscalls(int state)
3. get_children(int parent_id)
```

## How to use

### Building and Running xv6

Xv6 typically is ran using the QEMU emulator.

#### Building xv6

To build xv6 on an x86 ELF machine, like Linux, run:

```shell
 make
```

On macOS, run:

```shell
make TOOLPREFIX=i386-elf-
```

#### Running xv6

To build everything and start QEMU with the CGA console (in a new window and the serial console in your terminal on an x86 ELF machine), like Linux, run:

```shell
make qemu
```

On macOS run:

```shell
make TOOLPREFIX=i386-elf- qemu
```

To exit, either close the CGA window or press `ctrl + c` or `ctrl + a` and then `x` in your terminal.

Like above, but to start with only the serial console on an x86 ELF machine, like Linux, run:

```shell
make qemu-nox
```

On macOS, run:

```shell
make TOOLPREFIX=i386-elf- qemu-nox
```

To exit, press `ctrl + a` and then `x`.
