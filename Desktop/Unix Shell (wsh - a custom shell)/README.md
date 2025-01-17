# Custom Unix Shell (`wsh`)

## Overview

`wsh` is a custom Unix shell that replicates and extends the functionality of traditional Unix/Linux shells. It was developed as part of the **CS 537: Introduction to Operating Systems** course at the University of Wisconsin–Madison. This project provides hands-on experience with process management, inter-process communication, and system-level programming.

---

## Key Features

- **Command Execution**: Run standard Unix/Linux commands and scripts seamlessly.  
- **Command History**: Store and reuse previously executed commands for enhanced usability.  
- **Environment Variables**: Manage local variables with a custom-built hashmap.  
- **I/O Redirection**: Support for input/output redirection and piping between processes.  
- **Batch Scripting**: Execute `.wsh` script files for task automation.  

---

## Purpose

This project was designed to:
- Deepen understanding of operating system concepts through practical implementation.
- Master system calls like `fork()`, `execvp()`, and `pipe()`.
- Improve resource utilization techniques and process management.
- Enhance skills in C programming and Unix/Linux system programming.

---

## Technical Implementation

### Command History

The shell uses a **circular array** to manage command history efficiently, storing a fixed number of commands and discarding the oldest as new commands are added.

### Environment Variables

Environment variables are handled using a **hashmap with chaining** to resolve collisions, enabling quick and efficient storage and retrieval of local variables.

### Piping and I/O Redirection

The `pipe()` system call is used for inter-process communication. More advanced execution scenarios are supported through redirection of standard input (`stdin`) and output (`stdout`) using `dup2()`.

### Process Management

Core system calls like `fork()` and `execvp()` are utilized for process creation and execution, ensuring concurrent process handling and efficient resource usage.

---

## Getting Started

### Prerequisites

- **Operating System**: Unix/Linux  
- **Compiler**: GCC or any compatible C compiler  

### Installation

1. Clone the repository:

    git clone git@github.com:SrujayReddy/Custom-Unix-Shell.git
    cd Custom-Unix-Shell

2. Compile the code:

    gcc wsh.c -o wsh

3. Run the shell:

    ./wsh

---

## Usage

### Basic Commands

Run standard Unix/Linux commands:

    ls -l
    pwd

### Piping

Redirect the output of one command as input to another:

    ls | grep ".c"

### Input/Output Redirection

Redirect input and output to/from files:

    cat < input.txt > output.txt

### Command History

Access previously executed commands:

    history

### Batch Scripting

Write commands in a `.wsh` script file:

    echo "Hello, World!"
    mkdir new_project
    cd new_project

Run the script:

    ./wsh script.wsh

---

## Challenges & Learning

This project helped me:
- Master **system-level programming** concepts like process creation, signals, and inter-process communication.
- Implement complex functionalities such as **piping** and **I/O redirection** efficiently.
- Debug and optimize **resource management** and edge-case handling in a concurrent environment.

---

## Future Enhancements

- Add support for job control features (`fg`, `bg`, `jobs`).
- Enhance scripting with loops and conditionals.
- Introduce customizable shell prompts and aliasing.

---

## License

This project was developed as part of the **CS 537: Introduction to Operating Systems** course at the University of Wisconsin–Madison. It is shared strictly for educational and learning purposes only.

**Important Notes:**
- Redistribution or reuse of this code for academic submissions is prohibited and may violate academic integrity policies.
- The project is licensed under the [MIT License](https://opensource.org/licenses/MIT). Any usage outside academic purposes must include proper attribution.
