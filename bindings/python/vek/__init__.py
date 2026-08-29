import ctypes, os, sys
from pathlib import Path
class _Value(ctypes.Structure):
    _fields_=[("type",ctypes.c_int),("number",ctypes.c_double),("boolean",ctypes.c_int),("string_value",ctypes.c_char_p)]
def _load():
    env=os.getenv("VEK_LIBRARY")
    names=[env] if env else []
    names += ["vek.dll" if sys.platform.startswith("win") else "libvek.dylib" if sys.platform=="darwin" else "libvek.so"]
    for n in names:
        if not n: continue
        try:return ctypes.CDLL(n)
        except OSError:pass
    raise OSError("VEK shared library not found. Set VEK_LIBRARY to its full path.")
class Runtime:
    def __init__(self):
        self.lib=_load(); self.lib.vek_create.restype=ctypes.c_void_p
        self.lib.vek_destroy.argtypes=[ctypes.c_void_p]; self.lib.vek_destroy.restype=None
        self.lib.vek_last_error.argtypes=[ctypes.c_void_p]; self.lib.vek_last_error.restype=ctypes.c_char_p
        self.lib.vek_load_file.argtypes=[ctypes.c_void_p,ctypes.c_char_p]; self.lib.vek_load_file.restype=ctypes.c_int
        self.lib.vek_set_security_tier.argtypes=[ctypes.c_void_p,ctypes.c_int]; self.lib.vek_set_security_tier.restype=ctypes.c_int
        self.lib.vek_set_authority_role.argtypes=[ctypes.c_void_p,ctypes.c_int]; self.lib.vek_set_authority_role.restype=ctypes.c_int
        self.lib.vek_authority_validate_request.argtypes=[ctypes.c_void_p,ctypes.c_char_p,ctypes.c_char_p,ctypes.c_uint64,ctypes.c_char_p,ctypes.c_char_p,ctypes.c_char_p,ctypes.c_double]; self.lib.vek_authority_validate_request.restype=ctypes.c_int
        self.lib.vek_authority_last_reason.argtypes=[ctypes.c_void_p]; self.lib.vek_authority_last_reason.restype=ctypes.c_char_p
        self.lib.vek_call.argtypes=[ctypes.c_void_p,ctypes.c_char_p,ctypes.POINTER(_Value),ctypes.c_size_t]; self.lib.vek_call.restype=_Value
        self._r=self.lib.vek_create()
        if not self._r: raise RuntimeError("VEK runtime creation failed")
    @property
    def last_error(self): return (self.lib.vek_last_error(self._r) or b"").decode()
    def set_security_tier(self,tier):
        if not self.lib.vek_set_security_tier(self._r,int(tier)): raise RuntimeError("VEK security tier failed")
    def set_authority_role(self,role):
        if not self.lib.vek_set_authority_role(self._r,int(role)): raise RuntimeError("VEK authority role failed")
    def validate_request(self,action,actor,sequence,nonce,payload="",capability="",now_seconds=0.0):
        ok=self.lib.vek_authority_validate_request(self._r,str(action).encode(),str(actor).encode(),int(sequence),str(nonce).encode(),str(payload).encode(),str(capability).encode(),float(now_seconds))
        reason=(self.lib.vek_authority_last_reason(self._r) or b"").decode()
        return bool(ok),reason
    def load_file(self,path):
        if not self.lib.vek_load_file(self._r,str(path).encode()): raise RuntimeError(self.last_error)
    def call(self,name,*args):
        vals=[]
        for x in args:
            if isinstance(x,bool): vals.append(_Value(2,0,int(x),None))
            elif isinstance(x,(int,float)): vals.append(_Value(1,float(x),0,None))
            else: vals.append(_Value(3,0,0,str(x).encode()))
        arr=(_Value*len(vals))(*vals) if vals else None
        r=self.lib.vek_call(self._r,name.encode(),arr,len(vals))
        if r.type==0:
            if self.last_error: raise RuntimeError(self.last_error)
            return None
        if r.type==1:return r.number
        if r.type==2:return bool(r.boolean)
        return (r.string_value or b"").decode()
    def close(self):
        if self._r:self.lib.vek_destroy(self._r);self._r=None
    def __enter__(self):return self
    def __exit__(self,*_):self.close()
