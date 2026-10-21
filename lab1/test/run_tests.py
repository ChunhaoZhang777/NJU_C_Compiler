#!/usr/bin/env python3
"""Behavioral checks for Lab1."""

import argparse
import json
import re
import subprocess
import tempfile
from pathlib import Path


TEST_ROOT = Path(__file__).resolve().parent
LAB_ROOT = TEST_ROOT.parent


def command(args, **kwargs):
    return subprocess.run([str(x) for x in args], capture_output=True, text=True, timeout=15, **kwargs)


def compile_source(source, directory):
    path = directory / 'input.cmm'
    path.write_text(source)
    args = [LAB_ROOT / 'Code/parser', path]
    result = command(args)
    assert result.returncode >= 0, f'compiler signal {-result.returncode}: {result.stderr}'
    assert not result.stderr.strip(), result.stderr
    return result


def check_fixtures(directory):
    """Run every named source fixture against its declared result."""
    fixture_root = TEST_ROOT
    cases = json.loads((fixture_root / 'cases.json').read_text(encoding='utf-8'))
    listed = [case['file'] for case in cases]
    found = {path.relative_to(fixture_root).as_posix() for path in fixture_root.rglob('*.cmm')}
    assert len(listed) == len(set(listed)), 'Lab1: duplicate fixture entries'
    assert set(listed) == found, 'Lab1: manifest does not match source files'
    count = 0
    for case in cases:
        name = f"Lab1/{case['file']}"
        source = (fixture_root / case['file']).read_text(encoding='utf-8')
        try:
            assert 'runs' not in case, 'runtime fixture in frontend lab'
            result = compile_source(source, directory)
            assert result.returncode == case['exit_code'], result.stdout + result.stderr
            actual = re.findall(r'Error type (\w+) at Line (\d+):', result.stdout)
            expected = [(str(kind), str(line)) for (kind, line) in case.get('errors', [])]
            assert actual == expected, f'expected diagnostics {expected}, got {actual}'
            for fragment in case.get('contains', []):
                assert fragment in result.stdout, f'missing output: {fragment!r}'
            assert ('Program (' in result.stdout) == (result.returncode == 0)
            count += 1
        except AssertionError as error:
            raise AssertionError(f'{name}: {error}') from error
    return count


def check_regression(directory):
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
        result = compile_source(source, directory)
        assert result.returncode == 1, result.stdout + result.stderr
        assert f'Error type {kind} at Line {line}:' in result.stdout, result.stdout
        assert 'Program (' not in result.stdout
        count += 1
    result = compile_source('int main(){return 0;}// no newline', directory)
    assert result.returncode == 0, result.stdout
    count += 1
    result = compile_source('\n\nint main(){float x=01.23E2;int a=0123,b=0x3F;return -2*3;}', directory)
    for token in ('Program (3)', 'INT: 83', 'INT: 63', 'FLOAT: 123.000000'):
        assert token in result.stdout, result.stdout
    count += 1
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixtures-only', action='store_true', help='run only the named cases in cases.json')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='lab1-tests-') as temp:
        directory = Path(temp)
        count = check_fixtures(directory)
        if not args.fixtures_only:
            count += check_regression(directory)
    print(f'Lab1: {count} checks passed')


if __name__ == '__main__':
    main()
