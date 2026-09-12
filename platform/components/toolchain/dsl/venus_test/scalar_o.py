import os
import re
import json
import ast
from return_lengths import infer_lengths

def _load_local_include_text(file_path, visited=None):
    """
    递归加载本地 #include "xxx.h" 文本，用于 sizeof(type) 解析。
    """
    if visited is None:
        visited = set()
    abs_path = os.path.abspath(file_path)
    if abs_path in visited or not os.path.exists(abs_path):
        return ""
    visited.add(abs_path)

    try:
        with open(abs_path, 'r', encoding='utf-8', errors='ignore') as f:
            text = f.read()
    except Exception:
        return ""

    out = [text]
    base_dir = os.path.dirname(abs_path)
    for m in re.finditer(r'^\s*#\s*include\s+"([^"]+)"', text, flags=re.MULTILINE):
        inc = m.group(1).strip()
        inc_path = os.path.abspath(os.path.join(base_dir, inc))
        out.append(_load_local_include_text(inc_path, visited))
    return "\n".join(out)

def get_vector_type_size(type_name):
    """
    从 __vNNNiM 类型名推断字节数。
    例如 __v4096i8 -> 4096 * 8 / 8 = 4096 字节
         __v2048i16 -> 2048 * 16 / 8 = 4096 字节
    """
    m = re.match(r'__v(\d+)i(\d+)', type_name)
    if m:
        count = int(m.group(1))
        bits  = int(m.group(2))
        return count * bits // 8
    return None

def parse_vector_typedef_sizes(content):
    """
    解析形如：
      typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
    返回：{ "__v2048i16": 4096, ... }
    规则：size = N * base_type_size，N 取 ext_vector_type(N)。
    """
    typedef_sizes = {}
    pattern = re.compile(
        r'typedef\s+([A-Za-z_]\w*(?:\s+[A-Za-z_]\w*)*)\s+([A-Za-z_]\w*)\s+__attribute__\s*\(\(\s*ext_vector_type\s*\(\s*(\d+)\s*\)\s*\)\)\s*;',
        re.MULTILINE
    )
    for m in pattern.finditer(content):
        base_type = m.group(1).strip()
        alias = m.group(2).strip()
        n = int(m.group(3))
        elem_size = get_scalar_type_size(base_type)
        if elem_size is None:
            continue
        typedef_sizes[alias] = n * elem_size
    return typedef_sizes

def get_scalar_type_size(type_name):
    """
    从基础标量类型名推断字节数。
    """
    scalar_sizes = {
        'char': 1, 'unsigned char': 1, 'signed char': 1,
        'short': 2, 'unsigned short': 2, 'short int': 2,
        'int': 4, 'unsigned int': 4, 'unsigned': 4,
        'long': 4, 'unsigned long': 4,
        'float': 4,
        'double': 8, 'long double': 16,
        'long long': 8, 'unsigned long long': 8,
        'uint8_t': 1, 'int8_t': 1,
        'uint16_t': 2, 'int16_t': 2,
        'uint32_t': 4, 'int32_t': 4,
        'uint64_t': 8, 'int64_t': 8,
    }
    return scalar_sizes.get(type_name.strip())

def strip_attributes(content):
    """
    去掉所有 __attribute__((...)) 标注（支持嵌套括号）。
    """
    result = []
    i = 0
    while i < len(content):
        # 检测 __attribute__ 关键字
        if content[i:i+13] == '__attribute__':
            i += 13
            # 跳过空白
            while i < len(content) and content[i] in ' \t\n\r':
                i += 1
            # 消费平衡括号
            if i < len(content) and content[i] == '(':
                depth = 0
                while i < len(content):
                    if content[i] == '(':
                        depth += 1
                    elif content[i] == ')':
                        depth -= 1
                        if depth == 0:
                            i += 1
                            break
                    i += 1
        else:
            result.append(content[i])
            i += 1
    return ''.join(result)

