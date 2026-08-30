#!/usr/bin/env python3
from pathlib import Path
import hashlib
try:
    from cryptography.hazmat.primitives.asymmetric import ec
    from cryptography.hazmat.primitives import serialization, hashes
    from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature
except ImportError:
    raise SystemExit("Missing Python package 'cryptography'. Install with: py -m pip install cryptography")

root=Path(__file__).resolve().parents[1]
key_dir=root/'developer_keys'
key_dir.mkdir(exist_ok=True)
(root/'src'/'generated').mkdir(parents=True,exist_ok=True)
priv=ec.generate_private_key(ec.SECP256R1())
private_path=key_dir/'DO_NOT_SHIP_vek_private.pem'
private_path.write_bytes(priv.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
pub=priv.public_key().public_numbers();x=pub.x.to_bytes(32,'big');y=pub.y.to_bytes(32,'big')
files=sorted(p for p in (root/'scripts').rglob('*.vek') if p.is_file())
if not files: raise SystemExit('No .vek scripts found')
fmt=lambda b:', '.join('0x%02x'%v for v in b)
entries=[]
for path in files:
    data=path.read_bytes(); digest=hashlib.sha256(data).digest()
    rel=path.relative_to(root).as_posix()
    entries.append((rel,digest))
    der=priv.sign(data,ec.ECDSA(hashes.SHA256()));r,s=decode_dss_signature(der)
    Path(str(path)+'.sig').write_text((r.to_bytes(32,'big')+s.to_bytes(32,'big')).hex(),encoding='ascii')
entry_text=',\n'.join(f'    ScriptHashEntry{{"{rel}", {{{fmt(d)}}}}}' for rel,d in entries)
header=f'''#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace VekGeneratedKey {{
inline constexpr std::array<std::uint8_t,32> PublicX = {{{fmt(x)}}};
inline constexpr std::array<std::uint8_t,32> PublicY = {{{fmt(y)}}};
struct ScriptHashEntry {{ std::string_view path; std::array<std::uint8_t,32> sha256; }};
inline constexpr std::array<ScriptHashEntry,{len(entries)}> ScriptHashes = {{{{
{entry_text}
}}}};
}}
'''
(root/'src'/'generated'/'VekPublicKey.h').write_text(header,encoding='utf-8')
print(f'Generated new VEK signing identity and signed {len(files)} scripts.')
print('PRIVATE KEY:',private_path)
print('IMPORTANT: keep this key outside public/game distributions. Delete developer_keys/ from release packages.')
