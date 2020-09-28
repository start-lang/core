#ifndef TOKENS_H
#define TOKENS_H

#ifdef NANO
#   define FUNCTION        'F' // unused
#   define STARTFUNCTION   '{' // unused
#   define ENDFUNCTION     '}' // unused
#   define RETURN          'r' // unused
#   define SET_MEM_SIZE    'm' // unused
#endif

#define IF              '('
#define ELSE            ':'
#define ENDIF           ')'
#define WHILE           '['
#define BREAK           'x'
#define CONTINUE        'c'
#define ENDWHILE        ']'
#define ARRAY           '{' // unused
#define ENDARRAY        '}' // unused

#define NOP             ' '
#define COND_MODIFIER   '?'
#define FUNCTION_SET    '#' // unused
#define LEFT            '<'
#define RIGHT           '>'

#define START_STACK     '$'
#define PUSH            'p'
#define POP             'o'
#define STACK_HEIGHT    'h'

#define ZERO            'z'
#define SWITCH          '@'
#define STORE           '!'
#define LOAD            ';'

#define OUT             '.'
#define IN              ','
#define MEM_JUMP        'm' //signed int // unused
#define COPY_FROM       'v' // unused
#define RUN             '^' // unused
#define CODE_JMP        'j' // unused


#define NEW_VAR         '^'

#define SUM             '+'
#define SUB             '-'
#define MULTI           '*'
#define DIV             '/'
#define MOD             '%'

#define C_EQ            '='
#define C_NEQ           '!'
#define C_LT            '<'
#define C_GT            '>'
#define C_LE            'l'
#define C_GE            'g'
#define C_NOT_NULL      '?'
#define C_ZERO          'z'

#define AND             '&'
#define OR              '|'
#define XOR             '^'
#define SHIFT_LEFT      '<'
#define SHIFT_RIGHT     '>'
#define INV             '~'

#define T_INT8          'b'
#define T_INT16         's'
#define T_INT32         'i'
#define T_FLOAT         'f'
#define STRING          '\"' // unused
#define CHAR            '\'' // unused
#define SCAPE           '\\' // unused


#define INT8    0
#define INT16   1
#define INT32   2
#define FLOAT   3


#define SYS_REALLOC  // unused
#define SYS_SW_DELAY // unused

#endif
