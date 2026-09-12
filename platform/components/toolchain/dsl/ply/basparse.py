# An implementation of Dartmouth BASIC (1964)
#

import lex
import yacc
import basiclex

tokens = basiclex.tokens

precedence = (
    ('left', 'PLUS', 'MINUS'),
    ('left', 'TIMES', 'DIVIDE'),
    ('left', 'POWER'),
    ('right', 'UMINUS')
)

# A BASIC program is a series of statements.  We represent the program as a
# dictionary of tuples indexed by line number.

input_ports = {}
output_ports = {}

def p_program(p):
    '''program : program statement
               | statement'''

    if len(p) == 2 and p[1]:
        p[0] = {}
        line, stat = p[1]
        p[0][line] = stat
    elif len(p) == 3:
        p[0] = p[1]
        if not p[0]:
            p[0] = {}
        if p[2]:
            line, stat = p[2]
            p[0][line] = stat

# This catch-all rule is used for any catastrophic errors.  In this case,
# we simply return nothing


def p_program_error(p):
    '''program : error'''
    p[0] = None
    p.parser.error = 1

# Format of all BASIC statements.


def p_statement(p):
    '''statement : INTEGER command NEWLINE'''
    if isinstance(p[2], str):
        print("%s %s %s" % (p[2], "AT LINE", p[1]))
        p[0] = None
        p.parser.error = 1
    else:
        lineno = int(p[1])
        p[0] = (lineno, p[2])

# Interactive statements.


def p_statement_interactive(p):
    '''statement : RUN NEWLINE
                 | LIST NEWLINE
                 | NEW NEWLINE'''
    p[0] = (0, (p[1], 0))

# Blank line number


def p_statement_blank(p):
    '''statement : INTEGER NEWLINE'''
    p[0] = (0, ('BLANK', int(p[1])))

# Error handling for malformed statements


def p_statement_bad(p):
    '''statement : INTEGER error NEWLINE'''
    print("MALFORMED STATEMENT AT LINE %s" % p[1])
    p[0] = None
    p.parser.error = 1

# Blank line

def p_statement_newline(p):
    '''statement : NEWLINE'''
    p[0] = None

# LET statement


def p_command_let(p):
    '''command : LET variable EQUALS expr'''
    p[0] = ('LET', p[2], p[4])


def p_command_let_bad(p):
    '''command : LET variable EQUALS error'''
    p[0] = "BAD EXPRESSION IN LET"

def p_command_read(p):
    '''command : READ varlist'''
    p[0] = ('READ', p[2])

def p_command_read_bad(p):
    '''command : READ error'''
    p[0] = "MALFORMED VARIABLE LIST IN READ"


# DATA statement
def p_command_data(p):
    '''command : DATA numlist'''
    p[0] = ('DATA', p[2])


def p_command_data_bad(p):
    '''command : DATA error'''
    p[0] = "MALFORMED NUMBER LIST IN DATA"

# PRINT statement


def p_command_print(p):
    '''command : PRINT plist optend'''
    p[0] = ('PRINT', p[2], p[3])


def p_command_print_bad(p):
    '''command : PRINT error'''
    p[0] = "MALFORMED PRINT STATEMENT"

# Optional ending on PRINT. Either a comma (,) or semicolon (;)


def p_optend(p):
    '''optend : COMMA 
              | SEMI
              |'''
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = None

# PRINT statement with no arguments


def p_command_print_empty(p):
    '''command : PRINT'''
    p[0] = ('PRINT', [], None)

# GOTO statement


def p_command_goto(p):
    '''command : GOTO INTEGER'''
    p[0] = ('GOTO', int(p[2]))


def p_command_goto_bad(p):
    '''command : GOTO error'''
    p[0] = "INVALID LINE NUMBER IN GOTO"

# IF-THEN statement


def p_command_if(p):
    '''command : IF relexpr THEN INTEGER'''
    p[0] = ('IF', p[2], int(p[4]))