def extract_var_type(content, var_name):
    """
    在 C 文件内容中查找 var_name 的向量类型声明。
    匹配形如：  __v4096i8 output;
    返回类型字符串，找不到返回 None。
    """
    # 支持同一行多变量声明：
    #   __v1024i8 a, b, c;
    aliases = set(parse_vector_typedef_sizes(content))
    type_pattern = '|'.join(re.escape(name) for name in sorted(aliases))
    type_pattern = '(?:' + type_pattern + '|__v\\d+i\\d+)' if type_pattern else r'__v\d+i\d+'
    pattern = re.compile(r'\b(' + type_pattern + r')\s+([^;]+);')
    for m in pattern.finditer(content):
        type_name = m.group(1).strip()
        declarators = m.group(2)
        for decl in declarators.split(','):
            decl = decl.split('=')[0].strip()
            decl = re.sub(r'\[.*?\]', '', decl).strip()
            name_m = re.search(r'([A-Za-z_]\w*)$', decl)
            if name_m and name_m.group(1) == var_name:
                return type_name
    return None

def extract_var_declared_type(content, var_name):
    """
    在 C 文件内容中查找 var_name 的声明类型（非向量类型）。
    匹配形如：  short_struct output2;  或  short output2;
    返回类型字符串，找不到返回 None。
    """
    # 声明限定符不参与 sizeof。先移除它们，避免把
    # "volatile short_struct out" 的类型误识别成 "volatile"。
    declaration_content = re.sub(
        r'\b(?:const|volatile|static|extern|register|restrict)\b\s*',
        '',
        content
    )

    # 排除向量类型（已由 extract_var_type 处理）
    # 支持同一行多变量声明：
    #   short a, b, c;
    pattern = re.compile(r'\b(?!__v\d+i\d+)(\w[\w\s]*?)\s+([^;]+);')
    for m in pattern.finditer(declaration_content):
        type_str = m.group(1).strip()
        if type_str in ('return', 'if', 'while', 'for', 'else', 'typedef',
                        'struct', 'union', 'enum', 'inline'):
            continue
        declarators = m.group(2)
        for decl in declarators.split(','):
            decl = decl.split('=')[0].strip()
            decl = re.sub(r'\[.*?\]', '', decl).strip()
            name_m = re.search(r'([A-Za-z_]\w*)$', decl)
            if name_m and name_m.group(1) == var_name:
                return type_str
    return None

