#!/usr/bin/env python3
import sys, re

# --- LEXER ---
TOKENS = [
    ('COMMENT', r'//.*|/\*[\s\S]*?\*/'),
    ('STR', r'"([^"\\]|\\.)*"'),
    ('CHAR', r"'(\\.|[^'\\])'"),
    ('FLOAT', r'\d+\.\d+f?'),
    ('INT', r'\d+'),
    ('KW', r'\b(int|float|double|char|uint\w+|int\w+|if|else|while|do|for|break|continue|return|printf|scanf|getchar|putchar)\b'),
    ('ID', r'[a-zA-Z_]\w*'),
    ('OP', r'<<|>>|==|!=|<=|>=|\+=|-=|\*=|/=|%=|\+\+|--|&&|\|\||[+\-*/%<>=&|^~!?:]'),
    ('PUNC', r'[(){};,]'),
    ('HASH', r'#'),
    ('SPACE', r'\s+'),
]
TOK_REGEX = '|'.join(f'(?P<{name}>{pat})' for name, pat in TOKENS)

def lex(code):
    return [(m.lastgroup, m.group(m.lastgroup)) for m in re.finditer(TOK_REGEX, code) if m.lastgroup not in ('SPACE', 'HASH', 'COMMENT')]

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
class Continue(Node): pass
class DoWhile(Node):
    def __init__(self, cond, body): self.cond = cond; self.body = body
class Printf(Node):
    def __init__(self, fmt, args): self.fmt = fmt; self.args = args
class Scanf(Node):
    def __init__(self, fmt, args): self.fmt = fmt; self.args = args
class Getchar(Node): pass
class Putchar(Node):
    def __init__(self, expr): self.expr = expr
class Ternary(Node):
    def __init__(self, cond, then_, else_): self.cond = cond; self.then_ = then_; self.else_ = else_
class LogicOp(Node):
    def __init__(self, op, left, right): self.op = op; self.left = left; self.right = right
