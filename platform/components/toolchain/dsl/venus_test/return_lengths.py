"""Conservative BAS-constant return-length analysis (no Venus execution).

Only scalar POD arguments, integer expressions, assignments and if statements
are evaluated. Unsupported values remain unknown; loops invalidate their writes.
Function-level metadata uses the maximum over ALL BAS calls and return paths.
The existing object-capacity fallback remains responsible for unknown lengths.
"""
import re
from pycparser import c_ast as C, c_parser
from pycparser.plyparser import ParseError


def integer(text):
    text = re.sub(r'[uUlL]+$', '', text.strip())
    try:
        return int(text, 16 if text.lower().startswith('0x') else 10)
    except ValueError:
        return None


def scalar_type(node, typedefs):
    if isinstance(node, C.TypeDecl):
        return scalar_type(node.type, typedefs)
    if isinstance(node, C.IdentifierType):
        name = ' '.join(node.names)
        if name in typedefs:
            return scalar_type(typedefs[name], {})
        sizes = {'char': 1, 'signed char': 1, 'unsigned char': 1,
                 'short': 2, 'short int': 2, 'unsigned short': 2,
                 'int': 4, 'unsigned': 4, 'unsigned int': 4,
                 'long': 4, 'unsigned long': 4}
        if name in sizes:
            # Plain char signedness is target-dependent.
            if name == 'char':
                return None
            if sizes[name] == 4 and ('unsigned' in name or name == 'unsigned'):
                return None  # 32-bit unsigned arithmetic needs C promotion rules
            return sizes[name], name != 'char' and 'unsigned' not in name
    return None


def struct_type(node, typedefs):
    if isinstance(node, C.TypeDecl):
        return struct_type(node.type, typedefs)
    if isinstance(node, C.IdentifierType) and len(node.names) == 1:
        return struct_type(typedefs.get(node.names[0]), {})
    return node if isinstance(node, C.Struct) and node.decls else None


def decode_argument(raw, node, typedefs):
    """Decode actual little-endian BAS bytes, not the first list element."""
    if raw is None:
        return None
    scalar = scalar_type(node, typedefs)
    if scalar:
        size, signed = scalar
        if len(raw) < size:
            return None
        return int.from_bytes(raw[:size], 'little', signed=signed)
    struct = struct_type(node, typedefs)
    if struct:
        fields, offset = {}, 0
        for field in struct.decls:
            spec = scalar_type(field.type, typedefs)
            if not spec or field.bitsize is not None:
                return None
            size, signed = spec
            offset = (offset + size - 1) // size * size
            if offset + size > len(raw):
                return None
            fields[field.name] = int.from_bytes(raw[offset:offset + size], 'little', signed=signed)
            offset += size
        return fields
    return None


def bas_parameters(text):
    text = re.sub(r"'[^\n]*", '', text)
    values = {}
    for m in re.finditer(r'\bparameter\s+(char|short|int)\s+(\w+)\s*=\s*\{([^}]*)\}', text, re.S):
        width = {'char': 1, 'short': 2, 'int': 4}[m[1]]
        tokens = m[3].replace('\n', ' ').split(',')
        # Large vector inputs are deliberately outside scalar analysis.
        if len(tokens) * width > 256:
            continue
        numbers = [integer(t) for t in tokens if t.strip()]
        if not numbers or None in numbers:
            continue
        values[m[2]] = b''.join((n % (1 << (8 * width))).to_bytes(width, 'little') for n in numbers)
    return values


def key(node):
    if isinstance(node, C.ID):
        return node.name
    if isinstance(node, C.StructRef) and node.type == '.' and isinstance(node.name, C.ID):
        return node.name.name + '.' + node.field.name
    return None


