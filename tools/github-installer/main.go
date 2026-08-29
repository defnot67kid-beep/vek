//go:build windows

package main

import (
    "fmt"
    "os"
    "os/exec"
    "path/filepath"
    "runtime"
    "strings"
    "sync"
    "syscall"
    "time"
    "unsafe"
)

const version = "2.3.0"
const targetDir = `C:\vek`
const defaultRepo = "https://github.com/defnot67kid-beep/vek.git"

var (
    user32 = syscall.NewLazyDLL("user32.dll")
    kernel32 = syscall.NewLazyDLL("kernel32.dll")
    gdi32 = syscall.NewLazyDLL("gdi32.dll")
    advapi32 = syscall.NewLazyDLL("advapi32.dll")
    procDefWindowProcW = user32.NewProc("DefWindowProcW")
    procRegisterClassExW = user32.NewProc("RegisterClassExW")
    procCreateWindowExW = user32.NewProc("CreateWindowExW")
    procShowWindow = user32.NewProc("ShowWindow")
    procUpdateWindow = user32.NewProc("UpdateWindow")
    procGetMessageW = user32.NewProc("GetMessageW")
    procTranslateMessage = user32.NewProc("TranslateMessage")
    procDispatchMessageW = user32.NewProc("DispatchMessageW")
    procPostQuitMessage = user32.NewProc("PostQuitMessage")
    procPostMessageW = user32.NewProc("PostMessageW")
    procSetWindowTextW = user32.NewProc("SetWindowTextW")
    procSetTimer = user32.NewProc("SetTimer")
    procKillTimer = user32.NewProc("KillTimer")
    procSendMessageW = user32.NewProc("SendMessageW")
    procCreateSolidBrush = gdi32.NewProc("CreateSolidBrush")
    procCreateFontW = gdi32.NewProc("CreateFontW")
    procSetTextColor = gdi32.NewProc("SetTextColor")
    procSetBkColor = gdi32.NewProc("SetBkColor")
    procGetModuleHandleW = kernel32.NewProc("GetModuleHandleW")
    procRegSetValueExW = advapi32.NewProc("RegSetValueExW")
)

type WNDCLASSEX struct {
    CbSize uint32; Style uint32; LpfnWndProc uintptr; CbClsExtra int32; CbWndExtra int32
    HInstance syscall.Handle; HIcon syscall.Handle; HCursor syscall.Handle; HbrBackground syscall.Handle
    LpszMenuName *uint16; LpszClassName *uint16; HIconSm syscall.Handle
}
type POINT struct{ X,Y int32 }
type MSG struct { Hwnd syscall.Handle; Message uint32; WParam uintptr; LParam uintptr; Time uint32; Pt POINT; LPrivate uint32 }

const (
    WS_OVERLAPPEDWINDOW = 0x00CF0000
    WS_VISIBLE = 0x10000000
    WS_CHILD = 0x40000000
    SS_LEFT = 0x00000000
    WM_DESTROY = 0x0002
    WM_TIMER = 0x0113
    WM_CTLCOLORSTATIC = 0x0138
    WM_SETFONT = 0x0030
    WM_APP_UI = 0x8001
    WM_APP_DONE = 0x8002
    SW_SHOW = 5
)

var hwndMain, hwndLogo, hwndStatus, hwndProgress, hwndDetail syscall.Handle
var brush, font syscall.Handle
var animIndex int
var logoRunes []rune
var installStarted bool

var uiMu sync.Mutex
var pendingStatus, pendingProgress, pendingDetail string

const logo = ` __     __  ______  _  __
 \ \   / / |  ____|| |/ /
  \ \ / /  | |__   | ' /
   \ V /   |  __|  |  <
    \_/    |______||_|\_\

             V E K`

func utf16(s string) *uint16 { p,_ := syscall.UTF16PtrFromString(s); return p }
func rgb(r,g,b byte) uintptr { return uintptr(uint32(r)|uint32(g)<<8|uint32(b)<<16) }
func setText(hwnd syscall.Handle, s string) { procSetWindowTextW.Call(uintptr(hwnd), uintptr(unsafe.Pointer(utf16(s)))) }

func publish(status, progress, detail string) {
    uiMu.Lock()
    if status != "" { pendingStatus = status }
    if progress != "" { pendingProgress = progress }
    if detail != "" { pendingDetail = detail }
    uiMu.Unlock()
    procPostMessageW.Call(uintptr(hwndMain), WM_APP_UI, 0, 0)
}

func applyPendingUI() {
    uiMu.Lock()
    s, p, d := pendingStatus, pendingProgress, pendingDetail
    uiMu.Unlock()
    if s != "" { setText(hwndStatus, s) }
    if p != "" { setText(hwndProgress, p) }
    if d != "" { setText(hwndDetail, d) }
}