class Not(Node):
    def __init__(self, expr): self.expr = expr

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
        if t[0] == 'KW' and t[1] in ('int', 'float', 'double', 'char', 'uint16_t', 'uint32_t', 'uint8_t', 'int16_t', 'int32_t', 'int8_t'):
            return self.parse_decl()
        if t[1] == 'if': return self.parse_if()
        if t[1] == 'while': return self.parse_while()
        if t[1] == 'do': return self.parse_do_while()
        if t[1] == 'for': return self.parse_for()
        if t[1] == 'break':
            self.pos += 1; self.expect('PUNC', ';')
            return Break()
        if t[1] == 'continue':
            self.pos += 1; self.expect('PUNC', ';')
            return Continue()
        if t[1] == 'printf': return self.parse_printf()
        if t[1] == 'scanf': return self.parse_scanf()
        if t[1] == 'putchar':
            self.pos += 1
            self.expect('PUNC', '(')
            expr = self.parse_expr()
            self.expect('PUNC', ')')
            self.expect('PUNC', ';')
            return Putchar(expr)
        
        # Debug: print current token before parsing expression
        # print(f"DEBUG parse_stmt: current token = {t}, pos={self.pos}")
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

    def parse_do_while(self):
        self.expect('KW', 'do')
        body = self.parse_block()
        self.expect('KW', 'while'); self.expect('PUNC', '(')
        cond = self.parse_expr()
        self.expect('PUNC', ')'); self.expect('PUNC', ';')
        return DoWhile(cond, body)

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

    def parse_scanf(self):
        self.expect('KW', 'scanf'); self.expect('PUNC', '(')
        fmt = self.cur()[1]; self.expect('STR')
        args = []
        while self.match('PUNC', ','):
            # Expect & followed by id (address-of)
            if self.cur() == ('OP', '&'):
                self.pos += 1
            # parse just an identifier (simple case)
            id_tok = self.cur(); self.expect('ID')
            args.append(Id(id_tok[1]))
        self.expect('PUNC', ')'); self.expect('PUNC', ';')
        return Scanf(fmt[1:-1], args)

    def parse_block(self):
        if self.match('PUNC', '{'):
            stmts = []
            while not self.match('PUNC', '}'): stmts.append(self.parse_stmt())
            return Program(stmts)
        return Program([self.parse_stmt()])

    def parse_expr(self): return self.parse_assign()
    def parse_assign(self):
        left = self.parse_ternary()
        if self.cur()[0] == 'OP' and self.cur()[1] in ('=', '+=', '-=', '*=', '/=', '%='):
            op = self.cur()[1]
            self.pos += 1
            if op == '=':
                return Assign(left.name, self.parse_assign())
            else:
                # Compound assignment: a += b -> a = a + b
                # Convert compound op to binary op
                bin_op = op[0]  # '+', '-', '*', '/', '%'
                return Assign(left.name, BinOp(bin_op, left, self.parse_ternary()))
        return left
    def parse_ternary(self):
        cond = self.parse_logor()
        if self.match('OP', '?'):
            then_ = self.parse_expr()
            self.expect('OP', ':')
            else_ = self.parse_assign()
            return Ternary(cond, then_, else_)
        return cond
    def parse_logor(self):
        node = self.parse_logand()
        while self.cur()[1] == '||':
            self.pos += 1
            node = LogicOp('||', node, self.parse_logand())
        return node
    def parse_logand(self):
        node = self.parse_bitor()
        while self.cur()[1] == '&&':
            self.pos += 1
            node = LogicOp('&&', node, self.parse_bitor())
        return node
    def parse_bitor(self):
        node = self.parse_bitxor()
        while self.cur()[1] == '|':
            self.pos += 1
            node = BinOp('|', node, self.parse_bitxor())
        return node
    def parse_bitxor(self):
        node = self.parse_bitand()
        while self.cur()[1] == '^':
            self.pos += 1
            node = BinOp('^', node, self.parse_bitand())
        return node
    def parse_bitand(self):
        node = self.parse_eq()
        while self.cur()[1] == '&':
            self.pos += 1
            node = BinOp('&', node, self.parse_eq())
        return node
    def parse_eq(self):
        node = self.parse_rel()
        while self.cur()[1] in ('==', '!='):
            op = self.cur()[1]; self.pos += 1
            node = BinOp(op, node, self.parse_rel())
        return node
    def parse_rel(self):
        node = self.parse_shift()
        while self.cur()[1] in ('<', '>', '<=', '>='):
            op = self.cur()[1]; self.pos += 1
            node = BinOp(op, node, self.parse_shift())
        return node
    def parse_shift(self):
        node = self.parse_add()
        while self.cur()[1] in ('<<', '>>'):
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
        if self.cur()[1] == '!':
            self.pos += 1
            return Not(self.parse_unary())
        if self.cur()[1] == '~':
            self.pos += 1
            return UnaryOp('~', self.parse_unary(), prefix=True)
        if self.cur()[1] == '-':
            self.pos += 1
            # unary minus: 0 - expr
            return BinOp('-', Num('0', False), self.parse_unary())
        prim = self.parse_primary()
        if self.cur()[1] in ('++', '--'):
            op = self.cur()[1]; self.pos += 1
            return UnaryOp(op, prim, prefix=False)
        return prim
    def parse_primary(self):
        t = self.cur()
        if t[0] == 'KW' and t[1] == 'getchar':
            self.pos += 1
            self.expect('PUNC', '('); self.expect('PUNC', ')')
            return Getchar()
        if t[0] == 'ID':
            self.pos += 1; return Id(t[1])
        if t[0] == 'INT':
            self.pos += 1; return Num(t[1], False)
        if t[0] == 'FLOAT':
            self.pos += 1; return Num(t[1].replace('f',''), True)
        if t[0] == 'CHAR':
            self.pos += 1
            # Parse char literal, e.g. 'a', '\n', '\t'
            s = t[1][1:-1]  # strip quotes
            if s.startswith('\\'):
                escapes = {'n':'10', 't':'9', 'r':'13', '0':'0', '\\':'92', '\'':'39', '"':'34'}
                val = escapes.get(s[1], str(ord(s[1])))
            else:
                val = str(ord(s))
            return Num(val, False)
        if self.match('PUNC', '('):
            expr = self.parse_expr(); self.expect('PUNC', ')'); return expr
        raise Exception(f"Unexpected in expr: {t}")

