# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
  - Float constants
  - syntax: Token `STRING '"'`
  - syntax: Token `SCAPE '\'`
  - identifiers for variables
  - syntax: Token `NEW_VAR '^'`
  - runtime function declaration
  - syntax: Token `MALLOC 'm'`
  - syntax: Token `RETURN 'r'`
  - run code in tape
  - syntax: Token `RUN '#'`
  - syntax: Token `GOTO_ZERO 'z'`
### Changed
  - syntax: All upper case strings are now translated to api calls or memory jumps
  - syntax: Newline and tab are now treated as NOP
### Removed
  - syntax: Token `TYPE_SET 't'`
  - syntax: Token `OUT '.'`
  - syntax: Token `IN ','`
  - syntax: Token `MEM_JUMP 'm'`
  - syntax: Token `COPY_FROM 'v'`
  - syntax: Token `CODE_JMP 'j'`
  - B register

## [0.2.0] - 2017-09-18
### Added
  - Stack operators
  - syntax: Token `START_STACK '$'`
  - syntax: Token `PUSH 'p'`
  - syntax: Token `POP 'o'`
  - syntax: Token `STACK_HEIGHT 'h'`

### Changed
  - syntax: Token `CLEAR 'C'` is now `ZERO 'z'`
  - syntax: Token `C_ZERO 'O'` is now `z`

## [0.2.0] - 2017-09-18
### Changed
  - `run` function decomposed into `step`, `runall`, and `runblock`

## [0.1.3] - 2017-09-17
### Added
  - External api call
  - syntax: Token `STARTFUNCTION '{'`
  - syntax: Token `REMOTE_CALL '#'`
  - syntax: Token `REMOTE_REGISTER '$'`

### Changed
  - syntax: All upper case letter tokens are now translated to api calls
  - syntax: Token `T_INT16 'i'` is now `s`, standing for short integer
  - syntax: Token `T_INT32 'I'` is now `i`, standing for integer
  - syntax: Token `STORE 's'` is now `!`, referencing `OUT '.'`
  - syntax: Token `LOAD 'l'` is now `;`, referencing `IN ','`
  - syntax: Token `ENDFUNCTION ';'` is now `}`
