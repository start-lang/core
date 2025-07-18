![Version](https://start-lang.github.io/core/img/version.svg)
![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/start-lang/core/ci.yaml)
![Tests](https://start-lang.github.io/core/img/tests.svg)
![Coverage](https://start-lang.github.io/core/img/coverage.svg)
![GitHub commit activity](https://img.shields.io/github/commit-activity/t/start-lang/core/main?label=commits)

## 1. Introduction

\*T (pronounced "start") is a structured, interpreted, and compact esoteric language designed to work even in extremely resource-limited environments. The project began with a simple question: what would be the most minimal way to interact with a digital device to program it? The original idea involved an almost absurd scenario — an LED matrix, a potentiometer, and a button — but it served as the basis for imagining a language that could operate with minimal human input.

During initial tests, it became clear that languages like Brainfuck, despite being minimalist, were not practical for this type of interface. They require too many commands for simple tasks. \*T was created in response to this "limitation": a synthetically expressive language with single-character operators and a syntax designed to be dense yet interpretable.

Despite its experimental nature, the project includes an interpreter written in C with very few dependencies, its own test suite, compatibility with Brainfuck, support for WebAssembly execution, and an automated build and test system. The interpreter can run both in the terminal and in the browser and was designed to allow step-by-step execution — enabling it to run programs on devices that cannot fit the entire code into memory.

\*T is not a language made to make things easier. It is made to explore. The idea is to offer a minimal base that can be taken to unconventional uses and odd environments, where limitations become part of the challenge.

```st
9!>0!>1!?=[2<1-?!2>;<@>+] ;PN // Fibonacci of 9
```

### 1.1. Features

- Numeric constants and string loading
- Mathematical, logical and bitwise operators
- Multi-line and single-line comments
- Support for unsigned 8, 16, 32-bit integers and 32-bit floats
- Structures like if-else and while with continue and break support
- Stack operators
- Function declaration and execution with reusable scope
- Designed to run in limited environments, such as gadgets and microcontrollers
- Dynamic execution of code stored in memory
- Flexible numeric type system with dynamic switching
- Debugging tools for analysis and execution in restricted environments
- Interpreter extensibility with external functions
- Compatible with runtime code generation and execution
- Compatible with Brainfuck codes

You can try the language directly in your browser:

* [https://start-lang.github.io/core/](https://start-lang.github.io/core/)  — with an integrated editor and terminal
* [https://esolangs.org/wiki/\*T](https://esolangs.org/wiki/\*T)  — entry on the esolang community wiki

This repository is a starting point for exploring the language, compiling the interpreter, running examples, and contributing new ideas. The goal is to keep the project accessible, functional, and curious enough to inspire experiments with alternative languages and devices.

## 2. Requirements

To compile and run the \*T language interpreter locally, you need an environment with minimal development tools. In addition to direct execution in the terminal, the project can also run inside a Docker container configured with all dependencies or be compiled to WebAssembly and executed via Node.js or a browser.

### 2.1 Local Compilation

The interpreter can be compiled on Linux or macOS systems. The basic requirements are:

- `make`
- `clang` (recommended) or `gcc`
- `python3` (optional, for generating SVGs)
- `valgrind` or `leaks` (optional, for memory tests)

Local compilation can be done directly with the commands `make build-cli` or `make build-wasm`, depending on the target environment.

### 2.2 Execution via Docker

To isolate the environment and avoid differences in behavior between platforms, the project offers support for execution via Docker. The image used is: `aantunes/clang-wasm:latest`

It contains all the tools needed to compile the project to WebAssembly, run tests, generate assets, and execute the main targets of the `Makefile`. Using Docker is particularly important when you want to ensure reproducibility in builds or when the local operating system does not have the necessary toolchains.

Execution via Docker is done with targets that start with `docker-run-`, for example:

```bash
make docker-run-test
make docker-run-build-wasm
```

Important: avoid switching between local and dockerized builds without running `make clean` in between. This prevents conflicts caused by platform differences in the generated binary.

### 2.3 Execution with Node.js (WebAssembly)

To run the .wasm binaries of the interpreter locally, you need to have Node.js installed. The `wasm_runtime.js` runtime is compatible with both Node and browsers and can be used to test the WebAssembly version of the language:

```bash
node targets/web/wasm_runtime.js build/start.wasm '"Hello, World!" PS'
```

Using Node is only useful to validate the compatibility of the WebAssembly version with the native C version, as both share the same codebase.

## 3. How to compile and run

The \*T language can be compiled for both command-line environments and WebAssembly. The project is organized to allow direct use via terminal, execution in modern browsers, or even on embedded devices. This section covers the main ways to compile and run the interpreter, both locally and via Docker, explaining the necessary precautions and the design decisions that motivated this structure.

### 3.1. Compiling locally

To compile locally, the project requires a C99 compatible compiler. Initially, both `gcc` and `clang` work, but WebAssembly support depends on using `clang`. By default, the `Makefile` uses `clang`, and on systems where it is not available, you can force the use of `gcc` with:

```bash
make build-cli-gcc
```

The main build targets are:

- `build-cli`: compiles the command-line interpreter
- `build-test`: compiles the automated tests
- `build-benchmark`: compiles the interpreter with specific parameters to measure performance
- `build-wasm`: generates the WebAssembly binary compatible with browsers and Node.js

If you switch between locally made builds and builds made via Docker, it is important to run `make clean` before recompiling. This avoids architecture conflicts or inconsistencies between toolchains. Compiling part of the project with system libraries and another part inside the container can result in hard-to-diagnose errors, especially in the case of WebAssembly.

### 3.2. Running the CLI interpreter

After compiling with `make build-cli`, the interpreter will be available in `build/start`.

The command line accepts code as a direct argument, reading from `.st` files, and also input via `stdin`. Some examples:

```bash
./build/start '"Hello, World!" PS'
./build/start -f tests/quine.st
echo '>,[>,]<[.<]' | ./build/start -
```

You can pass additional options to activate debug modes, limit the number of steps, or restrict output. The main parameters are described in section 3.3..

## 3.3. CLI interpreter parameters

The command-line interpreter accepts various parameters that control the mode of execution, activate debugging, limit resources, or change the default behavior. The table below summarizes the main ones:

| Parameter           | Description                                                                 |
|---------------------|---------------------------------------------------------------------------|
| `-h`, `--help`       | Displays help with a description of the parameters and usage examples     |
| `-f FILE`           | Reads the code to be executed from a `.st` file                           |
| `-` or `--stdin`    | Reads the code directly from standard input                               |
| `-d`, `--debug`     | Activates step-by-step debugging mode, with textual output after each operation |
| `-i`, `--interactive` | Enters interactive mode with a visual interface in the terminal          |
| `-e`, `--exec-info` | Displays execution information at the end (time, memory, number of steps) |
| `-S N`              | Sets a step limit, in thousands (e.g., `-S 10` = 10,000 steps)           |
| `-O N`              | Sets a text output limit, in characters                                   |
| `-t N`              | Sets a maximum execution time, in seconds (e.g., `-t 5` = 5s)             |
| `-s`                | Shows the parsed code before executing                                    |
| `-v`, `--version`   | Displays the interpreter version                                          |

These parameters can be freely combined. Some practical examples:

- Execute code with interactive visualization:

  ```bash
  ./build/start -i '"Hello, World!" PS'
  ```

- Run a file with step and output limits:

  ```bash
  ./build/start -f tests/quine.st -S 50 -O 100
  ```

- Send code via `stdin` with debugging:

  ```bash
  echo '8!>0!>1!?=[2<1-?!2>;<@>+] ;PN' | ./build/start -d -
  ```

The default values, if not specified, are:

- `max_steps`: 2,500,000 steps
- `max_output`: unlimited
- `timeout`: 3 seconds (except in debug mode)

## 4. Testing

The \*T language includes its own test suite, developed to validate both the behavior of the language and the robustness of the interpreter implementation. The system was designed to ensure portability between different execution environments, including versions compiled with `clang`, `gcc`, and WebAssembly, and to facilitate the addition of new tests as the language evolves.

The tests cover everything from simple operations and individual operators to more complex cases like recursion, memory usage, chunked execution, and file and `stdin` input/output.

### 4.1. Test suite overview

The basis of the unit tests is the MicroCuts (Micro C Unit Test Suite) library, included in the project as a submodule. It provides assertion macros and verification utilities, used mainly in the `tests/unit/lang_assertions.c` file.

This file centralizes the semantic tests of the language. It validates:

- All the main operators of the language
- Numeric types (integers and floats)
- Interpreter behavior in the face of errors
- Step-by-step execution (one operator at a time)
- Memory allocation and deallocation
- Variable definition and scope
- Function creation and calling
- File reading and chunked execution

In addition to the unit tests, there are specific directories for other types of tests:

- `tests/cli/`: command-line interface tests with different input/output combinations
- `tests/bf/`: Brainfuck programs used to validate compatibility and performance
- `tests/mandelbrot/`: comparison between C and \*T versions of a fractal generator
- `tests/unit/`: internal assertions and validations, using MicroCuts

### 4.2. Test targets

The following are the main test commands available via Makefile:

- `make test`
  Runs the main language tests in fast mode. This target is used by CI and includes coverage, CLI execution, and WebAssembly (Node.js) execution verification.

- `make test-long`
  Runs the same set of tests as the `test` target, followed by a complete benchmark with interpreters compiled with `gcc`, `clang`, and WebAssembly. The benchmark result is saved in `build/benchmark.out`.

- `make test-full`
  Runs the `test-long` target followed by a memory analysis with `valgrind` (Linux) or `leaks` (macOS). Ideal for detecting leaks, undefined behavior, or invalid memory operations.

- `make test-cli`
  Runs all tests located in `tests/cli/`. Each test has a `.st` file with the source code and may have `.in` (input) and `.out` (expected output) files. The interpreter is executed, and the result is compared line by line. Any divergence invalidates the test.

- `make test-wasm-cli`
  Runs a simple test in WebAssembly using the interpreter compiled as `.wasm`, running with Node.js. The goal is to validate if the WebAssembly version produces the same output as the C version.

- `make test-wasm-example`
  Runs a more complete test in WebAssembly, executing an example code in the simulated web environment (`targets/web/example.js`).

- `make test-quick`
  Quickly executes the previously compiled test binary. Useful for re-executing after small code changes without recompiling.

### 4.3. Tests with input, files, and STDIN

The command-line tests cover different input forms:

- Code passed as a direct argument:

  ```bash
  ./build/start '"Hello, World!" PS'
  ```

- Code read from a `.st` file with `-f`:

  ```bash
  ./build/start -f tests/quine.st
  ```

- Code read via standard input (`stdin`) with `-`:

  ```bash
  echo '>,[>,]<[.<]' | ./build/start -
  ```

In addition, some tests involve interactive input (simulated) to validate codes that depend on user input, such as:

- `tests/bf/pi.st`: calculates digits of pi from a number read from input
- `tests/bf/calc.st`: performs operations with numbers read from input
- `tests/cli/*.in`: provide specific inputs for different test codes

This set of tests ensures that the interpreter works correctly with both inline input and files, that it correctly interprets the language operators, and that it maintains compatibility with legacy Brainfuck codes.

## 5. Docker and environment isolation

To ensure reproducibility and isolation of the build environment, the project includes support for Docker. The used image encapsulates the necessary tools to compile, test, and generate the binaries of the \*T language, including specific toolchains like `clang`, utilities for WebAssembly, benchmarking tools, and auxiliary scripts.

This approach has two main objectives:

1. Avoid discrepancies between different operating systems, compiler versions, or native dependencies.
2. Allow the entire build and test process to run predictably, whether on local machines or in CI/CD.

### 5.1. Available Docker targets

The main commands using Docker are:

- `make docker-run-test`
  Runs the main language tests, using the isolated environment.

- `make docker-run-build-cli`
  Compiles the interpreter.

- `make docker-run-build-wasm`
  Compiles the interpreter to WebAssembly inside the container.

- `make docker-run-test-mandelbrot`
  Executes the example that draws the mandelbrot fractal.

> It is important to avoid mixing locally made builds with builds made via Docker. As the generated binaries may differ (due to compiler, flags, or environment), it is best to always run `make clean` before switching between the two modes.

## 6. Repository structure

The repository is organized as follows:

- **`.github/workflows/`**
  Contains the GitHub Actions files responsible for running automated tests, builds, and publishing the site. They are mainly used in CI/CD processes.

- **`assets/`**
  Stores static files used in the web interface and during deployment.

- **`build/`**
  Temporary directory generated during the execution of Makefile commands. It is where the compiled binaries, intermediate files, test results, and assets ready for publication are located. It can be cleaned with the `make clean` target.

- **`doc/`**
  Contains the project documentation in Markdown format, written in Portuguese and English. These files are used on the website to provide references about the language, examples, and explanations of commands and \*T structures.

- **`lib/`**
  Contains external and internal libraries used by the project. There are three main ones:
  - `microcuts/` (Micro C Unit Test Suite): testing library based on assertions.
  - `wunstd/` (Web Unstandard): library with simplified implementations of standard C functions (`stdlib`, `string`, etc.) aimed at WebAssembly use.
  - `tools/`: internal library with auxiliary functions used in the interpreter and tests.

- **`src/`**
  Main directory of the language. Contains:
  - `start_lang.c`: implementation of the interpreter and main structures.
  - `start_lang.h`: definition of structures, constants, macros, and types used in the interpreter.
  - `start_tokens.h`: list of operators recognized by the language.

- **`targets/`**
  Contains specific entry points for different platforms:
  - `desktop/cli.c`: main CLI used to compile and run the language on Linux/macOS systems.
  - `web/`: files used in the WebAssembly version. Includes `wasm_runtime.js`, used in both browsers and Node.js.
  - `gfx/`: (future) graphical implementation with SDL, aimed at visual execution on devices like the RG35XX.
  - `arduino/`: (future) entry to compile the language using the Arduino CLI on microcontrollers like ESP32, RP2040, Teensy, and others.

- **`tests/`**
  Directory with the language tests, organized by type:
  - `bf/`: compatibility tests with Brainfuck, including a legacy converter that is now redundant.
  - `cli/`: automated CLI tests. Include `.st`, `.in`, and `.out` files used to validate behavior.
  - `mandelbrot/`: code in \*T and C for direct comparison. The C code was manually transpiled to \*T maintaining the structure.
  - `unit/`: unit tests of the language, the `validate` function in `lang_assertions.c` covers operators, error codes, chunked execution, dynamic allocation, variable and function creation.

## 7. Interpreter particularities

One of the main characteristics of the \*T language interpreter is its ability to execute instructions incrementally, through a step-by-step execution system. This means that instead of processing the entire code at once, the interpreter can analyze and execute a single operator at a time and return immediately after that operation. This approach was thought out from the beginning as a way to allow the language to run on devices with extremely restricted resources.

The idea is that the source code can be kept outside the main memory — for example, stored in an external file system, EEPROM, or even transferred via serial communication — and loaded in small blocks (chunks) as needed. The interpreter maintains its internal state between steps and is able to indicate whether it needs to continue reading the current chunk, move on to the next, or go back to the previous one. This control mechanism allows the system to use a minimal amount of RAM.

This mechanism was designed during development with a focus on environments like ATmega328, ATmega8, and similar microcontrollers. The final code of the interpreter has not yet been successfully ported to all these devices, but the architecture was designed with this in mind. All execution logic, dynamic allocation tests, and behavior limits were structured with the idea that partial code execution should always be possible.

## 8. Contributing to the project

This project is open to contributions. The goal is that anyone interested in languages, interpreters, or devices with limited resources can experiment, modify, propose changes, and share ideas.

If you want to contribute, here are some guidelines to keep the repository organized and facilitate the review process:

- **Make small and objective commits**
  Try to avoid bundling multiple changes into one commit. Prefer to send changes gradually, whenever possible with a clear description of what was done.

- **Include tests when changing the language behavior**
  If you are modifying an operator, adding a new function, or adjusting the syntax, create tests covering the new behavior. The `tests/` folder is already structured for this, with examples in `unit/`, `cli/`, and `bf/`.

- **Run the tests before submitting**
  Use the `make test` and `make memcheck` (or `make test-full`) targets to check if the language is still working correctly.

The project is open to suggestions, corrections, experiments, and contributions of different complexity levels. Even small improvements in code, tests, or documentation are welcome. If you have any questions about where to start, please contact us through issues or open a pull request with whatever you’re experimenting.

## 9. Acknowledgments

Here are some tools and people that made a difference during development:

- [**esolangs.org**](https://esolangs.org/), which was a gateway to the universe of esoteric languages.
- [**Urban Müller**](https://en.wikipedia.org/wiki/Brainfuck), creator of Brainfuck.
- [**Andy Wingo**](https://github.com/wingo/walloc), for `walloc` used in `wunstd`.
- [**Nathan Friedly**](https://github.com/nfriedly/miyoo-toolchain), for publishing the Docker image that made it possible to port the language to RG35XX (coming soon in `main`).