func wndProc(hwnd syscall.Handle, msg uint32, wParam,lParam uintptr) uintptr {
    switch msg {
    case WM_DESTROY:
        procPostQuitMessage.Call(0); return 0
    case WM_TIMER:
        if wParam == 1 { animateLogo(hwnd); return 0 }
        if wParam == 3 { procKillTimer.Call(uintptr(hwnd),3); procPostQuitMessage.Call(0); return 0 }
    case WM_APP_UI:
        applyPendingUI(); return 0
    case WM_APP_DONE:
        applyPendingUI()
        procSetTimer.Call(uintptr(hwnd),3,5000,0)
        return 0
    case WM_CTLCOLORSTATIC:
        procSetTextColor.Call(wParam, rgb(62,255,139))
        procSetBkColor.Call(wParam, rgb(5,10,8))
        return uintptr(brush)
    }
    r,_,_ := procDefWindowProcW.Call(uintptr(hwnd), uintptr(msg), wParam,lParam); return r
}

func animateLogo(hwnd syscall.Handle) {
    if animIndex < len(logoRunes) {
        animIndex++
        setText(hwndLogo,string(logoRunes[:animIndex]))
        delay := uintptr(90)
        if animIndex > len(logoRunes)/3 { delay = 35 }
        if animIndex > len(logoRunes)*2/3 { delay = 12 }
        procKillTimer.Call(uintptr(hwnd),1)
        procSetTimer.Call(uintptr(hwnd),1,delay,0)
        return
    }
    procKillTimer.Call(uintptr(hwnd),1)
    if !installStarted {
        installStarted = true
        go installWorkflow()
    }
}

func stage(n int, status, detail string) {
    hashes := strings.Repeat("#",n)+strings.Repeat(".",10-n)
    publish(status, fmt.Sprintf("INSTALLING VEK - [%s] %d/10",hashes,n), detail)
    time.Sleep(180*time.Millisecond)
}

func installWorkflow() {
    stage(1,"PRE-FLIGHT // detecting existing VEK","Checking C:\\vek and Git...")
    existing := false
    existingPath := ""
    localExe := filepath.Join(targetDir,"vek.exe")
    localRepo := filepath.Join(targetDir,"repo",".git")
    localVersion := filepath.Join(targetDir,"VERSION")
    if _,err := os.Stat(localExe); err == nil {
        existing=true
        existingPath=localExe
    } else if _,err := os.Stat(localRepo); err == nil {
        existing=true
        existingPath=filepath.Join(targetDir,"repo")
    } else if _,err := os.Stat(localVersion); err == nil {
        existing=true
        existingPath=targetDir
    } else if vekPath,lookErr := exec.LookPath("vek.exe"); lookErr == nil {
        existing=true
        existingPath=vekPath
    } else if vekPath,lookErr := exec.LookPath("vek"); lookErr == nil {
        existing=true
        existingPath=vekPath
    }
    if existing {
        publish("DETECTED // existing VEK installation","",existingPath+" // update/repair mode")
    } else {
        publish("NEW INSTALL // no existing VEK detected","",targetDir+" // fresh install mode")
    }

    stage(2,"GIT // locating git.exe","VEK installs from the configured GitHub repository.")
    git,err := exec.LookPath("git.exe")
    if err != nil { git,err = exec.LookPath("git") }
    if err != nil { fail("Git is required but was not found on PATH. Install Git for Windows, then run the installer again."); return }

    repoURL := configuredRepo()
    stage(3,"GITHUB // repository selected",repoURL)

    if err := os.MkdirAll(targetDir,0755); err != nil { fail(err.Error()); return }
    repoDir := filepath.Join(targetDir,"repo")
    if existing {
        stage(4,"UPDATE // synchronizing VEK source","Existing VEK detected — fetching origin/main...")
    } else {
        stage(4,"CLONE // synchronizing VEK source","Cloning VEK from GitHub. The window remains responsive while Git works...")
    }
    if err := syncRepo(git,repoURL,repoDir); err != nil { fail("GitHub clone/update failed: "+err.Error()); return }

    stage(5,"RUNTIME // deploying vek.exe","Installing the VEK 2.3 command/runtime executable.")
    if err := copyPackageFiles(); err != nil { fail(err.Error()); return }

    stage(6,"METADATA // storing repository config",repoURL)
    if err := os.WriteFile(filepath.Join(targetDir,"REPOSITORY.txt"),[]byte(repoURL+"\r\n"),0644); err != nil { fail(err.Error()); return }
    if err := os.WriteFile(filepath.Join(targetDir,"VERSION"),[]byte(version+"\r\n"),0644); err != nil { fail(err.Error()); return }

    stage(7,"WINDOWS // registering User PATH",targetDir)
    if err := addUserPath(targetDir); err != nil { fail("PATH update failed: "+err.Error()); return }

    stage(8,"VERIFY // checking managed clone",filepath.Join(repoDir,".git"))
    if _,err := os.Stat(filepath.Join(repoDir,".git")); err != nil { fail("Git clone verification failed."); return }

    stage(9,"FINALIZE // installation ready","Open a new Command Prompt after installation.")
    stage(10,"COMPLETE // VEK is ready","VEK INSTALLED // You may close this window or it will auto-close in 5 seconds.")
    procPostMessageW.Call(uintptr(hwndMain), WM_APP_DONE, 0, 0)
}