class Analysis:
    def __init__(self, typedefs, vectors, sizeof):
        self.typedefs, self.vectors, self.sizeof = typedefs, vectors, sizeof
        self.types = {}
        self.returns = []

    def expr(self, node, env):
        if node is None:
            return None
        if key(node):
            return env.get(key(node))
        if isinstance(node, C.Constant):
            value = integer(node.value) if node.type in ('int', 'long int') else None
            return value if value is not None and -(1<<31) <= value < (1<<31) else None
        if isinstance(node, C.Cast):
            return self.convert(self.expr(node.expr, env), node.to_type.type)
        if isinstance(node, C.UnaryOp):
            if node.op == 'sizeof':
                if isinstance(node.expr, C.ID):
                    return self.sizeof(node.expr.name)
                if isinstance(node.expr, C.Typename):
                    spec = scalar_type(node.expr.type, self.typedefs)
                    return spec[0] if spec else None
                return None
            x = self.expr(node.expr, env)
            if x is None:
                return None
            return {'+': lambda: x, '-': lambda: -x, '!': lambda: int(not x), '~': lambda: ~x}.get(node.op, lambda: None)()
        if isinstance(node, C.BinaryOp):
            a, b = self.expr(node.left, env), self.expr(node.right, env)
            if node.op == '&&' and (a == 0 or b == 0):
                return 0
            if node.op == '||' and (a is not None and a != 0 or b is not None and b != 0):
                return 1
            if a is None or b is None:
                return None
            trunc = lambda: (abs(a) // abs(b)) * (-1 if (a < 0) != (b < 0) else 1)
            ops = {'+': lambda: a+b, '-': lambda: a-b, '*': lambda: a*b,
                   '/': trunc, '%': lambda: a-trunc()*b, '==': lambda: int(a==b),
                   '!=': lambda: int(a!=b), '<': lambda: int(a<b), '>': lambda: int(a>b),
                   '<=': lambda: int(a<=b), '>=': lambda: int(a>=b),
                   '&&': lambda: int(bool(a) and bool(b)), '||': lambda: int(bool(a) or bool(b))}
            try:
                value = ops.get(node.op, lambda: None)()
                return value if value is not None and -(1<<31) <= value < (1<<31) else None
            except ZeroDivisionError:
                return None
        if isinstance(node, C.TernaryOp):
            cond = self.expr(node.cond, env)
            a, b = self.expr(node.iftrue, env), self.expr(node.iffalse, env)
            return (a if cond else b) if cond is not None else (a if a == b else None)
        return None

    def convert(self, value, typ):
        spec = scalar_type(typ, self.typedefs)
        if value is None or not spec:
            return None
        size, signed = spec
        lo, hi = (-(1 << (8*size-1)), 1 << (8*size-1)) if signed else (0, 1 << (8*size))
        # Refuse target-dependent overflow/conversions rather than guessing.
        return value if lo <= value < hi else None

    def invalidate_writes(self, node, env):
        if node is None:
            return
        target = None
        if isinstance(node, C.Assignment):
            target = key(node.lvalue)
        elif isinstance(node, C.Decl):
            target = node.name
        elif isinstance(node, C.UnaryOp) and node.op in ('++', '--', 'p++', 'p--', '&'):
            target = key(node.expr)
        if target:
            for name in list(env):
                if name == target or name.startswith(target + '.'):
                    env[name] = None
        for _, child in node.children():
            self.invalidate_writes(child, env)

    def statement(self, node, env):
        if node is None:
            return env
        if isinstance(node, C.Compound):
            for item in node.block_items or []:
                env = self.statement(item, env)
                if env is None:
                    break
            return env
        if isinstance(node, C.Decl):
            self.types[node.name] = node.type
            env[node.name] = self.convert(self.expr(node.init, env), node.type)
        elif isinstance(node, C.Assignment):
            name = key(node.lvalue)
            if name:
                value = self.expr(node.rvalue, env) if node.op == '=' else None
                env[name] = self.convert(value, self.types.get(name))
            else:
                # Pointer writes can alias any tracked scalar.
                for name in env:
                    env[name] = None
        elif isinstance(node, C.If):
            condition = self.expr(node.cond, env)
            if condition is not None:
                return self.statement(node.iftrue if condition else node.iffalse, env)
            left = self.statement(node.iftrue, dict(env))
            right = self.statement(node.iffalse, dict(env))
            if left is None:
                return right
            if right is None:
                return left
            return {k: left.get(k) if left.get(k) == right.get(k) else None for k in left.keys() | right.keys()}
        elif isinstance(node, C.FuncCall):
            name = node.name.name if isinstance(node.name, C.ID) else ''
            args = node.args.exprs if node.args else []
            if name == 'vreturn':
                self.returns.append([self.expr(arg, env) for arg in args[1::2]])
            elif name != 'printf':
                self.invalidate_writes(node, env)
        elif isinstance(node, C.Return):
            return None
        else:
            self.invalidate_writes(node, env)
        return env


def infer_lengths(content, type_content, function, bas_text, helpers):
    """Return proven per-port maxima or None. Never execute source code."""
    # Unsupported control flow or preprocessing cannot be analyzed safely.
    if re.search(r'\b(?:goto|switch|asm|__asm__)\b', content) or re.search(r'^\s*#\s*(?:if|elif|else)', content, re.M):
        return None
    match = re.search(r'\bint\s+' + re.escape(function) + r'\s*\(.*?\)\s*\{', content, re.S)
    if not match:
        return None
    body = helpers['_extract_function_body'](content, function)
    clean_types = helpers['strip_attributes'](type_content)
    decls = re.findall(r'\btypedef\s+(?:struct\s*(?:\w+\s*)?\{[^{}]*\}[^;]*|[^;{}]+);', clean_types)
    unique = {}
    for decl in decls:
        alias = re.search(r'(\w+)\s*;$', decl)
        if alias:
            if re.search(r'\b' + re.escape(alias[1]) + r'\b', content):
                unique.setdefault(alias[1], decl)
    for name, base in [('uint8_t','unsigned char'),('int8_t','signed char'),('uint16_t','unsigned short'),('int16_t','short'),('uint32_t','unsigned int'),('int32_t','int'),('intptr_t','int'),('uintptr_t','unsigned int')]:
        unique.setdefault(name, 'typedef '+base+' '+name+';')
    source = '\n'.join(unique.values()) + '\n' + content[match.start():match.end()] + body + '}'
    source = helpers['strip_attributes'](source)
    try:
        tree = c_parser.CParser().parse(source)
    except ParseError:
        return None
    typedefs = {n.name: n.type for n in tree.ext if isinstance(n, C.Typedef)}
    func = next(n for n in tree.ext if isinstance(n, C.FuncDef))
    # Reject features for which this small interpreter has no sound model.
    seen_names = set()
    def supported(node, parent=None):
        if isinstance(node, C.Decl) and node.name:
            if node.name in seen_names:
                return False  # shadowing needs a real scope stack
            seen_names.add(node.name)
        if isinstance(node, C.UnaryOp) and node.op in ('++', '--', 'p++', 'p--'):
            return False
        if isinstance(node, C.Assignment) and not isinstance(parent, (C.Compound, C.If)):
            return False  # assignment expressions need sequencing/side effects
        if isinstance(node, C.FuncCall):
            name = node.name.name if isinstance(node.name, C.ID) else ''
            args = node.args.exprs if node.args else []
            def address_taken(n):
                return (isinstance(n, C.UnaryOp) and n.op == '&') or any(address_taken(c) for _, c in n.children())
            write_conversion = r'%(?:\d+\$)?[-+ #0]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|ll|[hljztL])?n'
            readonly_printf = (name == 'printf' and args and isinstance(args[0], C.Constant)
                               and args[0].type == 'string' and not re.search(write_conversion, args[0].value))
            if name != 'vreturn' and not readonly_printf and any(address_taken(arg) for arg in args):
                return False
        if isinstance(node, (C.For, C.While, C.DoWhile)):
            def has_return(n):
                return (isinstance(n, C.FuncCall) and isinstance(n.name, C.ID) and n.name.name == 'vreturn') or any(has_return(c) for _, c in n.children())
            if has_return(node):
                return False
        return all(supported(child, node) for _, child in node.children())
    if not supported(func):
        return None
    params = [p for p in (func.decl.type.args.params if func.decl.type.args else []) if isinstance(p, C.Decl)]
    vector_types = helpers['parse_vector_typedef_sizes'](type_content)
    bas_text = re.sub(r"'[^\n]*", '', bas_text or '')
    raw_params = bas_parameters(bas_text)
    calls = [helpers['_split_args_respecting_parens'](m[1]) for m in re.finditer(r'\b' + re.escape(function) + r'\s*\(([^()]*)\)', bas_text)]
    # An unbound function must remain conservative too.
    calls = calls or [[]]
    results = []
    for actuals in calls:
        analyzer = Analysis(typedefs, vector_types, lambda token: helpers['resolve_sizeof'](content, token, type_content))
        env = {}
        for i, formal in enumerate(params):
            analyzer.types[formal.name] = formal.type
            value = decode_argument(raw_params.get(actuals[i].strip()) if i < len(actuals) else None, formal.type, typedefs)
            if isinstance(value, dict):
                struct = struct_type(formal.type, typedefs)
                for field in struct.decls:
                    analyzer.types[formal.name+'.'+field.name] = field.type
                env.update({formal.name+'.'+k: v for k,v in value.items()})
            elif not (isinstance(formal.type, C.TypeDecl) and isinstance(formal.type.type, C.IdentifierType) and formal.type.type.names[0] in vector_types):
                env[formal.name] = value
        analyzer.statement(func.body, env)
        results.extend(analyzer.returns)
    if not results or len({len(r) for r in results}) != 1:
        return None
    return [max(port) if all(v is not None and v >= 0 for v in port) else None for port in zip(*results)]