def get_struct_size(content, struct_type_name, _visited=None):
    """
    从 C 文件内容中解析 typedef struct 的大小（字节数）。
    先从原始内容提取 aligned(N)，再剥离 __attribute__ 做成员大小计算，
    最后将裸大小向上对齐到 N 的整数倍。
    只处理简单的 POD struct（所有字段都是基础标量类型）。
    返回字节数，解析失败返回 None。
    """
    if _visited is None:
        _visited = set()
    if struct_type_name in _visited:
        return None
    _visited.add(struct_type_name)

    # 从原始内容中提取该 struct 定义上的 aligned(N)
    # 匹配 typedef struct { ... } <name> ... __attribute__((aligned(N))) ...;
    align_val = None
    raw_patterns = [
        re.compile(
            r'typedef\s+struct\s*\{[^}]*\}[^;]*?' + re.escape(struct_type_name) + r'[^;]*;',
            re.DOTALL
        ),
        re.compile(
            r'struct\s+' + re.escape(struct_type_name) + r'\s*\{[^}]*\}[^;]*;',
            re.DOTALL
        ),
    ]
    for pat in raw_patterns:
        m = pat.search(content)
        if m:
            decl = m.group(0)
            am = re.search(r'aligned\s*\(\s*(\d+)\s*\)', decl)
            if am:
                align_val = int(am.group(1))
            break

    # 剥离 __attribute__ 后做成员大小累加
    clean = strip_attributes(content)

    patterns = [
        re.compile(
            r'typedef\s+struct\s*\{([^}]*)\}\s*' + re.escape(struct_type_name) + r'\s*;',
            re.DOTALL
        ),
        re.compile(
            r'struct\s+' + re.escape(struct_type_name) + r'\s*\{([^}]*)\}',
            re.DOTALL
        ),
    ]
    for pat in patterns:
        m = pat.search(clean)
        if m:
            body = m.group(1)
            total = 0
            for line in body.splitlines():
                line = line.strip().rstrip(';').strip()
                if not line:
                    continue
                # 去掉数组维度，累加元素个数
                array_count = 1
                arr_m = re.search(r'\[\s*(\d+)\s*\]', line)
                if arr_m:
                    array_count = int(arr_m.group(1))
                line = re.sub(r'\[.*?\]', '', line)
                parts = line.split()
                if len(parts) >= 2:
                    type_str = ' '.join(parts[:-1])
                    size = get_scalar_type_size(type_str)
                    if size is None:
                        size = get_scalar_type_size(parts[0])
                    if size is None:
                        size = get_struct_size(clean, type_str, _visited)
                    if size is None and parts:
                        size = get_struct_size(clean, parts[0], _visited)
                    if size is not None:
                        total += size * array_count
            if total > 0:
                if align_val and align_val > 1:
                    # 向上对齐到 align_val 的整数倍
                    total = ((total + align_val - 1) // align_val) * align_val
                return total
    return None

def resolve_sizeof(content, sizeof_var, type_content=None):
    """
    解析 sizeof(sizeof_var) 的实际字节数。
    依次尝试：向量类型 → struct 类型 → 标量类型。
    局部变量声明在 content（.c）而 typedef 在头文件时，传入 type_content（含 data_type.h）
    以便在步骤 3 用 get_struct_size(type_content, declared_type) 解析 struct 大小。
    返回 int 或 None。
    """
    # 0. 向量 typedef（ext_vector_type(N)）
    vector_typedef_sizes = parse_vector_typedef_sizes(type_content or content)
    vector_typedef_sizes.update(parse_vector_typedef_sizes(content))
    if sizeof_var in vector_typedef_sizes:
        return vector_typedef_sizes[sizeof_var]

    # 1. sizeof(类型名) 直解：标量/结构体
    size = get_scalar_type_size(sizeof_var)
    if size is not None:
        return size
    size = get_struct_size(content, sizeof_var)
    if size is not None:
        return size

    # 2. 向量变量类型
    type_name = extract_var_type((type_content or '') + '\n' + content, sizeof_var)
    if type_name:
        if type_name in vector_typedef_sizes:
            size = vector_typedef_sizes[type_name]
        else:
            size = get_vector_type_size(type_name)
        if size is not None:
            return size

    # 3. 查找变量的声明类型
    declared_type = extract_var_declared_type(content, sizeof_var)
    if declared_type:
        # 先尝试标量
        size = get_scalar_type_size(declared_type)
        if size is not None:
            return size
        # 再尝试 struct（先 content，再 type_content 头文件）
        size = get_struct_size(content, declared_type)
        if size is not None:
            return size
        if type_content is not None:
            size = get_struct_size(type_content, declared_type)
            if size is not None:
                return size

    return None

def _split_args_respecting_parens(args_str):
    args = []
    depth = 0
    current = []
    for ch in args_str:
        if ch == '(':
            depth += 1
            current.append(ch)
        elif ch == ')':
            depth -= 1
            current.append(ch)
        elif ch == ',' and depth == 0:
            args.append(''.join(current).strip())
            current = []
        else:
            current.append(ch)
    if current:
        args.append(''.join(current).strip())
    return args

def _get_target_dag_from_config(repo_dsl_dir):
    target = os.environ.get('TARGET_DAG')
    if target:
        if not re.fullmatch(r'[A-Za-z_]\w*', target):
            raise ValueError('Invalid TARGET_DAG: ' + target)
        return target
    cfg = os.path.join(repo_dsl_dir, 'config.mk')
    if not os.path.exists(cfg):
        return None
    with open(cfg, 'r') as f:
        for line in f:
            m = re.match(r'\s*TARGET_DAG\s*=\s*([A-Za-z_]\w*)\s*$', line)
            if m:
                return m.group(1)
    return None

def _load_bas_parameter_constants(repo_dsl_dir):
    """
    从 ../<TARGET_DAG>.bas 读取 parameter 常量。
    例如：parameter short demod_length = {480}
          parameter char input_sequence_length = {240, 0}  → 240（取首元素）
    """
    params = {}
    dag = _get_target_dag_from_config(repo_dsl_dir)
    if not dag:
        return params, None
    bas_path = os.path.join(repo_dsl_dir, f'{dag}.bas')
    if not os.path.exists(bas_path):
        return params, None
    with open(bas_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            m = re.match(r"\s*parameter\s+\w+\s+([A-Za-z_]\w*)\s*=\s*\{\s*([^}]+)\s*\}\s*$", line)
            if not m:
                continue
            name, val_expr = m.group(1), m.group(2).strip()
            v = _parse_bas_parameter_scalar_value(val_expr)
            if v is not None:
                params[name] = v
    return params, bas_path

def _parse_function_arg_names(content, main_func_name):
    """
    解析函数定义参数名列表（只取名字，不做复杂 C 语法处理）。
    """
    m = re.search(r'\bint\s+' + re.escape(main_func_name) + r'\s*\((.*?)\)\s*\{', content, re.DOTALL)
    if not m:
        return []
    sig = m.group(1).strip()
    if not sig:
        return []
    names = []
    for arg in _split_args_respecting_parens(sig):
        s = arg.strip()
        if not s or s == 'void':
            continue
        s = re.sub(r'\s+', ' ', s)
        mname = re.search(r'([A-Za-z_]\w*)\s*$', s)
        if mname:
            names.append(mname.group(1))
    return names

def _load_call_arg_values_from_bas(repo_dsl_dir, main_func_name, func_arg_names, bas_params):
    """
    在目标 bas 中找到 Task 调用，把调用实参映射到函数形参，并尽量转成 int。
    只做保守求值：字面量或 parameter 常量。
    """
    env = {}
    dag = _get_target_dag_from_config(repo_dsl_dir)
    if not dag:
        return env
    bas_path = os.path.join(repo_dsl_dir, f'{dag}.bas')
    if not os.path.exists(bas_path):
        return env

    call_pat = re.compile(re.escape(main_func_name) + r'\s*\((.*)\)')
    with open(bas_path, 'r', encoding='utf-8', errors='ignore') as f:
        bas_lines = f.readlines()
    for line in bas_lines:
        line = line.strip()
        if line.startswith("'"):  # BAS 注释
            continue
        cm = call_pat.search(line)
        if not cm:
            continue
        call_args = _split_args_respecting_parens(cm.group(1))
        for i, formal in enumerate(func_arg_names):
            if i >= len(call_args):
                break
            a = call_args[i].strip()
            v = _parse_int_literal(a)
            if v is None and re.fullmatch(r'[A-Za-z_]\w*', a):
                v = bas_params.get(a)
            if v is None and re.fullmatch(r'[A-Za-z_]\w*', a):
                v = _resolve_symbol_value_from_bas(repo_dsl_dir, a, bas_params, bas_lines)
            if v is not None:
                env[formal] = int(v)
        break
    return env

def _parse_bas_task_assignment(line):
    m = re.search(r'\[(.*?)\]\s*=\s*(Task_[A-Za-z_]\w*)\s*\((.*)\)', line)
    if not m:
        return None
    outs = [x.strip() for x in m.group(1).split(',') if x.strip()]
    task = m.group(2).strip()
    args = _split_args_respecting_parens(m.group(3))
    return outs, task, args

def _eval_task_output_runtime_value(repo_dsl_dir, task_name, output_port, call_args, bas_params):
    c_path = os.path.join(repo_dsl_dir, 'venus_test', f'{task_name}.c')
    if not os.path.exists(c_path):
        return None
    with open(c_path, 'r', encoding='utf-8', errors='ignore') as f:
        c_content = strip_comments(f.read())

    func_arg_names = _parse_function_arg_names(c_content, task_name)
    env = dict(bas_params)
    for i, formal in enumerate(func_arg_names):
        if i >= len(call_args):
            break
        a = call_args[i].strip()
        v = _parse_int_literal(a)
        if v is None and re.fullmatch(r'[A-Za-z_]\w*', a):
            v = bas_params.get(a)
        if v is not None:
            env[formal] = int(v)

    vm = re.search(r'\bvreturn\s*\(', c_content)
    if not vm:
        return None
    start = vm.end()
    depth = 1
    i = start
    while i < len(c_content) and depth > 0:
        if c_content[i] == '(':
            depth += 1
        elif c_content[i] == ')':
            depth -= 1
        i += 1
    args = _split_args_respecting_parens(c_content[start:i - 1])
    _apply_simple_assignments_before_index(c_content, task_name, vm.start(), env)

    pair_idx = 0
    idx = 0
    while idx + 1 < len(args):
        var_token = args[idx].strip()
        size_token = args[idx + 1].strip()
        if pair_idx == output_port:
            if var_token.startswith('&'):
                out_var = var_token.lstrip('&').strip()
                data_assign_pat = re.compile(re.escape(out_var) + r'\s*\.\s*data\s*=\s*([^;]+);')
                vals = data_assign_pat.findall(c_content[:vm.start()])
                if vals:
                    v = _eval_expr_with_env(vals[-1].strip(), env, c_content)
                    if v is not None:
                        return int(v)
            v = _eval_expr_with_env(size_token, env, c_content)
            return int(v) if v is not None else None
        pair_idx += 1
        idx += 2
    return None

def _resolve_symbol_value_from_bas(repo_dsl_dir, symbol, bas_params, bas_lines):
    for line in bas_lines:
        s = line.strip()
        if not s or s.startswith("'"):
            continue
        parsed = _parse_bas_task_assignment(s)
        if not parsed:
            continue
        outs, task_name, call_args = parsed
        if symbol not in outs:
            continue
        port = outs.index(symbol)
        return _eval_task_output_runtime_value(repo_dsl_dir, task_name, port, call_args, bas_params)
    return None

def _replace_sizeof_tokens(expr, content):
    """
    把 sizeof(var/type) 替换为可计算整数；失败则返回 None。
    """
    if not isinstance(expr, str):
        return None
    result = expr
    pat = re.compile(r'sizeof\s*\(\s*([A-Za-z_]\w*)\s*\)')
    while True:
        m = pat.search(result)
        if not m:
            return result
        token = m.group(1)
        size = resolve_sizeof(content, token)
        if size is None:
            return None
        result = result[:m.start()] + str(size) + result[m.end():]

def _eval_expr_with_env(expr, env, content):
    """
    求值 retlen/赋值表达式：
      - 支持 + - * /
      - 支持标识符和 .data 访问（x.data 等价 x）
      - 支持 sizeof(var/type)
    无法解析返回 None。
    """
    expr2 = _replace_sizeof_tokens(expr, content)
    if expr2 is None:
        return None

    try:
        node = ast.parse(expr2, mode='eval')
    except SyntaxError:
        return None

    def _eval(n):
        if isinstance(n, ast.Expression):
            return _eval(n.body)
        if isinstance(n, ast.Constant) and isinstance(n.value, (int, float)):
            return int(n.value)
        if isinstance(n, ast.Name):
            return env.get(n.id)
        if isinstance(n, ast.Attribute):
            # short_struct.data 等访问按底层变量处理
            if isinstance(n.value, ast.Name) and n.attr == 'data':
                return env.get(n.value.id)
            return None
        if isinstance(n, ast.UnaryOp) and isinstance(n.op, (ast.UAdd, ast.USub)):
            v = _eval(n.operand)
            if v is None:
                return None
            return +v if isinstance(n.op, ast.UAdd) else -v
        if isinstance(n, ast.BinOp) and isinstance(n.op, (ast.Add, ast.Sub, ast.Mult, ast.Div)):
            a = _eval(n.left)
            b = _eval(n.right)
            if a is None or b is None:
                return None
            if isinstance(n.op, ast.Add):
                return a + b
            if isinstance(n.op, ast.Sub):
                return a - b
            if isinstance(n.op, ast.Mult):
                return a * b
            if isinstance(n.op, ast.Div):
                if b == 0:
                    return None
                return int(a / b)
        return None

    return _eval(node)

def _extract_function_body(content, main_func_name):
    """
    抽取函数体文本。
    """
    m = re.search(r'\bint\s+' + re.escape(main_func_name) + r'\s*\(.*?\)\s*\{', content, re.DOTALL)
    if not m:
        return ""
    i = m.end()
    depth = 1
    start = i
    while i < len(content) and depth > 0:
        if content[i] == '{':
            depth += 1
        elif content[i] == '}':
            depth -= 1
        i += 1
    return content[start:i-1] if depth == 0 else ""

def _apply_simple_assignments_before_index(content, main_func_name, end_index, env):
    """
    在函数体内，从头到 end_index 之前，按顺序应用简单赋值，记录“最近赋值”。
    """
    body = _extract_function_body(content, main_func_name)
    if not body:
        return
    prefix = body[:max(0, end_index)]
    statements = prefix.split(';')
    for st in statements:
        s = st.strip()
        if not s or '=' not in s:
            continue
        if '==' in s or '!=' in s or '>=' in s or '<=' in s:
            continue
        left, right = s.split('=', 1)
        lhs = left.strip()
        rhs = right.strip()
        # 去声明前缀，例如 "short softBitlLength"
        lhs = re.sub(r'^(?:[A-Za-z_]\w*(?:\s+[A-Za-z_]\w*)*\s+)+', '', lhs).strip()
        if not lhs:
            continue
        val = _eval_expr_with_env(rhs, env, content)
        if val is None:
            continue
        if '.' in lhs:
            base, attr = lhs.split('.', 1)
            if attr.strip() == 'data' and re.fullmatch(r'[A-Za-z_]\w*', base.strip()):
                env[base.strip()] = int(val)
            continue
        if re.fullmatch(r'[A-Za-z_]\w*', lhs):
            env[lhs] = int(val)

def strip_comments(content):
    """去掉 C 风格注释。"""
    # Match comments and quoted strings together: a block-comment marker
    # inside a // comment (or a string) must not consume unrelated source.
    pattern = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/'
    return re.sub(pattern, lambda m: ('\n' * m[0].count('\n') or ' ')
                  if m[0].startswith(('//', '/*')) else m[0], content, flags=re.DOTALL)

def _parse_int_literal(expr: str):
    """
    保守解析整数常量：
    - 支持 123 / (123) / 0x1A / (0x1A)
    - 不支持表达式（如 1+2、sizeof 等）
    """
    if not isinstance(expr, str):
        return None
    s = expr.strip()
    # 去掉一层或多层外层括号
    while True:
        m = re.match(r'^\(\s*(.*?)\s*\)$', s)
        if not m:
            break
        s = m.group(1).strip()
    if re.fullmatch(r'0[xX][0-9a-fA-F]+', s):
        try:
            return int(s, 16)
        except ValueError:
            return None
    if re.fullmatch(r'\d+', s):
        try:
            return int(s)
        except ValueError:
            return None
    return None

def _parse_bas_parameter_scalar_value(val_expr: str):
    """
    从 BAS parameter 花括号内容解析标量整数值，供 short_struct.data 等静态追溯使用。

    规则（按序，不得调换）：
    1. 整串尝试 _parse_int_literal（兼容现有 {480}、{16} 单值写法）；
    2. 若失败且含逗号，按逗号分割，从左到右取第一个 _parse_int_literal 成功的 token
       （兼容 {240, 0}、{6, 0} 等多元素写法，首元素即 .data 语义）；
    3. 均失败返回 None。
    """
    if not isinstance(val_expr, str):
        return None
    s = val_expr.strip()
    v = _parse_int_literal(s)
    if v is not None:
        return v
    if ',' not in s:
        return None
    for part in s.split(','):
        v = _parse_int_literal(part.strip())
        if v is not None:
            return v
    return None

def _eval_simple_int_expr(expr: str, consts: dict):
    """
    仅支持非常有限的整数表达式求值（用于 vreturn 的长度参数）：
    - 字面量：123 / 0x1A（支持外层括号）
    - 标识符：NAME（要求在 consts 中）
    - 乘法：A * B（A/B 允许是字面量或 consts 中的标识符；不支持多重运算符链）
    其它情况一律返回 None，避免误解析。
    """
    if not isinstance(expr, str):
        return None
    s = expr.strip()
    # 去掉外层括号
    while True:
        m = re.match(r'^\(\s*(.*?)\s*\)$', s)
        if not m:
            break
        s = m.group(1).strip()

    lit = _parse_int_literal(s)
    if lit is not None:
        return lit

    if re.fullmatch(r'[A-Za-z_]\w*', s):
        return consts.get(s)

    # 只支持单个乘号的二元乘法
    if s.count('*') == 1 and all(op not in s for op in ('+', '-', '/', '%', '<<', '>>', '&', '|', '^')):
        left, right = [p.strip() for p in s.split('*', 1)]
        a = _parse_int_literal(left)
        if a is None and re.fullmatch(r'[A-Za-z_]\w*', left):
            a = consts.get(left)
        b = _parse_int_literal(right)
        if b is None and re.fullmatch(r'[A-Za-z_]\w*', right):
            b = consts.get(right)
        if a is not None and b is not None:
            return int(a) * int(b)

    return None

def build_const_table(content: str) -> dict:
    """
    从同一 C 文件中构建“可静态确定”的整数常量表。
    仅支持：
    - #define NAME 123 / (123) / 0x1A
    - (static)? (const)? int NAME = 123;  （仅单变量、仅字面量）
    """
    consts = {}
    if not isinstance(content, str):
        return consts

    # 1) #define NAME <int>
    for m in re.finditer(r'^\s*#\s*define\s+([A-Za-z_]\w*)\s+(.+?)\s*$', content, flags=re.MULTILINE):
        name = m.group(1)
        rhs = m.group(2)
        val = _parse_int_literal(rhs)
        if val is not None:
            consts[name] = val

    # 2) (static)? (const)? <int-type> NAME = <int>;
    # 支持常见整型（避免扩大到复杂类型导致误匹配）
    decl_pat = re.compile(
        r'^\s*(?:static\s+)?(?:const\s+)?(?:unsigned\s+)?(?:char|short|int|long|long\s+long)\s+([A-Za-z_]\w*)\s*=\s*([^;]+?)\s*;\s*$',
        flags=re.MULTILINE
    )
    for m in decl_pat.finditer(content):
        name = m.group(1)
        rhs = m.group(2)
        val = _parse_int_literal(rhs)
        if val is not None:
            consts[name] = val

    return consts

def extract_info(file_path):
    """
    解析 C 文件中的 vreturn(...) 调用，提取每个输出端口的大小。

    vreturn 的参数格式为若干对 (var, size_expr)：
      vreturn(output, 30)                        -> 1个输出: output=30
      vreturn(output, 30, &output2, sizeof(output2)) -> 2个输出: output=30, output2=sizeof(output2)

    规则：
      - 参数列表按逗号分割
      - 奇数位（0-based偶数索引）是变量名，偶数位（0-based奇数索引）是大小表达式
      - 变量名以 & 开头表示指针传递，对应一个输出端口
      - 大小表达式可以是数字字面量或 sizeof(var)
    """
    with open(file_path, 'r') as f:
        c_raw = f.read()
    c_content = strip_comments(c_raw)
    # 类型解析需要头文件上下文；表达式/赋值追溯仍基于本 C 文件
    type_text = _load_local_include_text(file_path)
    type_content = strip_comments(type_text)
    repo_dsl_dir = os.path.abspath(os.path.join(os.getcwd(), '..'))

    file_name = os.path.basename(file_path)
    main_func_name = os.path.splitext(file_name)[0]

    target = _get_target_dag_from_config(repo_dsl_dir)
    bas_path = os.path.join(repo_dsl_dir, str(target) + '.bas')
    bas_text = ''
    if os.path.exists(bas_path):
        with open(bas_path, encoding='utf-8') as bas_file:
            bas_text = bas_file.read()
    proven_lengths = infer_lengths(c_content, type_content, main_func_name, bas_text, globals())

    # 匹配 vreturn(...) 的完整参数（支持括号嵌套，如 sizeof(x)）
    vreturn_pattern = re.compile(r'\bvreturn\s*\(')
    outputs = []
    output_sets = []

    for vm in vreturn_pattern.finditer(c_content):
        current_outputs = []
        start = vm.end()  # 第一个参数的起始位置
        # 手动扫描平衡括号，提取完整参数字符串
        depth = 1
        i = start
        while i < len(c_content) and depth > 0:
            if c_content[i] == '(':
                depth += 1
            elif c_content[i] == ')':
                depth -= 1
            i += 1
        args_str = c_content[start:i - 1]  # 去掉最后的 ')'

        # 按逗号分割参数，但不能在括号内分割（处理 sizeof(x)）
        args = _split_args_respecting_parens(args_str)

        # 按 (var, size) 对解析，每对对应一个输出端口
        idx = 0
        while idx + 1 < len(args):
            var_token = args[idx].strip()
            size_token = args[idx + 1].strip()

            # 变量名：去掉 & 前缀
            var_name = var_token.lstrip('&').strip()

            # 解析大小（以 retlen 为准）
            size = None
            sizeof_m = re.fullmatch(r'sizeof\s*\(\s*(\w+)\s*\)', size_token)
            if sizeof_m:
                sizeof_var = sizeof_m.group(1)
                size = resolve_sizeof(type_content, sizeof_var)
                if size is None:
                    size = resolve_sizeof(c_content, sizeof_var, type_content)
                if size is None and var_name:
                    fallback_size = resolve_sizeof(c_content, var_name, type_content)
                    if fallback_size is None:
                        fallback_size = resolve_sizeof(type_content, var_name, type_content)
                    if fallback_size is not None:
                        size = fallback_size
            elif size_token.isdigit():
                size = int(size_token)
            else:
                # 支持 size_token 为同文件可解析常量、最近赋值表达式或四则表达式
                # Never use a mutable initializer or the first BAS call as a
                # function-wide bound. Unknown paths fall back to capacity.
                size = (proven_lengths[idx // 2]
                        if proven_lengths is not None and idx // 2 < len(proven_lengths)
                        else None)
                # 若长度表达式仍无法解析，则回退到“按输出变量自身类型求 sizeof”。
                # 例如 vreturn(pdcchbits, sequenceLength) 场景中 sequenceLength
                # 可能在静态环境下不可解，但 pdcchbits 的类型是可静态确定的。
                if var_name and (size is None or int(size) <= 0):
                    fallback_size = resolve_sizeof(c_content, var_name, type_content)
                    if fallback_size is None:
                        fallback_size = resolve_sizeof(type_content, var_name, type_content)
                    if fallback_size is not None:
                        size = fallback_size

            if var_name:
                current_outputs.append(
                    {var_name: int(size) if size is not None else "unresolved"}
                )

            idx += 2

        # 多个控制流分支可能返回完全相同的端口集合。它们描述的是同一
        # task ABI，不能在 scalar_o.json 中重复展开成额外端口。
        if current_outputs and current_outputs not in output_sets:
            outputs.extend(current_outputs)
            output_sets.append(current_outputs)

    # Return sites describe alternatives of the same ABI, not extra ports.
    if output_sets:
        if len({len(site) for site in output_sets}) != 1:
            raise ValueError(main_func_name + ': inconsistent vreturn port counts')
        outputs = []
        for ports in zip(*output_sets):
            name = next(iter(ports[0]))
            sizes = [next(iter(port.values())) for port in ports]
            outputs.append({name: max(sizes) if all(isinstance(s, int) for s in sizes) else 'unresolved'})
    return main_func_name, outputs

def save_to_json(data, json_file_path):
    with open(json_file_path, 'w') as json_file:
        json.dump(data, json_file, indent=4)

def main():
    all_files_info = []
    for filename in sorted(os.listdir(os.getcwd())):
        if filename.endswith('.c'):
            main_func_name, outputs = extract_info(filename)
            all_files_info.append({'function': main_func_name, 'vreturn_o': outputs})
    save_to_json(all_files_info, 'scalar_o.json')


if __name__ == '__main__':
    main()
