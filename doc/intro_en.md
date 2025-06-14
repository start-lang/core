\*T is an esoteric, structured and interpreted programming language that seeks to offer a creative challenge and an alternative paradigm for those who wish to explore the limits of programming. Most operators have only one character, allowing for compact syntax. In \*T, the focus is on creating almost encrypted codes and playing with new ways to solve problems. The idea is that the language values more the process and challenges encountered when programming, where the motto is "if it works, it's already good". Readability is part of the exploration form, which involves a deciphering game to move forward.

One of the main differentials of \*T is its portability, the interpreter was written in C with very few dependencies allowing experiments in environments with limited resources. Despite the limitations of interpreted languages, \*T offers an interesting functional environment for computational challenges on diverse platforms. The language's standard function library is still being standardized and may vary according to the environment or device, a set of functions that are simple to implement on different platforms is being thought out in a way that allows the creation of interesting programs, `.` (`PC` or `PRINT`) and `,` (`IN` or `INPUT`) allow textual input and output and should be present in all environments.

At the end of this page, an interpreter allows experimenting with the language's capabilities and exploring its compact syntax. Besides the online interpreter, the code can be compiled into an executable on various platforms. The idea is to port the project to portable gadgets that can be programmed directly on the device. Each development environment presents a different usability challenge for the interpreter, especially when we consider that not all devices will have a complete keyboard or a screen with good resolution, it's in these limitations that makes the challenge interesting.

This project was developed both for those who want a challenge when programming and experimenting with the limits of what can be done within a limited environment, and for those who want to create devices or port the language to new environments, trying to optimize usability and performance in each context.

```st
"Hello, World!" PS
```

> The examples on this page are executable - when hovering over them they are selected and you just need to expand the hidden terminal at the end of the page and click on the desired execution option: `Run` only executes and `Debug` performs operations step by step showing information about the interpreter state. To move to the next instruction you must press `space` or `enter`.

```st
9!>0!>1!?=[2<1-?!2>;<@>+] ;PN // Calculates the fibonacci of 9
```

Recently, the language was adapted to be compatible with Brainfuck, and being one of the most popular esoteric languages, basic concepts and techniques can be reused when programming in \*T. Brainfuck codes can be executed in the same way without adaptation. If this is the first time you explore an esolang similar to bf, below you will find a brief introduction.

```st
>,[>,]<[.<] // Reverses the input string
```

# Features

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

# Basic concepts

The main element in common with bf is the idea of representing memory as a tape/array consisting of "cells"/bytes, reading and manipulating the values of this tape is done through a read head/pointer, which receives instructions from the source code and performs actions accordingly. There are instructions to move the tape forward or backward, selecting new regions, performing mathematical operations and controlling program flow.

Every program starts at position zero of memory and can select other positions using the `>` and `<` operators. Numeric constants can be used preceding the operators, if there is no constant in use the value 1 is assumed.

**Example: move 3 bytes to the right**
  - in bf: ```>>>```
  - in \*T: ```3>```

The read head has some specific characteristics in \*T, it stores a register capable of storing values, type size in use and results of logical operations. These values can be used to simplify some operations. When a numeric constant is processed, it stays stored in the register, and may or may not be moved to the memory tape using the `!` operator. Similarly, values can be loaded into the register using `;`, or even swapped between register and memory using `@`.

Different numeric types are supported in \*T, integer types can have 3 sizes: 8, 16 and 32 bits, all unsigned. In addition to integer types, the 32-bit float type can also be used. By default the 8-bit integer type is used, to change the type you can use the operators: `b` (byte - uint8), `s` (short - uint16), `i` (int - uint32) and `f` (float).

When changing the numeric type in use, the displacement operators will move the tape by the size in bytes of each type: `b` - 1 byte, `s` - 2 bytes and 4 bytes for `i` and `f`.

