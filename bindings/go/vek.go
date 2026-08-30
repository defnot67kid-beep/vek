package vek

/*
#cgo CFLAGS: -I../../include
#cgo windows LDFLAGS: -lvek
#cgo linux LDFLAGS: -lvek
#cgo darwin LDFLAGS: -lvek
#include <stdlib.h>
#include <vek/vek_c.h>
*/
import "C"

import (
    "errors"
    "unsafe"
)

type Runtime struct { ptr *C.vek_runtime }

func New() (*Runtime,error) {
    r:=C.vek_create()
    if r==nil{return nil,errors.New("VEK runtime creation failed")}
    return &Runtime{ptr:r},nil
}
func (r *Runtime) Close(){if r!=nil&&r.ptr!=nil{C.vek_destroy(r.ptr);r.ptr=nil}}
func (r *Runtime) LoadFile(path string) error {c:=C.CString(path);defer C.free(unsafe.Pointer(c));if C.vek_load_file(r.ptr,c)==0{return errors.New(r.LastError())};return nil}
func (r *Runtime) LastError() string {if r==nil||r.ptr==nil{return "VEK: null runtime"};return C.GoString(C.vek_last_error(r.ptr))}
func (r *Runtime) LastDiagnostic() string {if r==nil||r.ptr==nil{return "VEK: null runtime"};return C.GoString(C.vek_last_diagnostic_text(r.ptr))}
func (r *Runtime) CallNumber(name string,args ...float64)(float64,error){cname:=C.CString(name);defer C.free(unsafe.Pointer(cname));vals:=make([]C.vek_value,len(args));for i,v:=range args{vals[i]._type=C.VEK_NUMBER;vals[i].number=C.double(v)};var p *C.vek_value;if len(vals)>0{p=&vals[0]};out:=C.vek_call(r.ptr,cname,p,C.size_t(len(vals)));if out._type!=C.VEK_NUMBER{return 0,errors.New(r.LastError())};return float64(out.number),nil}
func (r *Runtime) AttachDebugger(enabled bool){v:=0;if enabled{v=1};C.vek_debugger_attach(r.ptr,C.int(v))}
func (r *Runtime) AddBreakpoint(function string){c:=C.CString(function);defer C.free(unsafe.Pointer(c));C.vek_debugger_add_function_breakpoint(r.ptr,c)}
func (r *Runtime) Continue(){C.vek_debugger_continue(r.ptr)}
func (r *Runtime) DebugTraceJSON() string{return C.GoString(C.vek_debugger_trace_json(r.ptr))}
func (r *Runtime) InstallCrashHandler(path,product,build string) bool {p:=C.CString(path);q:=C.CString(product);b:=C.CString(build);defer C.free(unsafe.Pointer(p));defer C.free(unsafe.Pointer(q));defer C.free(unsafe.Pointer(b));return C.vek_install_crash_handler(r.ptr,p,q,b)!=0}
