# testlex.py

import unittest
try:
    import StringIO
except ImportError:
    import io as StringIO

import sys
import os
import warnings
import platform

sys.tracebacklimit = 0

import ply.lex

try:
    from importlib.util import cache_from_source
except ImportError:
    # Python 2.7, but we don't care.
    cache_from_source = None


def make_pymodule_path(filename, optimization=None):
    path = os.path.dirname(filename)
    file = os.path.basename(filename)
    mod, ext = os.path.splitext(file)

    if sys.hexversion >= 0x3050000:
        fullpath = cache_from_source(filename, optimization=optimization)
    elif sys.hexversion >= 0x3040000:
        fullpath = cache_from_source(filename, ext=='.pyc')
    elif sys.hexversion >= 0x3020000:
        import imp
        modname = mod+"."+imp.get_tag()+ext
        fullpath = os.path.join(path,'__pycache__',modname)
    else:
        fullpath = filename
    return fullpath

def pymodule_out_exists(filename, optimization=None):
    return os.path.exists(make_pymodule_path(filename,
                                             optimization=optimization))

def pymodule_out_remove(filename, optimization=None):
    os.remove(make_pymodule_path(filename, optimization=optimization))

def implementation():
    if platform.system().startswith("Java"):
        return "Jython"
    elif hasattr(sys, "pypy_version_info"):
        return "PyPy"
    else:
        return "CPython"

test_pyo = (implementation() == 'CPython')

def check_expected(result, expected, contains=False):
    if sys.version_info[0] >= 3:
        if isinstance(result,str):
            result = result.encode('ascii')
        if isinstance(expected,str):
            expected = expected.encode('ascii')
    resultlines = result.splitlines()
    expectedlines = expected.splitlines()

    if len(resultlines) != len(expectedlines):
        return False

    for rline,eline in zip(resultlines,expectedlines):
        if contains:
            if eline not in rline:
                return False
        else:
            if not rline.endswith(eline):
                return False
    return True

def run_import(module):
    code = "import "+module
    exec(code)
    del sys.modules[module]
    
# Tests related to errors and warnings when building lexers
class LexErrorWarningTests(unittest.TestCase):
    def setUp(self):
        sys.stderr = StringIO.StringIO()
        sys.stdout = StringIO.StringIO()
        if sys.hexversion >= 0x3020000:
            warnings.filterwarnings('ignore',category=ResourceWarning)

    def tearDown(self):
        sys.stderr = sys.__stderr__
        sys.stdout = sys.__stdout__
    
    def test_basiclex(self):
        self.assertRaises(SyntaxError,run_import,"basiclex")
        result = sys.stderr.getvalue()
        self.assertTrue(check_expected(result,
                              "basiclex.py:15: error'\n"))
 
import os
import subprocess
import shutil

# Tests related to various build options associated with lexers
class LexBuildOptionTests(unittest.TestCase):
    def setUp(self):
        sys.stderr = StringIO.StringIO()
        sys.stdout = StringIO.StringIO()
    def tearDown(self):
        sys.stderr = sys.__stderr__
        sys.stdout = sys.__stdout__
        try:
            shutil.rmtree("lexdir")
        except OSError:
            pass

    def test_lex_module(self):
        run_import("lex_module")
        result = sys.stdout.getvalue()
        self.assertTrue(check_expected(result,
                                    "(NUMBER,3,1,0)\n"
                                    "(PLUS,'+',1,1)\n"
                                    "(NUMBER,4,1,2)\n"))
        
    def test_lex_object(self):
        run_import("lex_object")
        result = sys.stdout.getvalue()
        self.assertTrue(check_expected(result,
                                    "(NUMBER,3,1,0)\n"
                                    "(PLUS,'+',1,1)\n"
                                    "(NUMBER,4,1,2)\n"))

    def test_lex_closure(self):
        run_import("lex_closure")
        result = sys.stdout.getvalue()
        self.assertTrue(check_expected(result,
                                    "(NUMBER,3,1,0)\n"
                                    "(PLUS,'+',1,1)\n"
                                    "(NUMBER,4,1,2)\n"))

    def test_lex_many_tokens(self):
        run_import("lex_many_tokens")
        result = sys.stdout.getvalue()
        self.assertTrue(check_expected(result,
                                    "(TOK34,'TOK34:',1,0)\n"
                                    "(TOK143,'TOK143:',1,7)\n"
                                    "(TOK269,'TOK269:',1,15)\n"
                                    "(TOK372,'TOK372:',1,23)\n"
                                    "(TOK452,'TOK452:',1,31)\n"
                                    "(TOK561,'TOK561:',1,39)\n"
                                    "(TOK999,'TOK999:',1,47)\n"
                                    ))
        
# Tests related to run-time behavior of lexers
class LexRunTests(unittest.TestCase):
    def setUp(self):
        sys.stderr = StringIO.StringIO()
        sys.stdout = StringIO.StringIO()
    def tearDown(self):
        sys.stderr = sys.__stderr__
        sys.stdout = sys.__stdout__

    def test_lex_hedit(self):
        run_import("lex_hedit")
        result = sys.stdout.getvalue()
        self.assertTrue(check_expected(result,
                                    "(H_EDIT_DESCRIPTOR,'abc',1,0)\n"
                                    "(H_EDIT_DESCRIPTOR,'abcdefghij',1,6)\n"
                                    "(H_EDIT_DESCRIPTOR,'xy',1,20)\n"))
       
    def test_lex_state_try(self):
        run_import("lex_state_try")
        result = sys.stdout.getvalue()
        self.assertTrue(check_expected(result,
                                    "(NUMBER,'3',1,0)\n"
                                    "(PLUS,'+',1,2)\n"
                                    "(NUMBER,'4',1,4)\n"
                                    "Entering comment state\n"
                                    "comment body LexToken(body_part,'This is a comment */',1,9)\n"
                                    "(PLUS,'+',1,30)\n"
                                    "(NUMBER,'10',1,32)\n"
                                    ))    



unittest.main()
