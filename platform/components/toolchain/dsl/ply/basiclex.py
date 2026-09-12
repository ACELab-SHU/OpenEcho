# An implementation of Dartmouth BASIC (1964)

import lex
import yacc

# spmd修改：PRAGMA、SPMD、BEGIN到keywords和tokens
keywords = (
    'LET', 'READ', 'DATA', 'PRINT', 'GOTO', 'IF', 'THEN', 'FOR', 'NEXT', 'TO', 'STEP',
    'END', 'STOP', 'DEF', 'GOSUB', 'DIM', 'REM', 'RETURN', 'RUN', 'LIST', 'NEW',
    'SHORT', 'INT', 'CHAR', 'FLOAT', 'DOUBLE', 'DAG', 'SCHEDULE_INTERVAL', 'START_DATE', 'END_DATE',
    'GLOBAL', 'PARAMETER', 'RETURN_VALUE', 'DFEDATA', 'DAG_INPUT',
    'PRAGMA', 'SPMD', 'BEGIN'
)


tokens = keywords + (
    'EQUALS', 'PLUS', 'MINUS', 'TIMES', 'DIVIDE', 'POWER',
    'LPAREN', 'RPAREN', 'LT', 'LE', 'GT', 'GE', 'NE', 'LBRACKET', 'RBRACKET', 'LBRACE', 'RBRACE',
    'COMMA', 'SEMI', 'INTEGER', 'FLOAT_O', 'STRING','COMMENT',
    'ID', 'NEWLINE', 'AMPERSAND'
)

t_ignore = ' \t'

def t_COMMENT(t):
    r"'.*"
    pass

def t_REM(t):
    r'REM .*'
    return t


def t_ID(t):
    r'[a-zA-Z_][0-9a-zA-Z_]*'
    if t.value.upper() in tokens:
        t.type = t.value.upper()
    return t

t_EQUALS = r'='
t_PLUS = r'\+'
t_MINUS = r'-'
t_TIMES = r'\*'
t_POWER = r'\^'
t_DIVIDE = r'/'
t_LPAREN = r'\('
t_RPAREN = r'\)'
t_LT = r'<'
t_LE = r'<='
t_GT = r'>'
t_GE = r'>='
t_NE = r'<>'
t_COMMA = r'\,'
t_SEMI = r';'
t_AMPERSAND = r'&'
t_INTEGER = r'\d+'
t_FLOAT_O = r'((\d*\.\d+)(E[\+-]?\d+)?|([1-9]\d*E[\+-]?\d+))'
t_STRING = r'\".*?\"'
t_LBRACKET = r'\['
t_RBRACKET = r'\]'
t_LBRACE = r'\{'
t_RBRACE = r'\}'
t_DAG = r'dag'
t_SCHEDULE_INTERVAL = r'schedule_interval'
t_START_DATE = r'start_date'
t_END_DATE = r'end_date'
t_DFEDATA = r'dfedata'
t_DAG_INPUT = r'dag_input'
t_PRAGMA = r'\#pragma'
t_SPMD = r'spmd'
t_BEGIN = r'begin'
t_END = r'end'


def t_NEWLINE(t):
    r'\n+'
    t.lexer.lineno += len(t.value)
    t.value = '\n'
    return t


def t_error(t):
    print("Illegal character %s" % t.value[0])
    t.lexer.skip(1)

lex.lex(debug=0)