def p_command_if_bad(p):
    '''command : IF error THEN INTEGER'''
    p[0] = "BAD RELATIONAL EXPRESSION"


def p_command_if_bad2(p):
    '''command : IF relexpr THEN error'''
    p[0] = "INVALID LINE NUMBER IN THEN"

# FOR statement


def p_command_for(p):
    '''command : FOR ID EQUALS expr TO expr optstep'''
    p[0] = ('FOR', p[2], p[4], p[6], p[7])


def p_command_for_bad_initial(p):
    '''command : FOR ID EQUALS error TO expr optstep'''
    p[0] = "BAD INITIAL VALUE IN FOR STATEMENT"


def p_command_for_bad_final(p):
    '''command : FOR ID EQUALS expr TO error optstep'''
    p[0] = "BAD FINAL VALUE IN FOR STATEMENT"


def p_command_for_bad_step(p):
    '''command : FOR ID EQUALS expr TO expr STEP error'''
    p[0] = "MALFORMED STEP IN FOR STATEMENT"

# Optional STEP qualifier on FOR statement


def p_optstep(p):
    '''optstep : STEP expr
               | empty'''
    if len(p) == 3:
        p[0] = p[2]
    else:
        p[0] = None

# NEXT statement


def p_command_next(p):
    '''command : NEXT ID'''

    p[0] = ('NEXT', p[2])


def p_command_next_bad(p):
    '''command : NEXT error'''
    p[0] = "MALFORMED NEXT"

# END statement


def p_command_end(p):
    '''command : END'''
    p[0] = ('END',)

# REM statement


def p_command_rem(p):
    '''command : REM'''
    p[0] = ('REM', p[1])

# STOP statement


def p_command_stop(p):
    '''command : STOP'''
    p[0] = ('STOP',)

# DEF statement


def p_command_def(p):
    '''command : DEF ID LPAREN ID RPAREN EQUALS expr'''
    p[0] = ('FUNC', p[2], p[4], p[7])



def p_command_def_bad_rhs(p):
    '''command : DEF ID LPAREN ID RPAREN EQUALS error'''
    p[0] = "BAD EXPRESSION IN DEF STATEMENT"


def p_command_def_bad_arg(p):
    '''command : DEF ID LPAREN error RPAREN EQUALS expr'''
    p[0] = "BAD ARGUMENT IN DEF STATEMENT"

# GOSUB statement
def p_command_gosub(p):
    '''command : GOSUB INTEGER'''
    p[0] = ('GOSUB', int(p[2]))


def p_command_gosub_bad(p):
    '''command : GOSUB error'''
    p[0] = "INVALID LINE NUMBER IN GOSUB"

# RETURN statement


def p_command_return(p):
    '''command : RETURN'''
    p[0] = ('RETURN',)


# DIM statement


def p_command_dim(p):
    '''command : DIM dimlist'''
    p[0] = ('DIM', p[2])


def p_command_dim_bad(p):
    '''command : DIM error'''
    p[0] = "MALFORMED VARIABLE LIST IN DIM"

# List of variables supplied to DIM statement


def p_dimlist(p):
    '''dimlist : dimlist COMMA dimitem
               | dimitem'''
    if len(p) == 4:
        p[0] = p[1]
        p[0].append(p[3])
    else:
        p[0] = [p[1]]

# DIM items


def p_dimitem_single(p):
    '''dimitem : ID LPAREN INTEGER RPAREN'''
    p[0] = (p[1], eval(p[3]), 0)


def p_dimitem_double(p):
    '''dimitem : ID LPAREN INTEGER COMMA INTEGER RPAREN'''
    p[0] = (p[1], eval(p[3]), eval(p[5]))


# Arithmetic expressions


def p_expr_binary(p):
    '''expr : expr PLUS expr
            | expr MINUS expr
            | expr TIMES expr
            | expr DIVIDE expr
            | expr POWER expr'''

    p[0] = ('BINOP', p[2], p[1], p[3])


