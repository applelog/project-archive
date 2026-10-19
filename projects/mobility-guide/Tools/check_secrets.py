import hashlib
import os
from pathlib import Path
import re
import shlex
import stat

assert os.environ.get('CONDA_DEFAULT_ENV') == 'min', 'Run in conda min'
root = Path.cwd()
dotenv = root / '.env'
assert stat.S_IMODE(dotenv.stat().st_mode) == 0o600, 'Unexpected .env permissions'
secrets = []
for line in dotenv.read_text().splitlines():
    if not line.strip() or line.lstrip().startswith('#'):
        continue
    name, sep, raw = line.partition('=')
    assert sep and re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', name.strip()), 'Invalid .env entry'
    values = shlex.split(raw, comments=True)
    assert len(values) <= 1, 'Invalid .env value'
    if values and len(values[0]) >= 8:
        secrets.append(values[0].encode())
assert secrets, 'No credential values available for comparison'
files = [root / name for name in ('README.md', 'SECURITY.md', 'programming_instructor.md', 'AGENTS.md', '.env.example')]
for folder in ('App', 'Core', 'Tools', 'Tests', 'docs', 'Data', 'MobilityGuide.xcodeproj'):
    files.extend(p for p in (root / folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
bundle = root / 'DerivedData/Build/Products/Debug-iphonesimulator/MobilityGuide.app'
bundle_files = [p for p in bundle.rglob('*') if p.is_file()]
assert bundle_files, 'App bundle missing'
assert not any(p.name.startswith('.env') for p in bundle_files), 'Dotenv found in app bundle'
files.extend(bundle_files)
matches = []
for path in files:
    data = path.read_bytes()
    if any(value in data for value in secrets):
        matches.append(str(path.relative_to(root)))
assert not matches, 'Credential detected in: ' + ', '.join(matches)
for name in ('network.json', 'station_steps.json'):
    assert hashlib.sha256((root / 'Data' / name).read_bytes()).digest() == hashlib.sha256((bundle / name).read_bytes()).digest(), 'Bundle resource mismatch'
print(f'Security check passed: .env mode 0600; {len(files)} files scanned; 0 credential matches; no bundled dotenv; resource hashes match')
