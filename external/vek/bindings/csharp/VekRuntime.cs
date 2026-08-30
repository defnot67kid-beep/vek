using System;
using System.Runtime.InteropServices;
namespace Vek {
 public enum VekValueType:int { Nil=0, Number=1, Bool=2, String=3, Json=4 }
 [StructLayout(LayoutKind.Sequential)] public struct VekValue { public VekValueType type; public double number; public int boolean; public IntPtr string_value; }
 public sealed class VekRuntime:IDisposable {
  const string Lib="vek"; IntPtr handle;
  [DllImport(Lib,CallingConvention=CallingConvention.Cdecl)] static extern IntPtr vek_create();
  [DllImport(Lib,CallingConvention=CallingConvention.Cdecl)] static extern void vek_destroy(IntPtr r);
  [DllImport(Lib,CallingConvention=CallingConvention.Cdecl)] static extern int vek_load_file(IntPtr r,string path);
  [DllImport(Lib,CallingConvention=CallingConvention.Cdecl)] static extern VekValue vek_call(IntPtr r,string name,[In] VekValue[] args,UIntPtr count);
  [DllImport(Lib,CallingConvention=CallingConvention.Cdecl)] static extern IntPtr vek_last_error(IntPtr r);
  [DllImport(Lib,CallingConvention=CallingConvention.Cdecl)] static extern int vek_set_security_tier(IntPtr r,int tier);
  [DllImport(Lib,CallingConvention=CallingConvention.Cdecl)] static extern int vek_set_authority_role(IntPtr r,int role);
  [DllImport(Lib,CallingConvention=CallingConvention.Cdecl)] static extern int vek_authority_validate_request(IntPtr r,string action,string actor,UInt64 sequence,string nonce,string payload,string capability,double nowSeconds);
  [DllImport(Lib,CallingConvention=CallingConvention.Cdecl)] static extern IntPtr vek_authority_last_reason(IntPtr r);
  public VekRuntime(){handle=vek_create();if(handle==IntPtr.Zero)throw new InvalidOperationException("VEK runtime creation failed");}
  public void SetSecurityTier(int tier){if(vek_set_security_tier(handle,tier)==0)throw new InvalidOperationException("VEK security tier failed");}
  public void SetAuthorityRole(int role){if(vek_set_authority_role(handle,role)==0)throw new InvalidOperationException("VEK authority role failed");}
  public (bool Allowed,string Reason) ValidateRequest(string action,string actor,UInt64 sequence,string nonce,string payload="",string capability="",double nowSeconds=0){var ok=vek_authority_validate_request(handle,action,actor,sequence,nonce,payload,capability,nowSeconds)!=0;var reason=Marshal.PtrToStringUTF8(vek_authority_last_reason(handle))??"";return(ok,reason);}
  public void LoadFile(string path){if(vek_load_file(handle,path)==0)throw new InvalidOperationException(LastError);}
  public double CallNumber(string name,params double[] args){var v=new VekValue[args.Length];for(int i=0;i<args.Length;i++)v[i]=new VekValue{type=VekValueType.Number,number=args[i]};var r=vek_call(handle,name,v,(UIntPtr)v.Length);if(r.type!=VekValueType.Number)throw new InvalidOperationException(LastError);return r.number;}
  public string LastError=>Marshal.PtrToStringUTF8(vek_last_error(handle))??"";
  public void Dispose(){if(handle!=IntPtr.Zero){vek_destroy(handle);handle=IntPtr.Zero;}GC.SuppressFinalize(this);}~VekRuntime(){Dispose();}
 }
}