def p_expr_number(p):
    '''expr : INTEGER
            | FLOAT_O'''
    p[0] = ('NUM', eval(p[1]))


def p_expr_variable(p):
    '''expr : variable'''
    p[0] = ('VAR', p[1])


def p_expr_group(p):
    '''expr : LPAREN expr RPAREN'''
    p[0] = ('GROUP', p[2])


def p_expr_unary(p):
    '''expr : MINUS expr %prec UMINUS'''
    p[0] = ('UNARY', '-', p[2])

# Relational expressions


def p_relexpr(p):
    '''relexpr : expr LT expr
               | expr LE expr
               | expr GT expr
               | expr GE expr
               | expr EQUALS expr
               | expr NE expr'''
    p[0] = ('RELOP', p[2], p[1], p[3])

def p_basetype(p):
    '''basetype : INT ID
                | SHORT ID
                | CHAR ID
                | FLOAT ID
                | DOUBLE ID'''
    p[0] = (p[1], p[2])

# def p_baselist(p):
#     '''baselist : baselist COMMA basetype
#                | basetype'''
#     if len(p) > 2:
#         p[0] = p[1]
#         p[0].append(p[3])
#     else:
#         p[0] = [p[1]]

# Variables

def p_variable(p):
    '''variable : ID
              | ID LPAREN expr RPAREN
              | ID LPAREN expr COMMA expr RPAREN
              | INTEGER
              | AMPERSAND ID'''
    if len(p) == 2:
        p[0] = (p[1], None, None)
    elif len(p) == 3:
        p[0] = ('POINTER', p[2])
    elif len(p) == 5:
        p[0] = (p[1], p[3], None)
    else:
        p[0] = (p[1], p[3], p[5])

# Builds a list of variable targets as a Python list

def p_varlist(p):
    '''varlist : varlist COMMA variable
               | variable'''
    if len(p) > 2:
        p[0] = p[1]
        p[0].append(p[3])
    else:
        p[0] = [p[1]]

def p_cexpr_binary(p):
    '''cexpr : cexpr PLUS cexpr
            | cexpr MINUS cexpr
            | cexpr TIMES cexpr
            | cexpr DIVIDE cexpr
            | cexpr POWER cexpr'''

    p[0] = ('BINOP', p[2], p[1], p[3])


def p_cexpr_number(p):
    '''cexpr : INTEGER
            | FLOAT_O'''
    p[0] = ('NUM', eval(p[1]))


def p_cexpr_variable(p):
    '''cexpr : variable'''
    p[0] = ('VAR', p[1])


def p_cexpr_group(p):
    '''cexpr : LPAREN cexpr RPAREN'''
    p[0] = ('GROUP', p[2])


def p_cexpr_unary(p):
    '''cexpr : MINUS cexpr %prec UMINUS'''
    p[0] = ('UNARY', '-', p[2])


def p_numlist(p):
    '''numlist : numlist COMMA number
               | number'''

    if len(p) > 2:
        p[0] = p[1]
        p[0].append(p[3])
    else:
        p[0] = [p[1]]

# A number. May be an integer or a float

def p_number(p):
    '''number  : INTEGER
               | FLOAT_O'''
    p[0] = eval(p[1])

# A signed number.


def p_number_signed(p):
    '''number  : MINUS INTEGER
               | MINUS FLOAT_O'''
    p[0] = eval("-" + p[2])

# List of targets for a print statement
# Returns a list of tuples (label,expr)


def p_plist(p):
    '''plist   : plist COMMA pitem
               | pitem'''
    if len(p) > 3:
        p[0] = p[1]
        p[0].append(p[3])
    else:
        p[0] = [p[1]]


def p_item_string(p):
    '''pitem : STRING'''
    p[0] = (p[1][1:-1], None)