func fail(msg string) {
    publish("INSTALL ERROR // VEK was not installed","INSTALLING VEK - [FAILED]",msg)
}

func configuredRepo() string {
    exe,_ := os.Executable(); base := filepath.Dir(exe)
    candidates := []string{filepath.Join(base,"REPOSITORY.txt"), filepath.Join(targetDir,"REPOSITORY.txt")}
    for _,p := range candidates {
        if b,err := os.ReadFile(p); err == nil {
            s := strings.TrimSpace(string(b))
            lower := strings.ToLower(s)
            if strings.HasPrefix(lower,"https://github.com/") && strings.HasSuffix(lower,".git") { return s }
        }
    }
    return defaultRepo
}

type progressWriter struct {
    mu sync.Mutex
    buf string
}
func (w *progressWriter) Write(p []byte) (int,error) {
    w.mu.Lock()
    w.buf += string(p)
    text := strings.ReplaceAll(w.buf,"\r","\n")
    lines := strings.Split(text,"\n")
    if len(lines) > 1 { w.buf = lines[len(lines)-1] }
    w.mu.Unlock()
    for i:=len(lines)-2;i>=0;i-- {
        line := strings.TrimSpace(lines[i])
        if line != "" {
            if len(line)>180 { line=line[:180] }
            publish("","", "GIT // "+line)
            break
        }
    }
    return len(p),nil
}

func runGitProgress(git string,args ...string) error {
    cmd := exec.Command(git,args...)
    cmd.Env = append(os.Environ(),"GIT_TERMINAL_PROMPT=0")
    pw := &progressWriter{}
    cmd.Stdout = pw
    cmd.Stderr = pw
    if err := cmd.Run(); err != nil {
        pw.mu.Lock(); last := strings.TrimSpace(pw.buf); pw.mu.Unlock()
        if last != "" { return fmt.Errorf("%v: %s",err,last) }
        return err
    }
    return nil
}

func syncRepo(git,repoURL,repoDir string) error {
    if _,err := os.Stat(filepath.Join(repoDir,".git")); err == nil {
        if err:=runGitProgress(git,"-C",repoDir,"remote","set-url","origin",repoURL); err!=nil{return err}
        if err:=runGitProgress(git,"-C",repoDir,"fetch","--progress","--depth","1","origin","main"); err!=nil{return err}
        if err:=runGitProgress(git,"-C",repoDir,"reset","--hard","origin/main"); err!=nil{return err}
        return nil
    }
    if _,err := os.Stat(repoDir); err == nil { if err:=os.RemoveAll(repoDir); err!=nil{return err} }
    return runGitProgress(git,"clone","--progress","--depth","1","--branch","main",repoURL,repoDir)
}

func copyPackageFiles() error {
    exe,err := os.Executable(); if err!=nil{return err}; src:=filepath.Dir(exe)
    for _,name := range []string{"vek.exe","VekInstaller.exe","README.md","manifest.sha256"} {
        from:=filepath.Join(src,name); to:=filepath.Join(targetDir,name)
        if samePath(from,to){continue}
        data,e:=os.ReadFile(from)
        if e!=nil { if name=="README.md"||name=="manifest.sha256"{continue}; return e }
        if e=os.WriteFile(to,data,0755); e!=nil{return e}
    }
    return nil
}
func samePath(a,b string) bool { aa,_:=filepath.Abs(a); bb,_:=filepath.Abs(b); return strings.EqualFold(filepath.Clean(aa),filepath.Clean(bb)) }