**Example: types and displacement**
  - ```32!>``` or ```b32!>``` - stores 32 in the first byte and moves to the second
  - ```s256!>``` - stores 256 using the first two bytes and moves to the third
  - ```i20000!>``` - stores the value using the first 4 bytes and moves to the fifth
  - ```f3.1415!>``` - stores the value using the first 4 bytes and moves to the fifth

For input and output the operators `,` and `.` are used, respectively. `,` allows reading a character and storing it in the current position of the tape. `.` prints the value of the current position as an ASCII character. Additionally in \*T the `PN`/`PRINTNUM` function can be useful for printing the value in decimal.

**Example: print the number 7**
  - in bf: ``-[----->+<]>++++.`` or (55 repetitions of `+` with `.` at the end)
  - in \*T: ``7+ PN``

While in bf only the `+` and `-` operators are present, \*T also has `*`, `/` and `%`, and allows using numeric constants. This way some operations are simplified.

**Example: "3 + 4"**
  - in bf: ```+++ ++++```
  - in \*T: ```3+ 4+```

**Example: "2 * 3"**
  - in bf: ```++[>+++<-]```
    - note that it's necessary to use a repetition loop to perform a multiplication, divisions are even more laborious to "emulate"
  - in \*T: ```2+ 3*``` or ```2@3*``` (using switch) or ```2p3o*``` (using stack operators)

A detail worth showing is the use of comments. When `//` is found the program jumps to the next line break, and when `/*` is found it jumps to the next `*/`, allowing comments with more than one line or comments within the same line.

Strings can be loaded into the tape using `"`, when writing `"Hello World"`, for each character, the corresponding value from the ASCII table is loaded, at the end the tape is moved to the beginning of the string, allowing the use of the `PS`/`PRINTSTRING` function to print the values in ASCII until reaching a value equal to zero. Optionally it's possible to use the `>` operator at the end of the string in the form `"Hello World">` to avoid returning to the beginning of the string and continue using valid values from the tape.

**Example: Comments and Strings**
  - Comments until the end of the line:
```st
// My first program
"Hello World!" PS
// End
```
  - Comment with start and end:
```st
/*
   My first program
*/
"Hello World!" /* String */ PS /* PRINTSTRING function */
/* End */
```
 - Concatenates two Strings using identifiers:
```st
S^ /* marks the start */ "Hello">
// goes to the end of the string and ignores the 0 value returning one position
<" World!" S /* jumps to position S */ PS /* prints the string */
```

Repetition loops are defined by `[` and `]` where repetition only happens when a logical operation with positive result was performed. Logical operations are performed by the `?` operator followed by the operation. Logical operations are always performed between the tape value and the register value.

**Logical operators**
  - `t` - truth
  - `?>` - greater than
  - `?<` - less than
  - `?=` - equal
  - `?!` - different
  - `?l` - less than or equal
  - `?g` - greater than or equal
  - `??` - different from zero
  - `?z` - equal to zero
  - `h` - true if stack is greater than zero

> If a logical operator is not used before the start or end of a repetition loop, the value of the current memory position is used, where if it's different from zero the loop should repeat. Having the same behavior as bf.

Conditionals can be used to execute code blocks if the logical operation is true or false (if/else) using `(` `if_true` `)` or `(` `if_true` `:` `if_false` `)`.

**Example: Conditional and RAND function**
  - Generates a random number and compares with number 5, printing if it's greater or less/equal
  - ``RAND!10% ;PN 5?>(" > 5":" <= 5") PS``

To simplify memory management, *T offers a way to map tape positions to a specific identifier, emulating the concept of variables. Identifiers consist of one or more uppercase letters, which can optionally be interspersed with underscores and numbers. Declaration is done using `ID^` where ID is the identifier. After declaration, just use the identifier alone whenever you want to access the corresponding position.

An advantage when debugging code is that the type in use is also recorded (only in the debugger), allowing you to visualize the variable value more intuitively, especially when types different from the default (byte) are used.

Identifiers can also be used to call stdlib functions of the language, such as:
	•	PN/PRINTNUM — prints a number.
	•	PS/PRINTSTRING — prints a string.
	•	RAND — generates a random number.

