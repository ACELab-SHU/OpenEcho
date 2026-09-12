# Return-length and vector-alias compatibility

`venus_test/scalar_o.py` reads the effective `TARGET_DAG` passed by Make,
before falling back to `config.mk`. A command-line target must not accidentally
load constants from the default DAG in `config.mk`.

Vector typedef aliases are recognized from `ext_vector_type(N)` and the base
element type. Compiler input and `input_type.json` use the same canonical
`__vNiM` spelling, including legacy aliases such as `v4096i8`. The original
workload source is not rewritten. Output capacity uses the declared element
count, even if an old alias name contains a different number.

## Static length analysis

Install the additional dependency with
`python3 -m pip install -r venus_test/requirements.txt` in the build environment.
The server validation uses Python 3.8 and pycparser 2.21.

`return_lengths.py` parses scalar control flow; it never executes task C or
Venus instructions. Compile-time BAS parameters are decoded as little-endian
bytes into scalar POD fields. For example, `parameter char n={0,2}` passed to
`short_struct` supplies 512, not zero. Runtime `dag_input` and `dfedata` are
unknown and are not specialized from test input files.

Every call to a task in the selected BAS is considered. Function-level output
metadata records a proven upper bound across these calls and possible return
sites. It does not change the runtime `vreturn` length or pretend that every
call has the same exact output length. Alternative return sites retain their
port positions rather than becoming additional output ports.

The intentionally small analysis supports signed integer expressions, simple
assignments, and `if` control flow. Unknown assignments invalidate old values.
Unsupported syntax, shadowing, side effects, runtime values and arithmetic
without a supported C integer model remain unknown. Unknown lengths keep the
existing object-capacity fallback; a type whose capacity cannot be resolved
still fails compilation. This is not a general C interpreter, a new golden
reference, or a substitute for runtime capacity/output checks.

Run focused regressions with `python3 -m unittest discover -s tests -v`.
Application validation must additionally compile fresh artifacts, run Gem5,
compare output bytes, and meet the selected platform's RTL requirements before
claiming hardware qualification. Changing the temporary-memory budget or
removing resource/correctness gates is outside this fix.
