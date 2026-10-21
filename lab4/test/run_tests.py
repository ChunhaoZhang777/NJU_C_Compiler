#!/usr/bin/env python3
"""Behavioral checks for Lab4."""

import argparse
import json
import os
import re
import shlex
import shutil
import subprocess
import tempfile
from pathlib import Path


TEST_ROOT = Path(__file__).resolve().parent
LAB_ROOT = TEST_ROOT.parent
SPIM = shlex.split(os.environ.get('SPIM', 'spim'))


RUNTIME_CASES = [
    ('large_stack_frame', 'int main(){int a[10000];a[9999]=7;write(a[9999]);return 0;}', [], [7]),
    ('hash_collision', 'int get(int collision_70[2]){return collision_70[0];}int main(){int collision_1501[2];collision_1501[0]=31;write(get(collision_1501));return 0;}', [], [31]),
    ('runtime_label_collision', 'int _prompt(){return 7;}int _ret(){return 9;}int main(){write(_prompt()+_ret());return 0;}', [], [16]),
    ('copy_short_array', 'int main(){int a[3],b[1];a[0]=1;a[1]=2;a[2]=3;b[0]=7;a=b;write(a[0]);write(a[1]);write(a[2]);return 0;}', [], [7,2,3]),
    ('chained_aggregate', 'int main(){int a[2],b[2],c[2];c[0]=11;c[1]=13;a=b=c;write(a[0]);write(a[1]);return 0;}', [], [11,13]),
    ('struct_copy_layout', 'struct A{int x[2];int y;};struct B{int z[1];int w;};int main(){struct A a;struct B b;a.x[1]=99;b.z[0]=5;b.w=7;a=b;write(a.x[0]);write(a.x[1]);write(a.y);return 0;}', [], [5,99,7]),
    ('arithmetic', 'int main(){write(-2*3+10);write(10/-3);write(-(1+2)*4);return 0;}', [], [4,-3,-12]),
    ('assignment_value', 'int main(){int a,b,c;c=a=b=7;write(c);write(a);write(b);return 0;}', [], [7,7,7]),
    ('logic_precedence', 'int main(){write(1||0&&0);write(!0+2);return 0;}', [], [1,3]),
    ('short_circuit', 'int main(){int a=0,b; b=0&&(a=1);b=1||(a=2);write(a);return 0;}', [], [0]),
    ('nested_calls', 'int twice(int x){return x*2;} int main(){write(twice(twice(3)));return 0;}', [], [12]),
    ('recursion', 'int fact(int n){if(n<2)return 1;return n*fact(n-1);}int main(){write(fact(6));return 0;}', [], [720]),
    ('stack_arguments', 'int sum(int a,int b,int c,int d,int e,int f,int g){return a+2*b+3*c+4*d+5*e+6*f+7*g;}int main(){write(sum(1,2,3,4,5,6,7));return 0;}', [], [140]),
    ('read_write', 'int main(){int x=read();write(x*2);write(write(9));return 0;}', [-5], [-10,9,0]),
    ('array_offset', 'int main(){int a[4],i=0;while(i<4){a[i]=i*3;i=i+1;}write(a[3]);return 0;}', [], [9]),
    ('array_parameter', 'int set(int p[3]){p[1]=42;return p[1];}int main(){int a[3];write(set(a));write(a[1]);return 0;}', [], [42,42]),
    ('matrix', 'int main(){int m[2][3];m[1][2]=17;m[0][1]=8;write(m[1][2]+m[0][1]);return 0;}', [], [25]),
    ('struct_parameter', 'struct S{int x;int y;};int set(struct S p){p.y=9;return p.y;}int main(){struct S a;a.x=2;write(set(a));write(a.x+a.y);return 0;}', [], [9,11]),
    ('aggregate_copy', 'int main(){int a[2],b[2];a[0]=3;a[1]=5;b=a;write(b[0]);write(b[1]);return 0;}', [], [3,5]),
    ('copy_to_parameter', 'int set(int p[2]){int b[2];b[0]=11;b[1]=13;p=b;return 0;}int main(){int a[2];set(a);write(a[0]);write(a[1]);return 0;}', [], [11,13]),
    ('source_name_collision', 'int main(){int _t1=3,_L1=8,tNULL=4;write(_t1+_L1+tNULL);return 0;}', [], [15]),
]


SEMANTIC_CASES = {
    1: 'int main(){return missing;}',
    2: 'int main(){return missing();}',
    3: 'int main(){int a;int a;return 0;}',
    4: 'int f(){return 0;}int f(){return 0;}',
    5: 'int main(){int a;a=1.0;return 0;}',
    6: 'int main(){1=2;return 0;}',
    7: 'int main(){return 1+1.0;}',
    8: 'int main(){return 1.0;}',
    9: 'int f(int a){return a;}int main(){return f();}',
    10: 'int main(){int a;return a[0];}',
    11: 'int main(){int a;return a();}',
    12: 'int main(){int a[2];return a[1.0];}',
    13: 'int main(){int a;return a.x;}',
    14: 'struct S{int x;};int main(){struct S a;return a.y;}',
    15: 'struct S{int x;int x;};int main(){return 0;}',
    16: 'struct S{int x;};struct S{int y;};int main(){return 0;}',
    17: 'struct Missing a;int main(){return 0;}',
}


def command(args, **kwargs):
    return subprocess.run([str(x) for x in args], capture_output=True, text=True, timeout=15, **kwargs)


