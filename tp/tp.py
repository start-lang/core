#!/usr/bin/env python3
import sys, re

# --- LEXER ---
TOKENS = [
    ('STR', r'"([^"\\]|\\.)*"'),
    ('FLOAT', r'\d+\.\d+f?'),
    ('INT', r'\d+'),
    ('KW', r'\b(int|float|double|uint\w+|int\w+|if|else|while|for|break|return|printf)\b'),
    ('ID', r'[a-zA-Z_]\w*'),
    ('OP', r'==|!=|<=|>=|\+\+|--|&&|\|\||[+\-*/%<>=]'),
    ('PUNC', r'[(){};,]'),
    ('HASH', r'#'),
    ('SPACE', r'\s+'),
]
TOK_REGEX = '|'.join(f'(?P<{name}>{pat})' for name, pat in TOKENS)

def lex(code):
    return [(m.lastgroup, m.group(m.lastgroup)) for m in re.finditer(TOK_REGEX, code) if m.lastgroup not in ('SPACE', 'HASH')]

# --- AST NODES ---
class Node: pass
class Program(Node):
    def __init__(self, stmts): self.stmts = stmts
class Decl(Node):
    def __init__(self, type_, decls): self.type_ = type_; self.decls = decls
class Assign(Node):
    def __init__(self, id_, expr): self.id_ = id_; self.expr = expr
class BinOp(Node):
    def __init__(self, op, left, right): self.op = op; self.left = left; self.right = right
class UnaryOp(Node):
    def __init__(self, op, expr, prefix=True): self.op = op; self.expr = expr; self.prefix = prefix
class Num(Node):
    def __init__(self, val, is_float): self.val = val; self.is_float = is_float
class Id(Node):
    def __init__(self, name): self.name = name
class If(Node):
    def __init__(self, cond, body, else_body): self.cond = cond; self.body = body; self.else_body = else_body
class While(Node):
    def __init__(self, cond, body): self.cond = cond; self.body = body
class For(Node):
    def __init__(self, init, cond, step, body): self.init = init; self.cond = cond; self.step = step; self.body = body
class Break(Node): pass
class Printf(Node):
    def __init__(self, fmt, args): self.fmt = fmt; self.args = args

