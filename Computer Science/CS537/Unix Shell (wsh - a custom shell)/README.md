# Custom Unix Shell (`wsh`)

## Project Overview

`wsh` is a custom Unix shell designed as a part of the coursework for **CS 537: Introduction to Operating Systems** at the University of Wisconsin-Madison. This project implements features similar to traditional Unix/Linux shells while providing hands-on experience with process management, inter-process communication, and system-level programming.

The primary goal of this project was to gain a deeper understanding of operating system principles by implementing core shell functionalities, including command execution, history tracking, and piping.

---

## Key Features

- **Command Execution**: Execute standard Unix/Linux commands and scripts.
- **Command History**: Store and reuse previously executed commands, improving usability.
- **Local Variables**: Manage environment variables through a custom-built hashmap.
- **I/O Redirection**: Supports input/output redirection and piping for inter-process communication.
- **Batch Scripting**: Execute `.wsh` script files to automate commands.

---

## Purpose

This project serves as an educational tool to:
1. Explore the implementation details of a Unix shell.
2. Develop a deeper understanding of system calls like `fork()`, `execvp()`, and `pipe()`.
3. Improve process management and resource utilization techniques.
4. Build practical skills in C programming and Unix/Linux system programming.

---

## Implementation Details

### History Tracking

The command history feature is implemented using a **circular array**. This ensures that the shell can efficiently manage a fixed-size history buffer, discarding old commands as new ones are added.

### Local Variables

Environment variables are handled using a **hashmap with chaining** for collision resolution. This allows for efficient storage and retrieval of key-value pairs representing environment variables.

### Piping and I/O Redirection

The `pipe()` system call is used to enable communication between processes. Standard input (`stdin`) and output (`stdout`) can be redirected using `dup2()` to handle advanced command execution scenarios.

### Process Management

Key system calls like `fork()` and `execvp()` are utilized to create and execute child processes, allowing concurrent execution and effective resource utilization.

---

## Getting Started

### Prerequisites

- **Operating System**: Unix/Linux-based system
- **Compiler**: GCC or any C compiler

### Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/your-username/wsh-shell.git
   cd wsh-shell
