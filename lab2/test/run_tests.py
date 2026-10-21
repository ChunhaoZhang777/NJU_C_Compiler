#!/usr/bin/env python3
"""Behavioral checks for Lab2."""

import argparse
import json
import re
import subprocess
import tempfile
import zipfile
from pathlib import Path


TEST_ROOT = Path(__file__).resolve().parent
LAB_ROOT = TEST_ROOT.parent


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
    assert len(listed) == len(set(listed)), 'Lab2: duplicate fixture entries'
    assert set(listed) == found, 'Lab2: manifest does not match source files'
    count = 0
    for case in cases:
        name = f"Lab2/{case['file']}"
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
            if result.returncode == 0:
                assert not result.stdout, result.stdout
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
        count += 1
    result = compile_source('int main(){return 0;}// no newline', directory)
    assert result.returncode == 0, result.stdout
    count += 1
    for (kind, source) in SEMANTIC_CASES.items():
        result = compile_source(source, directory)
        assert result.returncode == 1, result.stdout
        assert f'Error type {kind} at Line 1:' in result.stdout, result.stdout
        count += 1
    for source in (
        'int main(){return !1;}',
        'int main(){return missing[0];}',
        'int main(){return missing.x;}',
        'int f(struct Missing p){return 0;}',
    ):
        result = compile_source(source, directory)
        assert result.returncode in (0, 1), result.stderr
        count += 1
    for keyword in ('if', 'while'):
        result = compile_source(f'int main(){{{keyword}(1.0)return 1;return 0;}}', directory)
        assert 'Error type 7 at Line 1:' in result.stdout, result.stdout
        count += 1
    manifest = json.loads((TEST_ROOT / 'official_semantic.json').read_text())
    archive = LAB_ROOT / 'Manual/Tests_2_春季班.zip'
    if archive.exists():
        with zipfile.ZipFile(archive) as z:
            for (name, expected) in manifest.items():
                source = z.read(f'Tests/inputs/{name}.cmm').decode('utf-8-sig')
                result = compile_source(source, directory)
                actual = set(re.findall('Error type (\\d+) at Line (\\d+):', result.stdout))
                expected = {(str(kind), str(line)) for (kind, line) in expected}
                assert expected <= actual, f'{name}: missing {expected - actual}'
                if not expected:
                    assert result.returncode == 0 and (not result.stdout), result.stdout
                count += 1
    else:
        print(f'Lab2: official semantic archive missing; skipped {len(manifest)} cases')
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixtures-only', action='store_true', help='run only the named cases in cases.json')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='lab2-tests-') as temp:
        directory = Path(temp)
        count = check_fixtures(directory)
        if not args.fixtures_only:
            count += check_regression(directory)
    print(f'Lab2: {count} checks passed')


if __name__ == '__main__':
    main()
