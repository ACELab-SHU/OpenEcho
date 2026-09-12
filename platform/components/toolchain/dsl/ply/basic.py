# An implementation of Dartmouth BASIC (1964)
#
import sys
import basiclex
import basparse
import basinterp

def add_line_numbers(text, start_line=1, increment=1):
    lines = text.split('\n')
    new_lines = []
    current_line = start_line

    i = 0
    while i < len(lines):
        line = lines[i]
        # print(f"line:{line}")
        if line.startswith('dag') and not line.startswith('dag_input'):
            dag_lines = []
            while i < len(lines):
                if '}' in lines[i]:
                    dag_lines.append(lines[i].strip())
                    break
                elif lines[i].strip().startswith("'"):
                    i += 1
                    continue
                else:
                    dag_lines.append(lines[i].strip())
                    i += 1
            new_lines.append(f"{current_line} " + ' '.join(dag_lines))
            current_line += increment
        elif line.startswith('parameter'):
            parameter_lines = []
            while i < len(lines):
                if '}' in lines[i]:
                    parameter_lines.append(lines[i].strip())
                    break
                else:
                    parameter_lines.append(lines[i].strip())
                    i += 1
            new_lines.append(f"{current_line} " + ' '.join(parameter_lines))
            current_line += increment

        elif line.startswith('END'):
            new_lines.append(f"{current_line} {line} "+ '\n')
            current_line += increment

        elif line.strip():
            new_lines.append(f"{current_line} {line}")
            current_line += increment
        else:
            new_lines.append('')
        i += 1

    return '\n'.join(new_lines)

if len(sys.argv) == 2:
    with open(sys.argv[1]) as f:
        data_withno_line_numbers = f.read()
        data = add_line_numbers(data_withno_line_numbers)
        print(f"{data}")
    prog = basparse.parse(data)
    if not prog:
        raise SystemExit
    b = basinterp.BasicInterpreter(prog)
    try:
        b.run()
        raise SystemExit
    except RuntimeError:
        pass

else:
    b = basinterp.BasicInterpreter({})