# --- PARSER ---
class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0

    def cur(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else ('EOF', '')
    
    def match(self, k, v=None):
        t = self.cur()
        if t[0] == k and (v is None or t[1] == v):
            self.pos += 1
            return True
        return False
        
    def expect(self, k, v=None):
        t = self.cur()
        if not self.match(k, v):
            raise Exception(f"Expected {k} {v}, got {t}")
        return t

    def parse(self):
        stmts = []
        while self.cur()[0] != 'EOF':
            if self.cur()[1] == 'include':
                self.pos += 1
                while self.cur()[1] != '>': self.pos += 1
                self.pos += 1
                continue
            if self.cur() == ('KW', 'int') and self.pos + 1 < len(self.tokens) and self.tokens[self.pos+1] == ('ID', 'main'):
                self.pos += 2
                self.expect('PUNC', '('); self.expect('PUNC', ')')
                self.expect('PUNC', '{')
                continue
            if self.match('PUNC', '}'): continue
            if self.match('KW', 'return'):
                self.parse_expr()
                self.expect('PUNC', ';')
                continue
            stmts.append(self.parse_stmt())
        return Program(stmts)

    def parse_stmt(self):
        t = self.cur()
        if t[0] == 'KW' and t[1] in ('int', 'float', 'double', 'uint16_t', 'uint32_t', 'uint8_t', 'int16_t', 'int32_t', 'int8_t'):
            return self.parse_decl()
        if t[1] == 'if': return self.parse_if()
        if t[1] == 'while': return self.parse_while()
        if t[1] == 'for': return self.parse_for()
        if t[1] == 'break':
            self.pos += 1; self.expect('PUNC', ';')
            return Break()
        if t[1] == 'printf': return self.parse_printf()
        
        expr = self.parse_expr()
        self.expect('PUNC', ';')
        return expr

    def parse_decl(self):
        t_type = self.cur()[1]
        self.pos += 1
        decls = []
        while True:
            id_ = self.cur()[1]
            self.expect('ID')
            expr = None
            if self.match('OP', '='):
                expr = self.parse_expr()
            decls.append((id_, expr))
            if self.match('PUNC', ','): continue
            break
        self.expect('PUNC', ';')
        return Decl(t_type, decls)

    def parse_if(self):
        self.expect('KW', 'if'); self.expect('PUNC', '(')
        cond = self.parse_expr()
        self.expect('PUNC', ')')
        body = self.parse_block()
        else_body = None
        if self.match('KW', 'else'): else_body = self.parse_block()
        return If(cond, body, else_body)

    def parse_while(self):
        self.expect('KW', 'while'); self.expect('PUNC', '(')
        cond = self.parse_expr()
        self.expect('PUNC', ')')
        return While(cond, self.parse_block())

    def parse_for(self):
        self.expect('KW', 'for'); self.expect('PUNC', '(')
        init = self.parse_decl() if self.cur()[1] in ('int', 'float', 'uint16_t', 'uint32_t') else self.parse_expr()
        if not isinstance(init, Decl): self.expect('PUNC', ';')
        cond = self.parse_expr(); self.expect('PUNC', ';')
        step = self.parse_expr(); self.expect('PUNC', ')')
        return For(init, cond, step, self.parse_block())

    def parse_printf(self):
        self.expect('KW', 'printf'); self.expect('PUNC', '(')
        fmt = self.cur()[1]; self.expect('STR')
        args = []
        while self.match('PUNC', ','): args.append(self.parse_expr())
        self.expect('PUNC', ')'); self.expect('PUNC', ';')
        return Printf(fmt[1:-1], args)

    def parse_block(self):
        if self.match('PUNC', '{'):
            stmts = []
            while not self.match('PUNC', '}'): stmts.append(self.parse_stmt())
            return Program(stmts)
        return Program([self.parse_stmt()])

    def parse_expr(self): return self.parse_assign()
    def parse_assign(self):
        left = self.parse_eq()
        if self.match('OP', '='): return Assign(left.name, self.parse_assign())
        return left
    def parse_eq(self):
        node = self.parse_rel()
        while self.cur()[1] in ('==', '!='):
            op = self.cur()[1]; self.pos += 1
            node = BinOp(op, node, self.parse_rel())
        return node
    def parse_rel(self):
        node = self.parse_add()
        while self.cur()[1] in ('<', '>', '<=', '>='):
            op = self.cur()[1]; self.pos += 1
            node = BinOp(op, node, self.parse_add())
        return node
    def parse_add(self):
        node = self.parse_mul()
        while self.cur()[1] in ('+', '-'):
            op = self.cur()[1]; self.pos += 1
            node = BinOp(op, node, self.parse_mul())
        return node
    def parse_mul(self):
        node = self.parse_unary()
        while self.cur()[1] in ('*', '/', '%'):
            op = self.cur()[1]; self.pos += 1
            node = BinOp(op, node, self.parse_unary())
        return node
    def parse_unary(self):
        if self.cur()[1] in ('++', '--'):
            op = self.cur()[1]; self.pos += 1
            return UnaryOp(op, self.parse_primary(), prefix=True)
        prim = self.parse_primary()
        if self.cur()[1] in ('++', '--'):
            op = self.cur()[1]; self.pos += 1
            return UnaryOp(op, prim, prefix=False)
        return prim
    def parse_primary(self):
        t = self.cur()
        if t[0] == 'ID':
            self.pos += 1; return Id(t[1])
        if t[0] == 'INT':
            self.pos += 1; return Num(t[1], False)
        if t[0] == 'FLOAT':
            self.pos += 1; return Num(t[1].replace('f',''), True)
        if self.match('PUNC', '('):
            expr = self.parse_expr(); self.expect('PUNC', ')'); return expr
        raise Exception(f"Unexpected in expr: {t}")

# --- CODE GEN ---
TYPE_MAP = { 'int':'i', 'float':'f', 'double':'f', 'uint16_t':'s', 'uint32_t':'i', 'int16_t':'s', 'int8_t':'b', 'uint8_t':'b' }
CMP_MAP = {'<': '?<', '>': '?>', '<=': '?l', '>=': '?g', '==': '?=', '!=': '?!'}
ARITH_MAP = {'+': '+', '-': '-', '*': '*', '/': '/', '%': '%'}

class CodeGen:
    def __init__(self):
        self.out = []
        self.vars = {}
        self.tmps = {'i': 0, 'f': 0, 's': 0, 'b': 0}
        self.tmp_pool = []
        self.tmp_counts = {'i': 0, 'f': 0, 's': 0, 'b': 0}
    
    def emit(self, code):
        if self.pass_num == 2:
            self.out.append(code)
        
    def alloc_tmp(self, t):
        for tmp in self.tmp_pool:
            if tmp['type'] == t and not tmp['used']:
                tmp['used'] = True
                return tmp['name']
        idx = self.tmps[t]; self.tmps[t] += 1
        name = f"_T{t.upper()}{idx}"
        self.vars[name] = t
        self.tmp_pool.append({'name': name, 'type': t, 'used': True})
        return name
        
    def free_tmp(self, name):
        for tmp in self.tmp_pool:
            if tmp['name'] == name:
                tmp['used'] = False
                break

    def load_to_reg(self, val, target_type=None):
        t = target_type if target_type else val['type']
        if val['is_lit']:
            self.emit(f"{t}{val['val']}")
        else:
            # For variables, always load with their original type
            self.emit(f"{val['type']}{val['val']};")

    def cast_reg(self, from_t, to_t):
        if from_t == 'f' and to_t != 'f': self.emit('e')
        if from_t != 'f' and to_t == 'f': self.emit('e')

    def visit(self, node):
        if isinstance(node, Program):
            for s in node.stmts: self.visit(s)
        elif isinstance(node, Decl):
            st = TYPE_MAP.get(node.type_, 'i')
            for id_, expr in node.decls:
                vn = id_.upper()
                self.vars[vn] = st
                if expr:
                    res = self.visit(expr)
                    self.load_to_reg(res)
                    self.cast_reg(res['type'], st)
                    self.emit(f"{vn}!")
        elif isinstance(node, Assign):
            vn = node.id_.upper()
            st = self.vars[vn]
            res = self.visit(node.expr)
            self.load_to_reg(res)
            self.cast_reg(res['type'], st)
            self.emit(f"{vn}!")
            if res['is_tmp']: self.free_tmp(res['val'])
            return {'val': vn, 'type': st, 'is_lit': False, 'is_tmp': False}
        elif isinstance(node, BinOp):
            if node.op in CMP_MAP:
                return self.emit_cond(node)
            l = self.visit(node.left)
            r = self.visit(node.right)
            res_t = 'f' if 'f' in (l['type'], r['type']) else 'i'
            res_v = self.alloc_tmp(res_t)
            
            self.load_to_reg(l, target_type=res_t)
            self.cast_reg(l['type'], res_t)
            self.emit(f"{res_v}!")
            
            self.load_to_reg(r, target_type=res_t)
            # Only cast if the original type is different from target type and not a literal
            if r['type'] != res_t and not r['is_lit']:
                self.cast_reg(r['type'], res_t)
            self.emit(f"{res_v}{ARITH_MAP[node.op]}")
            
            if l['is_tmp']: self.free_tmp(l['val'])
            if r['is_tmp']: self.free_tmp(r['val'])
            return {'val': res_v, 'type': res_t, 'is_lit': False, 'is_tmp': True}
        elif isinstance(node, UnaryOp):
            if node.prefix:
                vn = node.expr.name.upper()
                st = self.vars[vn]
                op = '+' if node.op == '++' else '-'
                self.emit(f"1 {st}{vn}; 1{op} {st}{vn};")
                return {'val': vn, 'type': st, 'is_lit': False, 'is_tmp': False}
            else:
                # Postfix: load current value first, then modify
                vn = node.expr.name.upper()
                st = self.vars[vn]
                op = '+' if node.op == '++' else '-'
                # Load current value to register (for return value)
                self.emit(f"{st}{vn};")
                # Store current value in a temp
                tmp = self.alloc_tmp(st)
                self.emit(f"{tmp}!")
                # Now decrement/increment the variable
                self.emit(f"1 {st}{vn}; 1{op} {st}{vn};")
                # Load temp back to register for return
                self.emit(f"{st}{tmp};")
                if tmp.startswith('_T'):
                    self.free_tmp(tmp)
                return {'val': tmp if tmp.startswith('_T') else vn, 'type': st, 'is_lit': False, 'is_tmp': tmp.startswith('_T')}
        elif isinstance(node, Id):
            vn = node.name.upper()
            return {'val': vn, 'type': self.vars[vn], 'is_lit': False, 'is_tmp': False}
        elif isinstance(node, Num):
            t = 'f' if node.is_float else 'i'
            return {'val': node.val, 'type': t, 'is_lit': True, 'is_tmp': False}
        elif isinstance(node, Printf):
            fmt = node.fmt.replace('\\n', '')
            has_nl = '\\n' in node.fmt
            arg_idx = 0
            i = 0
            while i < len(fmt):
                if fmt[i] == '%' and i + 1 < len(fmt) and arg_idx < len(node.args):
                    spec = fmt[i+1]
                    if spec in ('d', 'f'):
                        res = self.visit(node.args[arg_idx])
                        self.load_to_reg(res)
                        self.emit("PN")
                        arg_idx += 1
                    elif spec == 'c':
                        res = self.visit(node.args[arg_idx])
                        self.load_to_reg(res)
                        self.emit("i_STR_! i_STR_;.")
                        arg_idx += 1
                    i += 2
                else:
                    # Collect literal characters until next % or end
                    start = i
                    while i < len(fmt) and not (fmt[i] == '%' and i + 1 < len(fmt)):
                        i += 1
                    if start < i:
                        literal = fmt[start:i]
                        self.emit('i_STR_;')
                        self.emit(f'"{literal}" PS')
            if has_nl:
                self.emit('b10@.@')
        elif isinstance(node, While):
            self.emit('t[')
            if not (isinstance(node.cond, Num) and node.cond.val == '1'):
                self.emit_cond(node.cond)
                self.emit('~(x:)')
            self.visit(node.body)
            self.emit('t]')
        elif isinstance(node, For):
            self.visit(node.init)
            self.emit('t[')
            self.emit_cond(node.cond)
            self.emit('~(x:)')
            self.visit(node.body)
            self.visit(node.step)
            self.emit('t]')
        elif isinstance(node, Break):
            self.emit('x:') # start language break token is x: (or just x and then it requires :) actually let's use x: as in older transplier
        elif isinstance(node, If):
            self.emit_cond(node.cond)
            self.emit('(')
            self.visit(node.body)
            if node.else_body:
                self.emit(':')
                self.visit(node.else_body)
            self.emit(')')

    def emit_cond(self, node):
        if not isinstance(node, BinOp) or node.op not in CMP_MAP:
            res = self.visit(node)
            self.load_to_reg(res)
            self.emit('??')
            return
            
        l = self.visit(node.left)
        r = self.visit(node.right)
        
        if l['is_lit']:
            tmp = self.alloc_tmp(l['type'])
            self.emit(f"{l['type']}{l['val']} {tmp}!")
            l = {'val': tmp, 'type': l['type'], 'is_tmp': True}
            
        # Point memory to left operand first
        if l['is_lit'] or l['is_tmp']:
            self.emit(f"{l['type']}{l['val']};")
        else:
            self.emit(f"{l['type']}{l['val']}")
        
        # Load right operand into register
        self.load_to_reg(r)
        self.cast_reg(r['type'], l['type'])
        
        # CMP_MAP is '<': '?<'. The virtual machine compares MEM ? REG.
        # So memory should be Left, register should be Right.
        self.emit(CMP_MAP[node.op])
        
        if l['is_tmp']: self.free_tmp(l['val'])
        if r['is_tmp']: self.free_tmp(r['val'])

    def generate(self, ast):
        self.pass_num = 1
        self.visit(ast)
        self.out = []
        self.pass_num = 2
        for tmp in self.tmp_pool: tmp['used'] = False
        for vn, st in self.vars.items():
            self.emit(f"{st}{vn}^ {st}>")
        self.emit("i_STR_^")
        self.visit(ast)
        return '\n'.join(self.out)

if __name__ == '__main__':
    with open(sys.argv[1]) as f: code = f.read()
    tokens = lex(code)
    ast = Parser(tokens).parse()
    print(CodeGen().generate(ast))