# --- CODE GEN ---
TYPE_MAP = { 'int':'i', 'float':'f', 'double':'f', 'char':'b', 'uint16_t':'s', 'uint32_t':'i', 'int16_t':'s', 'int8_t':'b', 'uint8_t':'b' }
CMP_MAP = {'<': '?<', '>': '?>', '<=': '?l', '>=': '?g', '==': '?=', '!=': '?!'}
# Bitwise ops use 'w' prefix in Start language
ARITH_MAP = {'+': '+', '-': '-', '*': '*', '/': '/', '%': '%',
             '&': 'w&', '|': 'w|', '^': 'w^', '<<': 'w<', '>>': 'w>'}

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
        if val.get('on_stack', False):
            # Value is on stack, pop it to register
            self.emit("o")
        elif val['is_lit']:
            # For literals, load with the target type
            self.emit(f"{t}{val['val']}")
        else:
            # For variables and temps, load with their original type
            # If target is wider than source, clear high bytes first with i0
            # Type widths: b=1, s=2, i=4, f=4
            widths = {'b': 1, 's': 2, 'i': 4, 'f': 4}
            src_w = widths.get(val['type'], 4)
            tgt_w = widths.get(target_type, src_w) if target_type else src_w
            if tgt_w > src_w:
                self.emit("i0")
            self.emit(f"{val['type']}{val['val']};")

    def cast_reg(self, from_t, to_t):
        # from_t -> to_t conversion in register
        if from_t == to_t: return
        if from_t == 'f' and to_t != 'f':
            # float -> int: the following type token already performs the
            # conversion in the runtime, so emitting TYPE_CAST here would
            # convert the register twice and truncate values to 0.
            return
        elif to_t == 'f':
            # int -> float
            if from_t == 'i':
                # i32 -> f32: just use 'f' type op (which auto-converts from int32)
                # Actually we need to emit nothing here, the caller will emit 'f' type
                self.emit('f')
            else:
                # i8/i16 -> float: need 'e' (reg.f32 = (float)reg.i32), then explicit f
                # Assumes high bytes are clean (load_to_reg emits i0 before load)
                self.emit('ef')

    def visit(self, node):
        if isinstance(node, Program):
            for s in node.stmts:
                # Statement-level postfix ++/-- discards the return value;
                # use the cheaper prefix path (no temp save/restore).
                if isinstance(s, UnaryOp) and not s.prefix and s.op in ('++', '--'):
                    self.visit(UnaryOp(s.op, s.expr, prefix=True))
                else:
                    self.visit(s)
        elif isinstance(node, Decl):
            st = TYPE_MAP.get(node.type_, 'i')
            for id_, expr in node.decls:
                vn = id_.upper()
                self.vars[vn] = st
                if expr:
                    if isinstance(expr, BinOp) and expr.op in ARITH_MAP:
                        self.emit_assign_binop(expr, vn, st)
                    else:
                        res = self.visit(expr)
                        if res.get('on_stack', False):
                            self.emit(f"{st}o {vn}!")
                        else:
                            self.load_to_reg(res, target_type=st)
                            self.cast_reg(res['type'], st)
                            self.emit(f"{st}")
                            self.emit(f"{vn}!")
                        if res.get('is_tmp', False): self.free_tmp(res['val'])
        elif isinstance(node, Assign):
            vn = node.id_.upper()
            st = self.vars[vn]
            if isinstance(node.expr, BinOp) and node.expr.op in ARITH_MAP:
                self.emit_assign_binop(node.expr, vn, st)
            else:
                res = self.visit(node.expr)
                if res.get('on_stack', False):
                    self.emit(f"{st}o {vn}!")
                else:
                    self.load_to_reg(res, target_type=st)
                    self.cast_reg(res['type'], st)
                    self.emit(f"{st}")
                    self.emit(f"{vn}!")
                if res['is_tmp']: self.free_tmp(res['val'])
            return {'val': vn, 'type': st, 'is_lit': False, 'is_tmp': False}
        elif isinstance(node, BinOp):
            if node.op in CMP_MAP:
                return self.emit_cond(node)
            # Constant folding: evaluate literal-only expressions at compile time
            if isinstance(node.left, Num) and isinstance(node.right, Num):
                lv = float(node.left.val) if node.left.is_float else int(node.left.val)
                rv = float(node.right.val) if node.right.is_float else int(node.right.val)
                is_f = node.left.is_float or node.right.is_float
                if node.op == '+':   result = lv + rv
                elif node.op == '-': result = lv - rv
                elif node.op == '*': result = lv * rv
                elif node.op == '/' and rv != 0:
                    result = lv / rv if is_f else int(lv) // int(rv)
                elif node.op == '%' and rv != 0:
                    result = int(lv) % int(rv); is_f = False
                elif node.op == '&':  result = int(lv) & int(rv);  is_f = False
                elif node.op == '|':  result = int(lv) | int(rv);  is_f = False
                elif node.op == '^':  result = int(lv) ^ int(rv);  is_f = False
                elif node.op == '<<': result = int(lv) << int(rv); is_f = False
                elif node.op == '>>': result = int(lv) >> int(rv); is_f = False
                else: result = None
                # Start language has no negative literal syntax; skip folding
                if result is not None and result >= 0:
                    t = 'f' if is_f else 'i'
                    return {'val': str(int(result) if not is_f else result),
                            'type': t, 'is_lit': True, 'is_tmp': False}
            l = self.visit(node.left)
            r = self.visit(node.right)
            res_t = 'f' if 'f' in (l['type'], r['type']) else 'i'
            res_v = self.alloc_tmp(res_t)
            
            # Load left operand into register and cast to result type
            self.load_to_reg(l, target_type=res_t)
            self.cast_reg(l['type'], res_t)
            # Ensure type is res_t before store (load may have changed _type)
            self.emit(f"{res_t}")
            self.emit(f"{res_v}!")
            
            # Load right operand into register and cast
            self.load_to_reg(r, target_type=res_t)
            if r['type'] != res_t and not r['is_lit']:
                self.cast_reg(r['type'], res_t)
            # Ensure type is res_t before math op
            self.emit(f"{res_t}")
            # Perform operation: mem(res_v) OP reg(right) -> mem(res_v)
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
            # For-step result is always discarded; treat postfix as prefix.
            step = node.step
            if isinstance(step, UnaryOp) and not step.prefix and step.op in ('++', '--'):
                step = UnaryOp(step.op, step.expr, prefix=True)
            self.visit(step)
            self.emit('t]')
        elif isinstance(node, Break):
            self.emit('x:') # start language break token is x: (or just x and then it requires :) actually let's use x: as in older transplier
        elif isinstance(node, Continue):
            self.emit('c')
        elif isinstance(node, DoWhile):
            self.emit('t[')
            self.visit(node.body)
            if not (isinstance(node.cond, Num) and node.cond.val == '1'):
                self.emit_cond(node.cond)
                self.emit('~(x:)')
            self.emit('t]')
        elif isinstance(node, If):
            self.emit_cond(node.cond)
            self.emit('(')
            self.visit(node.body)
            if node.else_body:
                self.emit(':')
                self.visit(node.else_body)
            self.emit(')')
        elif isinstance(node, Getchar):
            # Read one byte into _INPUT_ buffer, return it as byte
            self.ensure_input_buf()
            # Point _m to _INPUT_, emit `,` to read, keep result in memory
            self.emit('b_INPUT_,')
            # Return as byte value (caller will load via load_to_reg)
            return {'val': '_INPUT_', 'type': 'b', 'is_lit': False, 'is_tmp': False}
        elif isinstance(node, Putchar):
            # Write one char to stdout
            res = self.visit(node.expr)
            self.load_to_reg(res, target_type='b')
            # To print char: store to _STR_ as single byte, then print string of 1
            # Simpler: use the same trick as printf %c
            self.emit('i_STR_! i_STR_;.')
            if res.get('is_tmp', False): self.free_tmp(res['val'])
        elif isinstance(node, Scanf):
            self.ensure_input_buf()
            fmt = node.fmt
            arg_idx = 0
            i = 0
            while i < len(fmt):
                if fmt[i] == '%' and i + 1 < len(fmt) and arg_idx < len(node.args):
                    spec = fmt[i+1]
                    vn = node.args[arg_idx].name.upper()
                    st = self.vars[vn]
                    if spec == 'c':
                        # Read one char directly into var
                        self.emit(f'b{vn},')
                    elif spec == 'd':
                        # Read decimal integer (accumulate digits)
                        self.emit_read_int(vn, st)
                    arg_idx += 1
                    i += 2
                else:
                    # Skip literal chars in format (for scanf, these are whitespace to skip)
                    i += 1
        elif isinstance(node, Ternary):
            # Evaluate cond; if true, store then_ result, else store else_
            # Use a tmp to hold the result
            # First evaluate branches to infer type
            # Emit: cond (then_ to tmp : else_ to tmp)
            self.emit_cond(node.cond)
            self.emit('(')
            t_res = self.visit(node.then_)
            tmp = self.alloc_tmp(t_res['type'])
            self.load_to_reg(t_res, target_type=t_res['type'])
            self.cast_reg(t_res['type'], t_res['type'])
            self.emit(f"{t_res['type']}{tmp}!")
            if t_res.get('is_tmp', False): self.free_tmp(t_res['val'])
            self.emit(':')
            e_res = self.visit(node.else_)
            self.load_to_reg(e_res, target_type=t_res['type'])
            self.cast_reg(e_res['type'], t_res['type'])
            self.emit(f"{t_res['type']}{tmp}!")
            if e_res.get('is_tmp', False): self.free_tmp(e_res['val'])
            self.emit(')')
            return {'val': tmp, 'type': t_res['type'], 'is_lit': False, 'is_tmp': True}
        elif isinstance(node, Not):
            # Logical not: returns 0 or 1 based on whether expr is nonzero
            tmp = self.alloc_tmp('b')
            # Use emit_cond to set _ans, then invert
            self.emit_cond(node.expr)
            self.emit('~')  # invert _ans
            self.emit('(')
            self.emit(f'b1 b{tmp}!')
            self.emit(':')
            self.emit(f'b0 b{tmp}!')
            self.emit(')')
            return {'val': tmp, 'type': 'b', 'is_lit': False, 'is_tmp': True}
        elif isinstance(node, LogicOp):
            # Short-circuit logic: && and ||
            # Result is a byte (0 or 1)
            tmp = self.alloc_tmp('b')
            if node.op == '&&':
                # result = (left) ? (right ? 1 : 0) : 0
                self.emit_cond(node.left)
                self.emit('(')
                self.emit_cond(node.right)
                self.emit('(')
                self.emit(f'b1 b{tmp}!')
                self.emit(':')
                self.emit(f'b0 b{tmp}!')
                self.emit(')')
                self.emit(':')
                self.emit(f'b0 b{tmp}!')
                self.emit(')')
            else:  # ||
                self.emit_cond(node.left)
                self.emit('(')
                self.emit(f'b1 b{tmp}!')
                self.emit(':')
                self.emit_cond(node.right)
                self.emit('(')
                self.emit(f'b1 b{tmp}!')
                self.emit(':')
                self.emit(f'b0 b{tmp}!')
                self.emit(')')
                self.emit(')')
            return {'val': tmp, 'type': 'b', 'is_lit': False, 'is_tmp': True}

    def emit_cond(self, node):
        if not isinstance(node, BinOp) or node.op not in CMP_MAP:
            res = self.visit(node)
            self.load_to_reg(res)
            self.emit('??')
            return
            
        l = self.visit(node.left)
        r = self.visit(node.right)
        
        # Determine comparison type: if either is float, use float; else use widest
        if 'f' in (l['type'], r['type']):
            cmp_t = 'f'
        else:
            widths = {'b': 1, 's': 2, 'i': 4}
            lw = widths.get(l['type'], 4)
            rw = widths.get(r['type'], 4)
            cmp_t = l['type'] if lw >= rw else r['type']
        
        if l['is_lit']:
            tmp = self.alloc_tmp(cmp_t)
            self.load_to_reg(l, target_type=cmp_t)
            self.cast_reg(l['type'], cmp_t)
            self.emit(f"{cmp_t}{tmp}!")
            l = {'val': tmp, 'type': cmp_t, 'is_tmp': True, 'is_lit': False}
            
        # Point memory to left operand (set _type to cmp_t and move _m to l)
        if l['type'] != cmp_t and not l['is_tmp']:
            # Need to cast via register then store to tmp
            tmp = self.alloc_tmp(cmp_t)
            self.load_to_reg(l, target_type=cmp_t)
            self.cast_reg(l['type'], cmp_t)
            self.emit(f"{cmp_t}{tmp}!")
            l = {'val': tmp, 'type': cmp_t, 'is_tmp': True, 'is_lit': False}
        
        # Load right operand into register with cmp_t FIRST
        # (since loading moves _m, we need to point to left AFTER)
        self.load_to_reg(r, target_type=cmp_t)
        self.cast_reg(r['type'], cmp_t)
        # Ensure _type is cmp_t for comparison
        self.emit(f"{cmp_t}")
        
        # Point memory to left operand (just move _m, no load)
        self.emit(f"{l['type']}{l['val']}")
        # Set _type to cmp_t for comparison (may differ from left's type)
        self.emit(f"{cmp_t}")
        
        # CMP: mem(left) ? reg(right)
        self.emit(CMP_MAP[node.op])
        
        if l['is_tmp']: self.free_tmp(l['val'])
        if r['is_tmp']: self.free_tmp(r['val'])

    def peephole_tokens(self, tokens):
        """Remove standalone type tokens that repeat the already-current type."""
        TYPE_CHARS = {'i', 'b', 's', 'f'}
        result = []
        last_type = None
        for tok in tokens:
            if not tok:
                continue
            first = tok[0]
            if first in TYPE_CHARS:
                new_type = first
                if len(tok) == 1 and new_type == last_type:
                    continue  # redundant standalone type token
                last_type = new_type
            result.append(tok)
        return result

    def emit_assign_binop(self, bop, vn, st):
        """Emit optimized code for vn = bop (arithmetic BinOp only).

        Three cases in order of preference:
          1. a = a OP b  →  load b; a OP=
          2. a = b OP a  (commutative) →  load b; a OP=
          3. a = b OP c  →  load b; a=b; load c; a OP=  (use a as accumulator)
          fallback: use a temp (for e.g. a = b - a, non-commutative + right==target)
        """
        if bop.op not in ARITH_MAP:
            res = self.visit(bop)
            self.load_to_reg(res, target_type=st)
            self.cast_reg(res['type'], st)
            self.emit(st)
            self.emit(f"{vn}!")
            if res.get('is_tmp'): self.free_tmp(res['val'])
            return

        COMMUTATIVE = ('+', '*', '&', '|', '^')
        left_is_vn  = isinstance(bop.left, Id)  and bop.left.name.upper()  == vn
        right_is_vn = isinstance(bop.right, Id) and bop.right.name.upper() == vn

        if left_is_vn:
            # a = a OP b
            r = self.visit(bop.right)
            res_t = 'f' if (r['type'] == 'f' or st == 'f') else st
            self.load_to_reg(r, target_type=res_t)
            if r['type'] != res_t and not r['is_lit']:
                self.cast_reg(r['type'], res_t)
            self.emit(res_t)
            self.emit(f"{vn}{ARITH_MAP[bop.op]}")
            if r.get('is_tmp'): self.free_tmp(r['val'])

        elif right_is_vn and bop.op in COMMUTATIVE:
            # a = b OP a  (commutative → treat as a = a OP b)
            l = self.visit(bop.left)
            res_t = 'f' if (l['type'] == 'f' or st == 'f') else st
            self.load_to_reg(l, target_type=res_t)
            if l['type'] != res_t and not l['is_lit']:
                self.cast_reg(l['type'], res_t)
            self.emit(res_t)
            self.emit(f"{vn}{ARITH_MAP[bop.op]}")
            if l.get('is_tmp'): self.free_tmp(l['val'])

        elif not right_is_vn:
            # a = b OP c  — use a as accumulator (no separate temp needed)
            l = self.visit(bop.left)
            r = self.visit(bop.right)
            res_t = 'f' if ('f' in (l['type'], r['type']) or st == 'f') else st

            self.load_to_reg(l, target_type=res_t)
            self.cast_reg(l['type'], res_t)
            self.emit(res_t)
            self.emit(f"{vn}!")

            self.load_to_reg(r, target_type=res_t)
            if r['type'] != res_t and not r['is_lit']:
                self.cast_reg(r['type'], res_t)
            self.emit(res_t)
            self.emit(f"{vn}{ARITH_MAP[bop.op]}")

            if l.get('is_tmp'): self.free_tmp(l['val'])
            if r.get('is_tmp'): self.free_tmp(r['val'])

        else:
            # a = b OP a, non-commutative (e.g. a = b - a) — must use temp
            res = self.visit(bop)
            self.load_to_reg(res, target_type=st)
            self.cast_reg(res['type'], st)
            self.emit(st)
            self.emit(f"{vn}!")
            if res.get('is_tmp'): self.free_tmp(res['val'])

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
        tokens = ' '.join(self.out).split()
        tokens = self.peephole_tokens(tokens)
        return ' '.join(tokens)

    def ensure_input_buf(self):
        """Ensure _INPUT_ byte var exists for reading input."""
        if '_INPUT_' not in self.vars:
            self.vars['_INPUT_'] = 'b'

    def emit_read_int(self, vn, st):
        """Generate code to read a decimal integer from stdin into variable vn.
        Reads digit chars, accumulating: VAR = VAR*10 + (c - '0').
        Stops when a non-digit is read.
        Uses AST-level primitives through visit() for correct type handling.
        """
        # Initialize VAR = 0 : visit Assign(vn, Num(0))
        self.visit(Assign(vn, Num('0', False)))
        # Read first char
        self.emit('b_INPUT_,')
        # Loop: while char is in '0'..'9' accumulate and read next
        self.emit('t[')
        # Compare c < '0' or c > '9' -> break
        # Break if _INPUT_ < 48 or _INPUT_ > 57
        self.visit(If(
            LogicOp('||',
                    BinOp('<', Id('_INPUT_'), Num('48', False)),
                    BinOp('>', Id('_INPUT_'), Num('57', False))),
            Program([Break()]), None))
        # VAR = VAR * 10 + (_INPUT_ - 48)
        self.visit(Assign(vn,
            BinOp('+',
                  BinOp('*', Id(vn), Num('10', False)),
                  BinOp('-', Id('_INPUT_'), Num('48', False)))))
        # Read next char
        self.emit('b_INPUT_,')
        self.emit('t]')

if __name__ == '__main__':
    with open(sys.argv[1]) as f: code = f.read()
    tokens = lex(code)
    ast = Parser(tokens).parse()
    print(CodeGen().generate(ast))
