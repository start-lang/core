#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path


Token = tuple[str, str]

TOKEN_SPECS = (
    ("COMMENT", r"//.*|/\*[\s\S]*?\*/"),
    ("STR", r'"([^"\\]|\\.)*"'),
    ("CHAR", r"'(\\.|[^'\\])'"),
    ("FLOAT", r"\d+\.\d+f?"),
    ("INT", r"\d+"),
    (
        "KW",
        r"\b(int|float|double|char|uint\w+|int\w+|if|else|while|do|for|"
        r"break|continue|return|printf|scanf|getchar|putchar)\b",
    ),
    ("ID", r"[a-zA-Z_]\w*"),
    (
        "OP",
        r"<<=|>>=|<<|>>|==|!=|<=|>=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|"
        r"\+\+|--|&&|\|\||[+\-*/%<>=&|^~!?:]",
    ),
    ("PUNC", r"[(){};,]"),
    ("HASH", r"#"),
    ("SPACE", r"\s+"),
)

TOKEN_RE = re.compile("|".join(f"(?P<{name}>{pattern})" for name, pattern in TOKEN_SPECS))

TYPE_MAP = {
    "int": "i",
    "float": "f",
    "double": "f",
    "char": "b",
    "uint16_t": "s",
    "uint32_t": "i",
    "int16_t": "s",
    "int8_t": "b",
    "uint8_t": "b",
}
C_TYPES = set(TYPE_MAP)
FOR_DECL_TYPES = {"int", "float", "uint16_t", "uint32_t"}
ASSIGN_OPS = {"=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "|=", "^="}
CMP_MAP = {"<": "?<", ">": "?>", "<=": "?l", ">=": "?g", "==": "?=", "!=": "?!"}
ARITH_MAP = {
    "+": "+",
    "-": "-",
    "*": "*",
    "/": "/",
    "%": "%",
    "&": "w&",
    "|": "w|",
    "^": "w^",
    "<<": "w<",
    ">>": "w>",
}
COMMUTATIVE_OPS = {"+", "*", "&", "|", "^"}
TYPE_WIDTHS = {"b": 1, "s": 2, "i": 4, "f": 4}
BINARY_LEVELS = (
    {"||"},
    {"&&"},
    {"|"},
    {"^"},
    {"&"},
    {"==", "!="},
    {"<", ">", "<=", ">="},
    {"<<", ">>"},
    {"+", "-"},
    {"*", "/", "%"},
)


def lex(code: str) -> list[Token]:
    return [
        (match.lastgroup or "", match.group(match.lastgroup or ""))
        for match in TOKEN_RE.finditer(code)
        if match.lastgroup not in {"SPACE", "HASH", "COMMENT"}
    ]


class Node:
    pass


@dataclass
class Program(Node): statements: list[Node]


@dataclass
class Decl(Node): type_name: str; declarations: list[tuple[str, Node | None]]


@dataclass
class Assign(Node): name: str; expr: Node


@dataclass
class BinOp(Node): op: str; left: Node; right: Node


@dataclass
class UnaryOp(Node): op: str; expr: Node; prefix: bool = True


@dataclass
class Num(Node): value: str; is_float: bool


@dataclass
class Id(Node): name: str


@dataclass
class If(Node): cond: Node; body: Program; else_body: Program | None


@dataclass
class While(Node): cond: Node; body: Program


@dataclass
class For(Node): init: Node; cond: Node; step: Node; body: Program


class Break(Node): pass


class Continue(Node): pass


@dataclass
class DoWhile(Node): cond: Node; body: Program


@dataclass
class Printf(Node): fmt: str; args: list[Node]


@dataclass
class Scanf(Node): fmt: str; args: list[Id]


class Getchar(Node): pass


@dataclass
class Putchar(Node): expr: Node


@dataclass
class Ternary(Node): cond: Node; then_expr: Node; else_expr: Node


@dataclass
class LogicOp(Node): op: str; left: Node; right: Node


@dataclass
class Not(Node): expr: Node


class ParseError(Exception): pass


class Parser:
    def __init__(self, tokens: list[Token]):
        self.tokens = tokens
        self.pos = 0

    def current(self) -> Token:
        if self.pos < len(self.tokens):
            return self.tokens[self.pos]
        return ("EOF", "")

    def peek(self, offset: int = 1) -> Token:
        index = self.pos + offset
        if index < len(self.tokens):
            return self.tokens[index]
        return ("EOF", "")

    def match(self, kind: str, value: str | None = None) -> bool:
        token = self.current()
        if token[0] == kind and (value is None or token[1] == value):
            self.pos += 1
            return True
        return False

    def expect(self, kind: str, value: str | None = None) -> Token:
        token = self.current()
        if not self.match(kind, value):
            expected = f"{kind} {value}" if value is not None else kind
            raise ParseError(f"expected {expected}, got {token}")
        return token

    def parse(self) -> Program:
        statements: list[Node] = []
        while self.current()[0] != "EOF":
            if self.current()[1] == "include":
                self.skip_include()
            elif self.is_main_start():
                self.skip_main_header()
            elif self.match("PUNC", "}"):
                continue
            elif self.match("KW", "return"):
                self.parse_expr()
                self.expect("PUNC", ";")
            else:
                statements.append(self.parse_stmt())
        return Program(statements)

    def skip_include(self) -> None:
        self.pos += 1
        while self.current()[0] != "EOF" and self.current()[1] != ">":
            self.pos += 1
        if self.current()[1] == ">":
            self.pos += 1

    def is_main_start(self) -> bool:
        return self.current() == ("KW", "int") and self.peek() == ("ID", "main")

    def skip_main_header(self) -> None:
        self.pos += 2
        self.expect("PUNC", "(")
        self.expect("PUNC", ")")
        self.expect("PUNC", "{")

    def parse_stmt(self) -> Node:
        token = self.current()
        value = token[1]
        if token[0] == "KW" and value in C_TYPES:
            return self.parse_decl()
        if value == "if":
            return self.parse_if()
        if value == "while":
            return self.parse_while()
        if value == "do":
            return self.parse_do_while()
        if value == "for":
            return self.parse_for()
        if value == "break":
            self.pos += 1
            self.expect("PUNC", ";")
            return Break()
        if value == "continue":
            self.pos += 1
            self.expect("PUNC", ";")
            return Continue()
        if value == "printf":
            return self.parse_printf()
        if value == "scanf":
            return self.parse_scanf()
        if value == "putchar":
            return self.parse_putchar()

        expr = self.parse_expr()
        self.expect("PUNC", ";")
        return expr

    def parse_decl(self) -> Decl:
        type_name = self.current()[1]
        self.pos += 1
        declarations: list[tuple[str, Node | None]] = []
        while True:
            name = self.expect("ID")[1]
            expr = self.parse_expr() if self.match("OP", "=") else None
            declarations.append((name, expr))
            if not self.match("PUNC", ","):
                break
        self.expect("PUNC", ";")
        return Decl(type_name, declarations)

    def parse_if(self) -> If:
        self.expect("KW", "if")
        self.expect("PUNC", "(")
        cond = self.parse_expr()
        self.expect("PUNC", ")")
        body = self.parse_block()
        else_body = self.parse_block() if self.match("KW", "else") else None
        return If(cond, body, else_body)

    def parse_while(self) -> While:
        self.expect("KW", "while")
        self.expect("PUNC", "(")
        cond = self.parse_expr()
        self.expect("PUNC", ")")
        return While(cond, self.parse_block())

    def parse_do_while(self) -> DoWhile:
        self.expect("KW", "do")
        body = self.parse_block()
        self.expect("KW", "while")
        self.expect("PUNC", "(")
        cond = self.parse_expr()
        self.expect("PUNC", ")")
        self.expect("PUNC", ";")
        return DoWhile(cond, body)

    def parse_for(self) -> For:
        self.expect("KW", "for")
        self.expect("PUNC", "(")
        init = self.parse_decl() if self.current()[1] in FOR_DECL_TYPES else self.parse_expr()
        if not isinstance(init, Decl):
            self.expect("PUNC", ";")
        cond = self.parse_expr()
        self.expect("PUNC", ";")
        step = self.parse_expr()
        self.expect("PUNC", ")")
        return For(init, cond, step, self.parse_block())

    def parse_printf(self) -> Printf:
        self.expect("KW", "printf")
        self.expect("PUNC", "(")
        fmt = self.expect("STR")[1][1:-1]
        args = []
        while self.match("PUNC", ","):
            args.append(self.parse_expr())
        self.expect("PUNC", ")")
        self.expect("PUNC", ";")
        return Printf(fmt, args)

    def parse_scanf(self) -> Scanf:
        self.expect("KW", "scanf")
        self.expect("PUNC", "(")
        fmt = self.expect("STR")[1][1:-1]
        args = []
        while self.match("PUNC", ","):
            self.match("OP", "&")
            args.append(Id(self.expect("ID")[1]))
        self.expect("PUNC", ")")
        self.expect("PUNC", ";")
        return Scanf(fmt, args)

    def parse_putchar(self) -> Putchar:
        self.expect("KW", "putchar")
        self.expect("PUNC", "(")
        expr = self.parse_expr()
        self.expect("PUNC", ")")
        self.expect("PUNC", ";")
        return Putchar(expr)

    def parse_block(self) -> Program:
        if self.match("PUNC", "{"):
            statements = []
            while not self.match("PUNC", "}"):
                statements.append(self.parse_stmt())
            return Program(statements)
        return Program([self.parse_stmt()])

    def parse_expr(self) -> Node:
        return self.parse_assign()

    def parse_assign(self) -> Node:
        left = self.parse_ternary()
        if self.current()[0] != "OP" or self.current()[1] not in ASSIGN_OPS:
            return left

        op = self.current()[1]
        self.pos += 1
        if not isinstance(left, Id):
            raise ParseError(f"assignment target must be an identifier, got {left!r}")
        if op == "=":
            return Assign(left.name, self.parse_assign())
        return Assign(left.name, BinOp(op[:-1], left, self.parse_ternary()))

    def parse_ternary(self) -> Node:
        cond = self.parse_binary(0)
        if not self.match("OP", "?"):
            return cond
        then_expr = self.parse_expr()
        self.expect("OP", ":")
        return Ternary(cond, then_expr, self.parse_assign())

    def parse_binary(self, level: int) -> Node:
        if level == len(BINARY_LEVELS):
            return self.parse_unary()

        node = self.parse_binary(level + 1)
        while self.current()[1] in BINARY_LEVELS[level]:
            op = self.current()[1]
            self.pos += 1
            right = self.parse_binary(level + 1)
            node = LogicOp(op, node, right) if op in {"&&", "||"} else BinOp(op, node, right)
        return node

    def parse_unary(self) -> Node:
        if self.current()[1] in {"++", "--"}:
            op = self.current()[1]
            self.pos += 1
            return UnaryOp(op, self.parse_primary(), prefix=True)
        if self.match("OP", "!"):
            return Not(self.parse_unary())
        if self.match("OP", "~"):
            return UnaryOp("~", self.parse_unary(), prefix=True)
        if self.match("OP", "-"):
            return BinOp("-", Num("0", False), self.parse_unary())

        node = self.parse_primary()
        if self.current()[1] in {"++", "--"}:
            op = self.current()[1]
            self.pos += 1
            return UnaryOp(op, node, prefix=False)
        return node

    def parse_primary(self) -> Node:
        token = self.current()
        if token == ("KW", "getchar"):
            self.pos += 1
            self.expect("PUNC", "(")
            self.expect("PUNC", ")")
            return Getchar()
        if token[0] == "ID":
            self.pos += 1
            return Id(token[1])
        if token[0] == "INT":
            self.pos += 1
            return Num(token[1], False)
        if token[0] == "FLOAT":
            self.pos += 1
            return Num(token[1].replace("f", ""), True)
        if token[0] == "CHAR":
            self.pos += 1
            return Num(parse_char_literal(token[1]), False)
        if self.match("PUNC", "("):
            expr = self.parse_expr()
            self.expect("PUNC", ")")
            return expr
        raise ParseError(f"unexpected expression token: {token}")


def parse_char_literal(token: str) -> str:
    content = token[1:-1]
    if not content.startswith("\\"):
        return str(ord(content))
    escapes = {"n": "10", "t": "9", "r": "13", "0": "0", "\\": "92", "'": "39", '"': "34"}
    return escapes.get(content[1], str(ord(content[1])))


@dataclass
class Value: name: str; type_name: str; is_lit: bool = False; is_tmp: bool = False


@dataclass
class Temp: name: str; type_name: str; used: bool = False


class CodeGen:
    def __init__(self):
        self.out: list[str] = []
        self.vars: dict[str, str] = {}
        self.tmp_indexes = {"i": 0, "f": 0, "s": 0, "b": 0}
        self.tmp_pool: list[Temp] = []
        self.pass_num = 2

    def emit(self, code: str) -> None:
        if self.pass_num == 2:
            self.out.append(code)

    def alloc_tmp(self, type_name: str) -> str:
        for tmp in self.tmp_pool:
            if tmp.type_name == type_name and not tmp.used:
                tmp.used = True
                return tmp.name

        index = self.tmp_indexes[type_name]
        self.tmp_indexes[type_name] += 1
        name = f"_T{type_name.upper()}{index}"
        self.vars[name] = type_name
        self.tmp_pool.append(Temp(name, type_name, used=True))
        return name

    def free_tmp(self, name: str) -> None:
        for tmp in self.tmp_pool:
            if tmp.name == name:
                tmp.used = False
                return

    def visit(self, node: Node) -> Value | None:
        method_name = f"visit_{type(node).__name__.lower()}"
        method = getattr(self, method_name, None)
        if method is None:
            raise TypeError(f"unsupported AST node: {type(node).__name__}")
        return method(node)

    def visit_program(self, node: Program) -> None:
        for statement in node.statements:
            self.visit(self.discard_statement_value(statement))

    def discard_statement_value(self, node: Node) -> Node:
        if isinstance(node, UnaryOp) and not node.prefix and node.op in {"++", "--"}:
            return UnaryOp(node.op, node.expr, prefix=True)
        return node

    def visit_decl(self, node: Decl) -> None:
        storage_type = TYPE_MAP.get(node.type_name, "i")
        for name, expr in node.declarations:
            var_name = name.upper()
            self.vars[var_name] = storage_type
            if expr is not None:
                self.store_expr(expr, var_name, storage_type)

    def visit_assign(self, node: Assign) -> Value:
        var_name = node.name.upper()
        storage_type = self.vars[var_name]
        self.store_expr(node.expr, var_name, storage_type)
        return Value(var_name, storage_type)

    def store_expr(self, expr: Node, var_name: str, storage_type: str) -> None:
        if isinstance(expr, BinOp) and expr.op in ARITH_MAP:
            self.emit_assign_binop(expr, var_name, storage_type)
            return

        result = self.visit(expr)
        if result is None:
            return
        self.load_to_reg(result, target_type=storage_type)
        self.cast_reg(result.type_name, storage_type)
        self.emit(storage_type)
        self.emit(f"{var_name}!")
        self.release(result)

    def visit_binop(self, node: BinOp) -> Value:
        if node.op in CMP_MAP:
            self.emit_cond(node)
            return Value("_ANS", "b")

        folded = self.fold_constants(node)
        if folded is not None:
            return folded

        left = self.visit(node.left)
        right = self.visit(node.right)
        if left is None or right is None:
            raise TypeError("binary operands must produce values")

        result_type = "f" if "f" in {left.type_name, right.type_name} else "i"
        result_name = self.alloc_tmp(result_type)

        self.load_to_reg(left, target_type=result_type)
        self.cast_reg(left.type_name, result_type)
        self.emit(result_type)
        self.emit(f"{result_name}!")

        self.load_to_reg(right, target_type=result_type)
        if right.type_name != result_type and not right.is_lit:
            self.cast_reg(right.type_name, result_type)
        self.emit(result_type)
        self.emit(f"{result_name}{ARITH_MAP[node.op]}")

        self.release(left)
        self.release(right)
        return Value(result_name, result_type, is_tmp=True)

    def fold_constants(self, node: BinOp) -> Value | None:
        if not isinstance(node.left, Num) or not isinstance(node.right, Num):
            return None

        left = float(node.left.value) if node.left.is_float else int(node.left.value)
        right = float(node.right.value) if node.right.is_float else int(node.right.value)
        is_float = node.left.is_float or node.right.is_float

        def integer(fn):
            return fn(int(left), int(right)), False

        folders = {
            "+": lambda: (left + right, is_float),
            "-": lambda: (left - right, is_float),
            "*": lambda: (left * right, is_float),
            "/": lambda: (left / right if is_float else int(left) // int(right), is_float),
            "%": lambda: integer(lambda a, b: a % b),
            "&": lambda: integer(lambda a, b: a & b),
            "|": lambda: integer(lambda a, b: a | b),
            "^": lambda: integer(lambda a, b: a ^ b),
            "<<": lambda: integer(lambda a, b: a << b),
            ">>": lambda: integer(lambda a, b: a >> b),
        }

        try:
            result, is_float = folders[node.op]()
        except (KeyError, ZeroDivisionError):
            return None

        if result < 0:
            return None
        value = str(result if is_float else int(result))
        return Value(value, "f" if is_float else "i", is_lit=True)

    def visit_unaryop(self, node: UnaryOp) -> Value:
        if not isinstance(node.expr, Id):
            raise TypeError("unary operators only support identifiers")
        var_name = node.expr.name.upper()
        storage_type = self.vars[var_name]
        op = "+" if node.op == "++" else "-"

        if node.prefix:
            self.emit(f"1 {storage_type}{var_name}; 1{op} {storage_type}{var_name};")
            return Value(var_name, storage_type)

        tmp = self.alloc_tmp(storage_type)
        self.emit(f"{storage_type}{var_name};")
        self.emit(f"{tmp}!")
        self.emit(f"1 {storage_type}{var_name}; 1{op} {storage_type}{var_name};")
        self.emit(f"{storage_type}{tmp};")
        self.free_tmp(tmp)
        return Value(tmp, storage_type, is_tmp=True)

    def visit_id(self, node: Id) -> Value:
        var_name = node.name.upper()
        return Value(var_name, self.vars[var_name])

    def visit_num(self, node: Num) -> Value:
        return Value(node.value, "f" if node.is_float else "i", is_lit=True)

    def visit_printf(self, node: Printf) -> None:
        fmt = node.fmt.replace("\\n", "")
        has_newline = "\\n" in node.fmt
        arg_index = 0
        index = 0

        while index < len(fmt):
            if fmt[index] == "%" and index + 1 < len(fmt) and arg_index < len(node.args):
                spec = fmt[index + 1]
                self.emit_printf_arg(spec, node.args[arg_index])
                arg_index += 1
                index += 2
                continue

            start = index
            while index < len(fmt) and not (fmt[index] == "%" and index + 1 < len(fmt)):
                index += 1
            if start < index:
                self.emit("i_STR_;")
                self.emit(f'"{fmt[start:index]}" PS')

        if has_newline:
            self.emit("b10@.@")

    def emit_printf_arg(self, spec: str, arg: Node) -> None:
        result = self.visit(arg)
        if result is None:
            return
        if spec in {"d", "f"}:
            self.load_to_reg(result)
            self.emit("PN")
        elif spec == "c":
            self.load_to_reg(result)
            self.emit("i_STR_! i_STR_;.")
        self.release(result)

    def visit_while(self, node: While) -> None:
        self.emit("t[")
        if not is_literal_one(node.cond):
            self.emit_cond(node.cond)
            self.emit("~(x:)")
        self.visit(node.body)
        self.emit("t]")

    def visit_for(self, node: For) -> None:
        self.visit(node.init)
        self.emit("t[")
        self.emit_cond(node.cond)
        self.emit("~(x:)")
        self.visit(node.body)
        self.visit(self.discard_statement_value(node.step))
        self.emit("t]")

    def visit_break(self, node: Break) -> None:
        self.emit("x:")

    def visit_continue(self, node: Continue) -> None:
        self.emit("c")

    def visit_dowhile(self, node: DoWhile) -> None:
        self.emit("t[")
        self.visit(node.body)
        if not is_literal_one(node.cond):
            self.emit_cond(node.cond)
            self.emit("~(x:)")
        self.emit("t]")

    def visit_if(self, node: If) -> None:
        self.emit_cond(node.cond)
        self.emit("(")
        self.visit(node.body)
        if node.else_body is not None:
            self.emit(":")
            self.visit(node.else_body)
        self.emit(")")

    def visit_getchar(self, node: Getchar) -> Value:
        self.ensure_input_buffer()
        self.emit("b_INPUT_,")
        return Value("_INPUT_", "b")

    def visit_putchar(self, node: Putchar) -> None:
        result = self.visit(node.expr)
        if result is None:
            return
        self.load_to_reg(result, target_type="b")
        self.emit("i_STR_! i_STR_;.")
        self.release(result)

    def visit_scanf(self, node: Scanf) -> None:
        self.ensure_input_buffer()
        arg_index = 0
        index = 0
        while index < len(node.fmt):
            if node.fmt[index] == "%" and index + 1 < len(node.fmt) and arg_index < len(node.args):
                spec = node.fmt[index + 1]
                var_name = node.args[arg_index].name.upper()
                if spec == "c":
                    self.emit(f"b{var_name},")
                elif spec == "d":
                    self.emit_read_int(var_name)
                arg_index += 1
                index += 2
            else:
                index += 1

    def visit_ternary(self, node: Ternary) -> Value:
        self.emit_cond(node.cond)
        self.emit("(")

        then_value = self.visit(node.then_expr)
        if then_value is None:
            raise TypeError("ternary branch must produce a value")
        tmp = self.alloc_tmp(then_value.type_name)
        self.load_to_reg(then_value, target_type=then_value.type_name)
        self.emit(f"{then_value.type_name}{tmp}!")
        self.release(then_value)

        self.emit(":")
        else_value = self.visit(node.else_expr)
        if else_value is None:
            raise TypeError("ternary branch must produce a value")
        self.load_to_reg(else_value, target_type=then_value.type_name)
        self.cast_reg(else_value.type_name, then_value.type_name)
        self.emit(f"{then_value.type_name}{tmp}!")
        self.release(else_value)

        self.emit(")")
        return Value(tmp, then_value.type_name, is_tmp=True)

    def visit_not(self, node: Not) -> Value:
        tmp = self.alloc_tmp("b")
        self.emit_cond(node.expr)
        self.emit("~")
        self.emit("(")
        self.emit(f"b1 b{tmp}!")
        self.emit(":")
        self.emit(f"b0 b{tmp}!")
        self.emit(")")
        return Value(tmp, "b", is_tmp=True)

    def visit_logicop(self, node: LogicOp) -> Value:
        tmp = self.alloc_tmp("b")
        if node.op == "&&":
            self.emit_cond(node.left)
            self.emit("(")
            self.emit_cond(node.right)
            self.emit("(")
            self.emit(f"b1 b{tmp}!")
            self.emit(":")
            self.emit(f"b0 b{tmp}!")
            self.emit(")")
            self.emit(":")
            self.emit(f"b0 b{tmp}!")
            self.emit(")")
        else:
            self.emit_cond(node.left)
            self.emit("(")
            self.emit(f"b1 b{tmp}!")
            self.emit(":")
            self.emit_cond(node.right)
            self.emit("(")
            self.emit(f"b1 b{tmp}!")
            self.emit(":")
            self.emit(f"b0 b{tmp}!")
            self.emit(")")
            self.emit(")")
        return Value(tmp, "b", is_tmp=True)

    def emit_cond(self, node: Node) -> None:
        if not isinstance(node, BinOp) or node.op not in CMP_MAP:
            result = self.visit(node)
            if result is not None:
                self.load_to_reg(result)
                self.emit("??")
            return

        left = self.visit(node.left)
        right = self.visit(node.right)
        if left is None or right is None:
            raise TypeError("comparison operands must produce values")

        cmp_type = self.comparison_type(left, right)
        if left.is_lit:
            tmp = self.alloc_tmp(cmp_type)
            self.load_to_reg(left, target_type=cmp_type)
            self.cast_reg(left.type_name, cmp_type)
            self.emit(f"{cmp_type}{tmp}!")
            left = Value(tmp, cmp_type, is_tmp=True)

        if left.type_name != cmp_type and not left.is_tmp:
            tmp = self.alloc_tmp(cmp_type)
            self.load_to_reg(left, target_type=cmp_type)
            self.cast_reg(left.type_name, cmp_type)
            self.emit(f"{cmp_type}{tmp}!")
            left = Value(tmp, cmp_type, is_tmp=True)

        self.load_to_reg(right, target_type=cmp_type)
        self.cast_reg(right.type_name, cmp_type)
        self.emit(cmp_type)
        self.emit(f"{left.type_name}{left.name}")
        self.emit(cmp_type)
        self.emit(CMP_MAP[node.op])

        self.release(left)
        self.release(right)

    def comparison_type(self, left: Value, right: Value) -> str:
        if "f" in {left.type_name, right.type_name}:
            return "f"
        left_width = TYPE_WIDTHS.get(left.type_name, 4)
        right_width = TYPE_WIDTHS.get(right.type_name, 4)
        return left.type_name if left_width >= right_width else right.type_name

    def load_to_reg(self, value: Value, target_type: str | None = None) -> None:
        load_type = target_type or value.type_name
        if value.is_lit:
            self.emit(f"{load_type}{value.name}")
            return

        source_width = TYPE_WIDTHS.get(value.type_name, 4)
        target_width = TYPE_WIDTHS.get(target_type, source_width) if target_type else source_width
        if target_width > source_width:
            self.emit("i0")
        self.emit(f"{value.type_name}{value.name};")

    def cast_reg(self, from_type: str, to_type: str) -> None:
        if from_type == to_type:
            return
        if from_type == "f" and to_type != "f":
            return
        if to_type == "f":
            if from_type == "i":
                self.emit("f")
            elif from_type in ("s", "b"):
                self.emit("i")
                self.emit("f")
            else:
                self.emit("ef")

    def emit_assign_binop(self, node: BinOp, var_name: str, storage_type: str) -> None:
        if node.op not in ARITH_MAP:
            self.store_expr(node, var_name, storage_type)
            return

        left_is_target = isinstance(node.left, Id) and node.left.name.upper() == var_name
        right_is_target = isinstance(node.right, Id) and node.right.name.upper() == var_name

        if left_is_target:
            self.emit_accumulate(node.right, node.op, var_name, storage_type)
        elif right_is_target and node.op in COMMUTATIVE_OPS:
            self.emit_accumulate(node.left, node.op, var_name, storage_type)
        elif not right_is_target:
            self.emit_assign_accumulator(node, var_name, storage_type)
        else:
            result = self.visit_binop(node)
            self.load_to_reg(result, target_type=storage_type)
            self.cast_reg(result.type_name, storage_type)
            self.emit(storage_type)
            self.emit(f"{var_name}!")
            self.release(result)

    def emit_accumulate(self, expr: Node, op: str, var_name: str, storage_type: str) -> None:
        value = self.visit(expr)
        if value is None:
            return
        result_type = "f" if value.type_name == "f" or storage_type == "f" else storage_type
        self.load_to_reg(value, target_type=result_type)
        if value.type_name != result_type and not value.is_lit:
            self.cast_reg(value.type_name, result_type)
        self.emit(result_type)
        self.emit(f"{var_name}{ARITH_MAP[op]}")
        self.release(value)

    def emit_assign_accumulator(self, node: BinOp, var_name: str, storage_type: str) -> None:
        left = self.visit(node.left)
        right = self.visit(node.right)
        if left is None or right is None:
            return

        result_type = "f" if "f" in {left.type_name, right.type_name} or storage_type == "f" else storage_type

        self.load_to_reg(left, target_type=result_type)
        self.cast_reg(left.type_name, result_type)
        self.emit(result_type)
        self.emit(f"{var_name}!")

        self.load_to_reg(right, target_type=result_type)
        if right.type_name != result_type and not right.is_lit:
            self.cast_reg(right.type_name, result_type)
        self.emit(result_type)
        self.emit(f"{var_name}{ARITH_MAP[node.op]}")

        self.release(left)
        self.release(right)

    def peephole_tokens(self, tokens: list[str]) -> list[str]:
        result = []
        current_type: str | None = "b"
        types = {"i", "b", "s", "f"}
        barriers = {"[", "]", "(", ")", ":", "c"}
        for token in tokens:
            if not token:
                continue
            if '"' in token:
                result.append(token)
                continue
            if token == "ef":
                result.append(token)
                current_type = "f"
                continue
            first = token[0]
            if first in types:
                if first == current_type:
                    rest = token[1:]
                    if not rest:
                        continue
                    token = rest
                else:
                    current_type = first
            result.append(token)
            if any(barrier in token for barrier in barriers):
                current_type = None
        return result

    def generate(self, ast: Program) -> str:
        self.pass_num = 1
        self.visit(ast)

        self.out = []
        self.pass_num = 2
        for tmp in self.tmp_pool:
            tmp.used = False

        for var_name, storage_type in self.vars.items():
            self.emit(f"{storage_type}{var_name}^ {storage_type}>")
        self.emit("i_STR_^")
        self.visit(ast)
        return " ".join(self.peephole_tokens(" ".join(self.out).split()))

    def ensure_input_buffer(self) -> None:
        if "_INPUT_" not in self.vars:
            self.vars["_INPUT_"] = "b"

    def emit_read_int(self, var_name: str) -> None:
        self.visit(Assign(var_name, Num("0", False)))
        self.emit("b_INPUT_,")
        self.emit("t[")
        self.visit(
            If(
                LogicOp(
                    "||",
                    BinOp("<", Id("_INPUT_"), Num("48", False)),
                    BinOp(">", Id("_INPUT_"), Num("57", False)),
                ),
                Program([Break()]),
                None,
            )
        )
        self.visit(
            Assign(
                var_name,
                BinOp(
                    "+",
                    BinOp("*", Id(var_name), Num("10", False)),
                    BinOp("-", Id("_INPUT_"), Num("48", False)),
                ),
            )
        )
        self.emit("b_INPUT_,")
        self.emit("t]")

    def release(self, value: Value) -> None:
        if value.is_tmp:
            self.free_tmp(value.name)


def is_literal_one(node: Node) -> bool: return isinstance(node, Num) and node.value == "1"


def transpile(code: str) -> str: return CodeGen().generate(Parser(lex(code)).parse())


def main() -> int:
    parser = argparse.ArgumentParser(description="transpile C subset to *T")
    parser.add_argument("source", type=Path)
    args = parser.parse_args()
    print(transpile(args.source.read_text(encoding="utf-8")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