def p_item_string_expr(p):
    '''pitem : STRING expr'''
    p[0] = (p[1][1:-1], p[2])


def p_item_expr(p):
    '''pitem : expr'''
    p[0] = ("", p[1])

# Array
    
def p_array(p):
    '''array : LBRACE numlist RBRACE'''
    p[0] = (p[2])    

# Global

def p_global(p):
    '''command : GLOBAL basetype EQUALS INTEGER
               | GLOBAL basetype '''
    if len(p) > 3:
        p[0] = ('GLOBAL', p[2], p[4])
    else:
        p[0] = ('GLOBAL', p[2] , "NULL")

# Parameter

def p_parameter(p):
    '''command : PARAMETER basetype EQUALS array'''
    p[0] = ('PARAMETER', p[2], p[4])

# Return_value

def p_return_value(p):
    '''command : RETURN_VALUE basetype LBRACKET INTEGER RBRACKET'''
    p[0] = ('RETURN_VALUE', p[2], p[4])

def p_dfedata(p):
    '''command : DFEDATA basetype LBRACKET INTEGER RBRACKET'''
    p[0] = ('DFEDATA', p[2], p[4])

def p_dag_input(p):
    '''command : DAG_INPUT basetype LBRACKET INTEGER RBRACKET'''
    p[0] = ('DAG_INPUT', p[2], p[4])

# Common
def p_venus_expression(p):
    '''venus_expression : LBRACKET varlist RBRACKET EQUALS taskpara
                        | LBRACKET RBRACKET EQUALS taskpara'''
    if len(p) > 5:
        p[0] = (p[2], p[5])
    else:
        p[0] = (None, p[4])

def p_venus_expression_spmd_begin(p):
    '''venus_expression : PRAGMA SPMD ID LPAREN INTEGER RPAREN BEGIN'''
    p[0] = (None, ('SPMD_BEGIN', int(p[5])))

def p_venus_expression_spmd_end(p):
    '''venus_expression : PRAGMA SPMD END'''
    p[0] = (None, ('SPMD_END',))

def p_venus_expression_temp_pin(p):
    '''venus_expression : PRAGMA ID ID LPAREN varlist RPAREN'''
    if p[2] != 'temp' or p[3] != 'pin':
        raise SyntaxError(
            f"Unsupported pragma: #pragma {p[2]} {p[3]}(...); "
            f"only '#pragma temp pin(varlist)' and '#pragma spmd ...' are allowed"
        )
    p[0] = (None, ('TEMP_PIN', p[5]))

def p_taskpara(p):
    '''taskpara : ID LPAREN varlist RPAREN
                | ID LPAREN RPAREN'''
    if len(p) == 5:
        p[0] = (p[1], p[3])
    else:
        p[0] = (p[1], None)

def p_dag(p):
    '''dag : DAG ID EQUALS LBRACE venus_expressions RBRACE LT optional_params GT
            | DAG ID EQUALS LBRACE venus_expressions RBRACE'''
    if len(p) == 10:
        p[0] = {
            'id': p[2],
            'expressions': p[5],
            'schedule_interval': p[8].get('schedule_interval'),
            'start_date': p[8].get('start_date'),
            'end_date': p[8].get('end_date')
        }
    else:
        p[0] = {
            'id': p[2],
            'expressions': p[5],
            'schedule_interval': None,
            'start_date': None,
            'end_date': None
        }

def p_optional_params(p):
    '''optional_params : optional_param optional_params
                       | empty'''
    if len(p) == 3:
        p[0] = p[1]
        for key in p[2]:
            if key in p[0]:
                raise ValueError(f"Duplicate optional parameter: {key}")
            p[0][key] = p[2][key]
    else:
        p[0] = {}

def p_optional_param(p):
    '''optional_param : schedule_interval
                      | start_date
                      | end_date'''
    p[0] = p[1]

def p_schedule_interval(p):
    '''schedule_interval : SCHEDULE_INTERVAL EQUALS STRING'''
    p[0] = {'schedule_interval': p[3]}