def compile_source(source, directory):
    path = directory / 'input.cmm'
    path.write_text(source)
    args = [LAB_ROOT / 'Code/parser', path]
    output = directory / 'output.s'
    output.unlink(missing_ok=True)
    args.append(output)
    result = command(args)
    assert result.returncode >= 0, f'compiler signal {-result.returncode}: {result.stderr}'
    assert not result.stderr.strip(), result.stderr
    return result, output


def check_runtime(name, source, inputs, expected, directory, have_spim):
    result, output = compile_source(source, directory)
    assert result.returncode == 0, result.stdout + result.stderr
    if have_spim:
        run = command(SPIM + ['-file', output], input=''.join((f'{x}\n' for x in inputs)))
        assert run.returncode == 0, run.stderr
        assert not run.stderr.strip(), run.stderr
        body = run.stdout.split('Loaded:', 1)[-1].split('\n', 1)[-1]
        body = body.replace('Enter an integer:', '')
        assert not re.search('Exception|syntax error|undefined|Bad address', body), body
        actual = [int(line) for line in body.splitlines() if re.fullmatch('-?\\d+', line.strip())]
    else:
        return
    assert actual == expected, f'{name}: expected {expected}, got {actual}'


def check_fixtures(directory, have_spim):
    """Run every named source fixture against its declared result."""
    fixture_root = TEST_ROOT
    cases = json.loads((fixture_root / 'cases.json').read_text(encoding='utf-8'))
    listed = [case['file'] for case in cases]
    found = {path.relative_to(fixture_root).as_posix() for path in fixture_root.rglob('*.cmm')}
    assert len(listed) == len(set(listed)), 'Lab4: duplicate fixture entries'
    assert set(listed) == found, 'Lab4: manifest does not match source files'
    count = 0
    for case in cases:
        name = f"Lab4/{case['file']}"
        source = (fixture_root / case['file']).read_text(encoding='utf-8')
        try:
            if 'runs' in case:
                assert case['runs'], 'runtime cases need input/output pairs'
                for run in case['runs']:
                    check_runtime(name, source, run['input'], run['output'], directory, have_spim)
                    count += 1
                continue
            result, output = compile_source(source, directory)
            assert result.returncode == case['exit_code'], result.stdout + result.stderr
            actual = re.findall(r'Error type (\w+) at Line (\d+):', result.stdout)
            expected = [(str(kind), str(line)) for (kind, line) in case.get('errors', [])]
            assert actual == expected, f'expected diagnostics {expected}, got {actual}'
            for fragment in case.get('contains', []):
                assert fragment in result.stdout, f'missing output: {fragment!r}'
            if result.returncode == 0:
                assert not result.stdout, result.stdout
            if result.returncode != 0:
                assert not output.exists(), 'failed compilation created an output file'
            count += 1
        except AssertionError as error:
            raise AssertionError(f'{name}: {error}') from error
    return count


def check_regression(directory, have_spim):
    count = 0
    compiler = LAB_ROOT / 'Code/parser'
    result = command([compiler])
    assert result.returncode == 1 and 'Usage:' in result.stderr
    count += 1
    for source, kind, line in [
        ('int main(){return ~1;}', 'A', 1),
        ('int main(){return 0;}/* missing', 'A', 1),
        ('\n' * 10010 + 'int main(){return ~1;}', 'A', 10011),
        ('int main(){return 0}', 'B', 1),
        ('int main(){return 09;}', 'A', 1),
        ('int main(){return 0x1G;}', 'A', 1),
        ('float a=1.05e;', 'A', 1),
    ]:
        result, _ = compile_source(source, directory)
        assert result.returncode == 1, result.stdout + result.stderr
        assert f'Error type {kind} at Line {line}:' in result.stdout, result.stdout
        count += 1
    result, _ = compile_source('int main(){return 0;}// no newline', directory)
    assert result.returncode == 0, result.stdout
    count += 1
    for (kind, source) in SEMANTIC_CASES.items():
        result, _ = compile_source(source, directory)
        assert result.returncode == 1, result.stdout
        assert f'Error type {kind} at Line 1:' in result.stdout, result.stdout
        count += 1
    for source in (
        'int main(){return !1;}',
        'int main(){return missing[0];}',
        'int main(){return missing.x;}',
        'int f(struct Missing p){return 0;}',
    ):
        result, _ = compile_source(source, directory)
        assert result.returncode in (0, 1), result.stderr
        count += 1
    for keyword in ('if', 'while'):
        result, _ = compile_source(f'int main(){{{keyword}(1.0)return 1;return 0;}}', directory)
        assert 'Error type 7 at Line 1:' in result.stdout, result.stdout
        count += 1
    for case in RUNTIME_CASES:
        check_runtime(*case, directory, have_spim)
        count += 1
    output = directory / 'output.s'
    output.write_text('preserve previous result\n')
    source = directory / 'invalid.cmm'
    source.write_text('int main(){return missing;}')
    result = command([compiler, source, output])
    assert result.returncode == 1 and output.read_text() == 'preserve previous result\n'
    count += 1
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixtures-only', action='store_true', help='run only the named cases in cases.json')
    parser.add_argument('--require-spim', action='store_true')
    args = parser.parse_args()
    have_spim = bool(SPIM) and shutil.which(SPIM[0]) is not None
    if args.require_spim and (not have_spim):
        parser.error('SPIM is required; install it or set SPIM to its command')
    with tempfile.TemporaryDirectory(prefix='lab4-tests-') as temp:
        directory = Path(temp)
        count = check_fixtures(directory, have_spim)
        if not args.fixtures_only:
            count += check_regression(directory, have_spim)
    print(f'Lab4: {count} checks passed')
    if not have_spim:
        print('SPIM unavailable: compilation checked; execution skipped. Use --require-spim to require execution.')


if __name__ == '__main__':
    main()
