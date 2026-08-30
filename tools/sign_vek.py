#!/usr/bin/env python3
from pathlib import Path
import sys, hashlib
try:
    from cryptography.hazmat.primitives import serialization, hashes
    from cryptography.hazmat.primitives.asymmetric import ec
    from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature
except ImportError:
    raise SystemExit("Missing Python package 'cryptography'. Install with: py -m pip install cryptography")
root=Path(__file__).resolve().parents[1]
key_path=root/'developer_keys'/'DO_NOT_SHIP_vek_private.pem'
if not key_path.exists():
    raise SystemExit('Private VEK signing key missing. Run tools/generate_vek_signing_key.py to rotate the signing identity and rebuild the game.')
private=serialization.load_pem_private_key(key_path.read_bytes(),password=None)
files=[Path(x).resolve() for x in sys.argv[1:]] if len(sys.argv)>1 else sorted((root/'scripts').rglob('*.vek'))
for path in files:
    if not path.is_file() or path.suffix!='.vek': raise SystemExit(f'VEK script not found: {path}')
    try: path.relative_to(root/'scripts')
    except ValueError: raise SystemExit(f'Refusing to sign a script outside scripts/: {path}')
    data=path.read_bytes(); der=private.sign(data,ec.ECDSA(hashes.SHA256()));r,s=decode_dss_signature(der)
    Path(str(path)+'.sig').write_text((r.to_bytes(32,'big')+s.to_bytes(32,'big')).hex(),encoding='ascii')
    print(f'Signed {path.relative_to(root)}')
print('NOTE: secure releases also pin script SHA-256 values in src/generated/VekPublicKey.h. If script contents changed, regenerate the signing identity/manifest and rebuild.')