func addUserPath(dir string) error {
    var key syscall.Handle
    sub,_:=syscall.UTF16PtrFromString(`Environment`)
    if err:=syscall.RegOpenKeyEx(syscall.HKEY_CURRENT_USER,sub,0,syscall.KEY_QUERY_VALUE|syscall.KEY_SET_VALUE,&key);err!=nil{return err}
    defer syscall.RegCloseKey(key)
    name,_:=syscall.UTF16PtrFromString("Path"); var typ uint32; var n uint32
    _=syscall.RegQueryValueEx(key,name,nil,&typ,nil,&n)
    buf:=make([]byte,n+2); if n>0{_=syscall.RegQueryValueEx(key,name,nil,&typ,&buf[0],&n)}
    current:=""
    if len(buf)>=2 { u16:=(*[1<<20]uint16)(unsafe.Pointer(&buf[0]))[:len(buf)/2:len(buf)/2]; current=syscall.UTF16ToString(u16) }
    for _,p:=range strings.Split(current,";"){if strings.EqualFold(filepath.Clean(strings.TrimSpace(p)),filepath.Clean(dir)){return nil}}
    if current!=""&&!strings.HasSuffix(current,";"){current+=";"}; current+=dir
    data,_:=syscall.UTF16FromString(current); bytes:=unsafe.Slice((*byte)(unsafe.Pointer(&data[0])),len(data)*2)
    if typ!=syscall.REG_SZ&&typ!=syscall.REG_EXPAND_SZ{typ=syscall.REG_EXPAND_SZ}
    r,_,e:=procRegSetValueExW.Call(uintptr(key),uintptr(unsafe.Pointer(name)),0,uintptr(typ),uintptr(unsafe.Pointer(&bytes[0])),uintptr(len(bytes)))
    if r!=0{return e}; return nil
}

func applyFont(hwnd syscall.Handle) {
    if font != 0 { procSendMessageW.Call(uintptr(hwnd), WM_SETFONT, uintptr(font), 1) }
}

func main(){
    // Win32 windows and their message queues are thread-affine. Without this,
    // Go may move the goroutine to another OS thread and Windows can mark the
    // installer as "Not Responding" even while background work is running.
    runtime.LockOSThread()

    logoRunes=[]rune(logo)
    br,_,_:=procCreateSolidBrush.Call(rgb(5,10,8)); brush=syscall.Handle(br)
    f,_,_:=procCreateFontW.Call(^uintptr(17),0,0,0,400,0,0,0,1,0,0,0,0,uintptr(unsafe.Pointer(utf16("Consolas"))))
    font=syscall.Handle(f)
    hi,_,_:=procGetModuleHandleW.Call(0); hInst:=syscall.Handle(hi)
    className:=utf16("VEKGithubInstallerWindow"); cb:=syscall.NewCallback(wndProc)
    wc:=WNDCLASSEX{CbSize:uint32(unsafe.Sizeof(WNDCLASSEX{})),LpfnWndProc:cb,HInstance:hInst,HbrBackground:brush,LpszClassName:className}
    if r,_,e:=procRegisterClassExW.Call(uintptr(unsafe.Pointer(&wc)));r==0{panic(e)}
    raw,_,e:=procCreateWindowExW.Call(0,uintptr(unsafe.Pointer(className)),uintptr(unsafe.Pointer(utf16("VEK // GitHub Installer"))),WS_OVERLAPPEDWINDOW|WS_VISIBLE,160,120,790,575,0,0,uintptr(hInst),0)
    if raw==0{panic(e)}; hwndMain=syscall.Handle(raw)
    createLabel:=func(text string,x,y,w,h int)syscall.Handle{
        r,_,_:=procCreateWindowExW.Call(0,uintptr(unsafe.Pointer(utf16("STATIC"))),uintptr(unsafe.Pointer(utf16(text))),WS_CHILD|WS_VISIBLE|SS_LEFT,uintptr(x),uintptr(y),uintptr(w),uintptr(h),uintptr(hwndMain),0,uintptr(hInst),0)
        hnd:=syscall.Handle(r); applyFont(hnd); return hnd
    }
    hwndLogo=createLabel("",32,24,710,260)
    hwndStatus=createLabel("BOOT // initializing GitHub installer",32,300,710,28)
    hwndProgress=createLabel("INSTALLING VEK - [..........] 0/10",32,345,710,28)
    hwndDetail=createLabel("SOURCE // GitHub clone // transparent install",32,395,710,70)
    procShowWindow.Call(uintptr(hwndMain),SW_SHOW);procUpdateWindow.Call(uintptr(hwndMain));procSetTimer.Call(uintptr(hwndMain),1,90,0)
    var msg MSG
    for{
        r,_,_:=procGetMessageW.Call(uintptr(unsafe.Pointer(&msg)),0,0,0)
        if int32(r)<=0{break}
        procTranslateMessage.Call(uintptr(unsafe.Pointer(&msg)))
        procDispatchMessageW.Call(uintptr(unsafe.Pointer(&msg)))
    }
}