# Summary

- NOP - No operation
  - Any whitespace character (space, tab and line break) is ignored
- Numeric constant
  - A sequence of numbers is always stored in the register
  - There are 4 commands used to define the type in use:
    - `b` 8bits unsigned - default
    - `s` 16bits unsigned
    - `i` 32bits unsigned
    - `f` float 32bits
  - Examples:
    - ```123``` ```b255``` ```s350``` ```f8.76```
- Memory operations
  - `<` - Move left by type size
  - `>` - Move right by type size
  - `!` - Copy register value to tape
  - `;` - Copy tape value to register
  - `@` - Swap tape value with register value and vice versa
  - `z` - Move 2 bytes to the right until finding a zero
  - `#` - Execute tape values as if they were commands until `0`
  - `m` - Reallocate tape size based on current register
- Mathematical operations
  - `+` - Add register value to tape value
  - `-` - Subtract register value from tape value
  - `*` - Multiply tape value by register value
  - `/` - Divide tape value by register value
  - `%` - Store remainder of tape value division by register value in tape
  - `w` - Introduces bitwise operations (&, |, ^, \~, <<, >>) based on active type
- String
  - Sequence of digits surrounded by double quotes `"`, internal double quotes must be preceded by `\`
  - Always saved on tape in sequence and terminated with value 0
  - This operation doesn't move the tape, but adding the `>` command in sequence moves the tape to the end of the string
- Identifiers
  - Any sequence of uppercase letters represents an identifier
  - Identifiers can contain `_` after the first letter
  - If the id is of a variable, then the tape is moved to it
  - If the id is of a function, then it is executed
  - In the online interpreter there are 3 implemented functions:
    - PC or PRINT:
      - Prints register value as character
    - PS or PRINTSTR:
      - Prints current tape as string (from current position to first `0`)
    - PN:
      - Prints register value as number (depends on type used)
- Variables
  - Tape positions can be mapped to ids using the `^` operator followed by the id
  - Example:
    - `X^1!>2!>3!X;` - saves the first tape position as id `X`, saves `1` in the first cell, `2` in the second and `3` in the third, returns to the first cell using the id name and loads the position value into the register, which is `1`
- Comparisons
  - There is an exclusive register for comparisons that is used by if and while operations
  - Comparison is made between tape (left) and register (right)
    - `t` - sets the exclusive register value as true
    - `?>` - greater than
    - `?<` - less than
    - `?=` - equal
    - `?!` - different
    - `?l` - less than or equal
    - `?g` - greater than or equal
    - `??` - different from zero
    - `?z` - equal to zero
    - `?h` - true if stack is greater than zero
    - `~` - inverts the exclusive register value
  - Examples:
    - `0!1?<` - Is `0` less than `1`? True
    - `1!1?<` - Is `1` less than `1`? False
    - `2!1?<` - Is `2` less than `1`? False
- If
  - Syntax: `(` `if_true` `)` or `(` `if_true` `:` `if_false` `)`
  - Uses the comparison register value to define if the first or second block should be executed
  - The first block is always executed if the value is `1`
  - The second block is executed if the value is `0` and the block has not been omitted
  - Example:
    - `2!1?>(2:3)!` - saves `2` if `2` is greater than `1` otherwise save `3`, therefore `2` is saved
- While
  - Syntax: `[` `block` `]`
  - `[` - enters the loop if the comparison register value is `1`
  - `]` - repeats if the comparison register value is `1`
  - `c` - *continue* - returns to the beginning of the loop
  - `x` - *break* - ends the loop
- Stack
  - `p` and `o` are equivalent to `!>` and `;<` respectively, but the stack height can be consulted using `h` and the condition `?h` is also available.
  - `p` - inserts an item on the stack, moving right by type size
  - `o` - removes an item from the stack, moving left by type size
  - `h` - stores stack size in register
- Functions
  - Besides mapping tape positions, ids can be used to declare functions
  - Syntax: `NAME` `{` `code` `}`
  - `r` - returns from function
- Comments
  - Multi-line or single-line comments can be used
  - Syntax: `/*` `multi-line content` `*/` or `//` `rest of line` `\n`