def p_start_date(p):
    '''start_date : START_DATE EQUALS STRING'''
    p[0] = {'start_date': p[3]}

def p_end_date(p):
    '''end_date : END_DATE EQUALS STRING'''
    p[0] = {'end_date': p[3]}

def p_venus_expressions(p):
    '''venus_expressions : venus_expressions venus_expression
                         | venus_expression '''
    if len(p) > 2:
        p[0] = p[1] + [p[2]]
    else:
        p[0] = [p[1]]

def p_command(p):
    '''command : dag'''
    dag_data = p[1]
    dag_id = dag_data['id']
    dag_expressions = dag_data['expressions']
    schedule_interval = dag_data['schedule_interval']
    start_date = dag_data['start_date']
    end_date = dag_data['end_date']
    
    dag_object = {
        'id': dag_id,
        'tasks': [],
        'schedule_interval': schedule_interval,
        'start_date': start_date,
        'end_date': end_date
    }
    
    for task in dag_expressions:
        output_vars, task_para = task
        if isinstance(task_para, tuple) and len(task_para) == 2 and task_para[0] == 'SPMD_BEGIN':
            dag_object['tasks'].append({'name': 'SPMD_BEGIN', 'min_core_num': task_para[1], 'input': None, 'output': None})
        elif isinstance(task_para, tuple) and len(task_para) == 1 and task_para[0] == 'SPMD_END':
            dag_object['tasks'].append({'name': 'SPMD_END', 'input': None, 'output': None})
        elif isinstance(task_para, tuple) and len(task_para) == 2 and task_para[0] == 'TEMP_PIN':
            pin_vars = []
            for item in task_para[1]:
                # 普通名：(name, None, None)；指针：('POINTER', name) 仅二元组，禁止直接 item[2]
                if isinstance(item, tuple) and len(item) >= 1 and item[0] == 'POINTER':
                    raise ValueError(
                        f"#pragma temp pin(...) only accepts plain variable names, got: {item}"
                    )
                if (not isinstance(item, tuple) or len(item) != 3
                        or not isinstance(item[0], str)
                        or item[1] is not None or item[2] is not None):
                    raise ValueError(
                        f"#pragma temp pin(...) only accepts plain variable names, got: {item}"
                    )
                pin_vars.append(item[0])
            if not pin_vars:
                raise ValueError("#pragma temp pin(...) requires at least one variable")
            dag_object['tasks'].append({'name': 'TEMP_PIN', 'vars': pin_vars, 'input': None, 'output': None})
        elif isinstance(task_para, tuple) and len(task_para) == 2:
            task_name, input_vars = task_para
            dag_object['tasks'].append({'name': task_name, 'input': input_vars, 'output': output_vars})
        # 在这里处理 task_name 和 input_vars
        # elif len(task_para) == 4:
        #     task_name, input_vars, slice_length, data_type = task_para
        #     dag_object['tasks'].append({'name': task_name, 'input': input_vars, 'output': output_vars,'slice_length':slice_length, 'data_type':data_type})
        
    
    p[0] = ["DAG", dag_object]

# def p_command(p):
#     '''command : dag'''
#     dag_name, dag_tasks = p[1]
#     dag_object = {'name': dag_name, 'tasks': []}
#     for task in dag_tasks:
#         output_vars, task_para = task
#         task_name, input_vars = task_para
#         dag_object['tasks'].append({'name': task_name, 'input': input_vars, 'output': output_vars})
#     p[0] = ["DAG", dag_object]
    
# Empty


def p_empty(p):
    '''empty : '''

# Catastrophic error handler

def p_error(p):
    if not p:
        print("SYNTAX ERROR AT EOF")

bparser = yacc.yacc()


def parse(data, debug=0):
    bparser.error = 0
    p = bparser.parse(data, debug=debug)
    if bparser.error:
        return None
    return p

##DSL