# How to use

The official interpreter can be compiled for Linux or Mac OS, a version compiled for WASM runs on this page. Other *targets* should be published in the next language updates, an Arduino/ESP32 compatible library (M5stack Cardputer) is in initial stages and a simple demo using SDL1.2/2 can be compiled for desktop and also the RG35xx portable console.

### Example: Drawing fractal in ASCII
The example below was converted from C to *T and allows drawing the Mandelbrot fractal using ASCII characters. On some devices the page may become unresponsive for a few seconds while it executes.

```st
s
LL^24!>L^!>
CC^70!>C^!>
f
CX^>
CY^>
TX^>
TY^>
TZ^>
X^>
Y^>
bI^
sL 1- [
  sCC;C! 1- [
    b0I!
    f0X!Y!
    i0sCC;efCX!3/ i0sC;efCX@/ 2-
    i0sLL;efCY!2/ i0sL;efCY@/ 1-
    bI 10! [
      f
      X;TX!*
      Y;TY!*
      0TZ! TX;TZ+ TY;TZ+ 4?> (
        x
      )
      Y 2*  X;Y*  CY;Y+
      TX;X! TY;X- CX;X+
      bI 1- ??
    ]
    bI;TZ! 38+ .
    sC 1- ??
  ]
  b10TZ! .
  sL 1- ??
]
```

Expected result:

```
.........-----,,,,,,+*)(&&&()*+++,,,,,,,,-------------------..../////
.......------,,,,++++)&&&&&&&&)*+++,,,,,,,,,------------------...////
......------,,++++**)(&&&&&&&&())***++,,,,,,,,-----------------...///
....-------,+*&())(&&&&&&&&&&&&&&'&&&'*+++,,,,,,-----------------..//
...-------,++&&&&&&&&&&&&&&&&&&&&&&&&')**++++++,,,,---------------.//
..-------,,+*)&&&&&&&&&&&&&&&&&&&&&&&&')***+++++++++,,-------------./
.-------,,++((&&&&&&&&&&&&&&&&&&&&&&&&&'()))*******)&*+,,,,---------/
.-------,,+*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'((&'&&'&&&&**+++,,,,,,----.
-------,,,+*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&')*+++++,,,,,,,--
-------,,,++)&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'((*+++++,,,,,,-
-------,,,++*)&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'))****+,,++,
-------,,,++*)(&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
-------,,,++*)&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'))****+,,++,
-------,,,++)&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'((*+++++,,,,,,-
-------,,,+*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&')*+++++,,,,,,,--
.-------,,+*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&'((&'&&'&&&&**+++,,,,,,----.
.-------,,++((&&&&&&&&&&&&&&&&&&&&&&&&&'()))*******)&*+,,,,---------/
..-------,,+*)&&&&&&&&&&&&&&&&&&&&&&&&')***+++++++++,,-------------./
...-------,++&&&&&&&&&&&&&&&&&&&&&&&&')**++++++,,,,---------------.//
....-------,+*&())(&&&&&&&&&&&&&&'&&&'*+++,,,,,,-----------------..//
......------,,++++**)(&&&&&&&&())***++,,,,,,,,-----------------...///
.......------,,,,++++)&&&&&&&&)*+++,,,,,,,,,------------------...////
.........-----,,,,,,+*)(&&&()*+++,,,,,,,,-------------------..../////
```

# REPL

Use the text box below to test the interpreter, just type the code and press `Run` or `Debug` at the end of the page to execute.

# Limitations

- Error codes are not yet documented
- The online version of the interpreter sometimes needs an extra key press to switch between debug and execution mode

# Next steps

> If you liked the idea visit the project repository on GitHub and give it a star, if the project is getting good visibility I can give it more attention for bug fixes, documentation and next releases <3
