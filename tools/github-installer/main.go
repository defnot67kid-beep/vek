//go:build windows

package main

import (
    "fmt"
    "math"
    "os"
    "os/exec"
    "path/filepath"
    "runtime"
    "sort"
    "strconv"
    "strings"
    "sync"
    "syscall"
    "time"
    "unsafe"
)

const (
    version     = "2.5.0"
    targetDir   = `C:\vek`
    defaultRepo = "https://github.com/defnot67kid-beep/vek.git"

    windowW = 1120
    windowH = 780
)

var (
    user32   = syscall.NewLazyDLL("user32.dll")
    kernel32 = syscall.NewLazyDLL("kernel32.dll")
    gdi32    = syscall.NewLazyDLL("gdi32.dll")
    advapi32 = syscall.NewLazyDLL("advapi32.dll")

    procDefWindowProcW   = user32.NewProc("DefWindowProcW")
    procRegisterClassExW = user32.NewProc("RegisterClassExW")
    procCreateWindowExW  = user32.NewProc("CreateWindowExW")
    procShowWindow       = user32.NewProc("ShowWindow")
    procUpdateWindow     = user32.NewProc("UpdateWindow")
    procGetMessageW      = user32.NewProc("GetMessageW")
    procTranslateMessage = user32.NewProc("TranslateMessage")
    procDispatchMessageW = user32.NewProc("DispatchMessageW")
    procPostQuitMessage  = user32.NewProc("PostQuitMessage")
    procPostMessageW     = user32.NewProc("PostMessageW")
    procSetTimer         = user32.NewProc("SetTimer")
    procKillTimer        = user32.NewProc("KillTimer")
    procInvalidateRect   = user32.NewProc("InvalidateRect")
    procBeginPaint       = user32.NewProc("BeginPaint")
    procEndPaint         = user32.NewProc("EndPaint")
    procFillRect         = user32.NewProc("FillRect")
    procDrawTextW        = user32.NewProc("DrawTextW")
    procGetClientRect    = user32.NewProc("GetClientRect")

    procCreateSolidBrush = gdi32.NewProc("CreateSolidBrush")
    procCreatePen        = gdi32.NewProc("CreatePen")
    procSelectObject     = gdi32.NewProc("SelectObject")
    procDeleteObject     = gdi32.NewProc("DeleteObject")
    procMoveToEx         = gdi32.NewProc("MoveToEx")
    procLineTo           = gdi32.NewProc("LineTo")
    procPolygon          = gdi32.NewProc("Polygon")
    procRectangle        = gdi32.NewProc("Rectangle")
    procRoundRect        = gdi32.NewProc("RoundRect")
    procSetTextColor     = gdi32.NewProc("SetTextColor")
    procSetBkMode        = gdi32.NewProc("SetBkMode")
    procCreateFontW      = gdi32.NewProc("CreateFontW")

    procGetModuleHandleW = kernel32.NewProc("GetModuleHandleW")
    procRegSetValueExW   = advapi32.NewProc("RegSetValueExW")
)

type WNDCLASSEX struct {
    CbSize        uint32
    Style         uint32
    LpfnWndProc   uintptr
    CbClsExtra    int32
    CbWndExtra    int32
    HInstance     syscall.Handle
    HIcon         syscall.Handle
    HCursor       syscall.Handle
    HbrBackground syscall.Handle
    LpszMenuName  *uint16
    LpszClassName *uint16
    HIconSm       syscall.Handle
}

type POINT struct{ X, Y int32 }
type RECT struct{ Left, Top, Right, Bottom int32 }
type PAINTSTRUCT struct {
    Hdc         syscall.Handle
    FErase      int32
    RcPaint     RECT
    FRestore    int32
    FIncUpdate  int32
    RgbReserved [32]byte
}
type MSG struct {
    Hwnd     syscall.Handle
    Message  uint32
    WParam   uintptr
    LParam   uintptr
    Time     uint32
    Pt       POINT
    LPrivate uint32
}

type vec3 struct{ x, y, z float64 }
type vec2 struct{ x, y float64 }
type obstacle struct{ x, w, h float64 }

type semver struct{ major, minor, patch int; ok bool }

type installMode int
const (
    modeInstall installMode = iota
    modeRepair
    modeSourceOnly
)

type updatePolicy int
const (
    policyManual updatePolicy = iota
    policyAuto
)

const (
    WS_OVERLAPPEDWINDOW = 0x00CF0000
    WS_VISIBLE          = 0x10000000

    WM_DESTROY     = 0x0002
    WM_PAINT       = 0x000F
    WM_CLOSE       = 0x0010
    WM_KEYDOWN     = 0x0100
    WM_TIMER       = 0x0113
    WM_LBUTTONDOWN = 0x0201
    WM_APP_UI      = 0x8001
    WM_APP_DONE    = 0x8002
    WM_APP_VERSION = 0x8003
    WM_APP_AUTO    = 0x8004

    VK_SPACE = 0x20
    SW_SHOW  = 5

    PS_SOLID = 0
    TRANSPARENT = 1

    DT_LEFT       = 0x00000000
    DT_CENTER     = 0x00000001
    DT_RIGHT      = 0x00000002
    DT_VCENTER    = 0x00000004
    DT_SINGLELINE = 0x00000020
    DT_WORDBREAK  = 0x00000010
)

var (
    hwndMain syscall.Handle
    fontSmall syscall.Handle
    fontBody syscall.Handle
    fontButton syscall.Handle
    fontTitle syscall.Handle

    stateMu sync.Mutex
    statusLine   = "READY // choose a download mode"
    detailLine   = "SOURCE // https://github.com/defnot67kid-beep/vek.git"
    versionLine  = "VERSION STATUS // checking installed and latest VEK..."
    progressStep = 0
    installing   = false
    installDone  = false
    installFailed = false
    currentMode installMode
    policy = policyManual
    completeAt time.Time

    angle float64
    gridOffset float64
    playerY float64 = 636
    playerVy float64
    playerRot float64
    onGround = true
    score int
    crashes int
    obstacles = []obstacle{{420,34,38},{690,40,52},{970,34,42},{1260,52,64}}
    lastFrame = time.Now()

    latestVersion = "unknown"
    installedVersion = "none"
)

var downloadButtons = []RECT{
    {40, 275, 260, 343},
    {290, 275, 510, 343},
    {540, 275, 760, 343},
}
var autoRect = RECT{810, 283, 930, 335}
var manualRect = RECT{945, 283, 1065, 335}

func utf16(s string) *uint16 { p, _ := syscall.UTF16PtrFromString(s); return p }
func rgb(r,g,b byte) uintptr { return uintptr(uint32(r)|uint32(g)<<8|uint32(b)<<16) }
func lowWord(v uintptr) int { return int(uint16(v & 0xffff)) }
func highWord(v uintptr) int { return int(uint16((v >> 16) & 0xffff)) }
func inside(r RECT, x,y int) bool { return x>=int(r.Left)&&x<int(r.Right)&&y>=int(r.Top)&&y<int(r.Bottom) }

func createFont(height int, weight int, face string) syscall.Handle {
    r,_,_ := procCreateFontW.Call(uintptr(int32(-height)),0,0,0,uintptr(weight),0,0,0,1,0,0,0,0,uintptr(unsafe.Pointer(utf16(face))))
    return syscall.Handle(r)
}

func selectObj(hdc syscall.Handle, obj syscall.Handle) syscall.Handle {
    old,_,_ := procSelectObject.Call(uintptr(hdc),uintptr(obj))
    return syscall.Handle(old)
}

func withPen(hdc syscall.Handle, color uintptr, width int, fn func()) {
    p,_,_:=procCreatePen.Call(PS_SOLID,uintptr(width),color)
    pen:=syscall.Handle(p)
    old:=selectObj(hdc,pen)
    fn()
    selectObj(hdc,old)
    procDeleteObject.Call(uintptr(pen))
}

func fill(hdc syscall.Handle, r RECT, color uintptr) {
    b,_,_:=procCreateSolidBrush.Call(color)
    procFillRect.Call(uintptr(hdc),uintptr(unsafe.Pointer(&r)),b)
    procDeleteObject.Call(b)
}

func line(hdc syscall.Handle,x1,y1,x2,y2 int) {
    procMoveToEx.Call(uintptr(hdc),uintptr(x1),uintptr(y1),0)
    procLineTo.Call(uintptr(hdc),uintptr(x2),uintptr(y2))
}

func text(hdc syscall.Handle, s string, r RECT, color uintptr, font syscall.Handle, flags uintptr) {
    old:=selectObj(hdc,font)
    procSetTextColor.Call(uintptr(hdc),color)
    procSetBkMode.Call(uintptr(hdc),TRANSPARENT)
    p:=utf16(s)
    procDrawTextW.Call(uintptr(hdc),uintptr(unsafe.Pointer(p)),uintptr(^uint32(0)),uintptr(unsafe.Pointer(&r)),flags)
    selectObj(hdc,old)
}

func publish(status, detail string, progress int) {
    stateMu.Lock()
    if status!="" { statusLine=status }
    if detail!="" { detailLine=detail }
    if progress>=0 { progressStep=progress }
    stateMu.Unlock()
    if hwndMain!=0 { procPostMessageW.Call(uintptr(hwndMain),WM_APP_UI,0,0) }
}

func setVersionStatus(installed, latest string) {
    stateMu.Lock()
    installedVersion=installed
    latestVersion=latest
    versionLine=versionStatusText(installed,latest)
    stateMu.Unlock()
    if hwndMain!=0 { procPostMessageW.Call(uintptr(hwndMain),WM_APP_VERSION,0,0) }
}

func versionStatusText(installed,latest string) string {
    if installed=="none" {
        if latest!="unknown" { return "VERSION STATUS // Your VEK version is NOT INSTALLED // latest v"+latest }
        return "VERSION STATUS // Your VEK version is NOT INSTALLED // latest unknown"
    }
    if latest=="unknown" { return "VERSION STATUS // installed v"+installed+" // update status UNKNOWN" }
    a,b:=parseSemver(installed),parseSemver(latest)
    if a.ok&&b.ok&&compareSemver(a,b)<0 { return "VERSION STATUS // Your VEK version is OUTDATED // installed v"+installed+" // latest v"+latest }
    if a.ok&&b.ok&&compareSemver(a,b)==0 { return "VERSION STATUS // Your VEK version is UP TO DATE // v"+installed }
    if a.ok&&b.ok&&compareSemver(a,b)>0 { return "VERSION STATUS // installed v"+installed+" is newer than latest tag v"+latest }
    return "VERSION STATUS // installed "+installed+" // latest "+latest
}

func parseSemver(s string) semver {
    s=strings.TrimSpace(strings.TrimPrefix(strings.TrimPrefix(s,"v"),"V"))
    if i:=strings.IndexAny(s,"-+");i>=0{s=s[:i]}
    parts:=strings.Split(s,".")
    if len(parts)<3{return semver{}}
    a,e1:=strconv.Atoi(parts[0]);b,e2:=strconv.Atoi(parts[1]);c,e3:=strconv.Atoi(parts[2])
    return semver{a,b,c,e1==nil&&e2==nil&&e3==nil&&a>=0&&b>=0&&c>=0}
}
func compareSemver(a,b semver) int {
    if !a.ok||!b.ok{return 0}
    if a.major!=b.major{if a.major<b.major{return -1};return 1}
    if a.minor!=b.minor{if a.minor<b.minor{return -1};return 1}
    if a.patch!=b.patch{if a.patch<b.patch{return -1};return 1}
    return 0
}

func installedVersionFromDisk() string {
    b,err:=os.ReadFile(filepath.Join(targetDir,"VERSION"))
    if err!=nil{return "none"}
    s:=strings.TrimSpace(string(b)); if parseSemver(s).ok{return strings.TrimPrefix(s,"v")}
    return "none"
}

func findGit() (string,error) {
    if g,e:=exec.LookPath("git.exe");e==nil{return g,nil}
    return exec.LookPath("git")
}

func configuredRepo() string {
    exe,_:=os.Executable(); base:=filepath.Dir(exe)
    for _,p:=range []string{filepath.Join(base,"REPOSITORY.txt"),filepath.Join(targetDir,"REPOSITORY.txt")} {
        if b,e:=os.ReadFile(p);e==nil{
            s:=strings.TrimSpace(string(b)); lo:=strings.ToLower(s)
            if strings.HasPrefix(lo,"https://github.com/")&&strings.HasSuffix(lo,".git"){return s}
        }
    }
    return defaultRepo
}

func latestGitHubVersion(git,repo string) string {
    cmd:=exec.Command(git,"ls-remote","--tags","--refs",repo)
    cmd.Env=append(os.Environ(),"GIT_TERMINAL_PROMPT=0")
    out,err:=cmd.Output();if err!=nil{return "unknown"}
    versions:=[]semver{}
    labels:=map[[3]int]string{}
    for _,ln:=range strings.Split(string(out),"\n"){
        fields:=strings.Fields(ln);if len(fields)<2{continue}
        ref:=fields[1]
        const marker="refs/tags/v"
        i:=strings.Index(ref,marker);if i<0{continue}
        raw:=strings.TrimPrefix(ref[i+len("refs/tags/"):],"v")
        sv:=parseSemver(raw);if !sv.ok{continue}
        versions=append(versions,sv);labels[[3]int{sv.major,sv.minor,sv.patch}]=raw
    }
    if len(versions)==0{return "unknown"}
    sort.Slice(versions,func(i,j int)bool{return compareSemver(versions[i],versions[j])<0})
    v:=versions[len(versions)-1]
    return labels[[3]int{v.major,v.minor,v.patch}]
}

func policyFile() string { return filepath.Join(targetDir,"UPDATE_POLICY") }
func loadPolicy() updatePolicy {
    b,e:=os.ReadFile(policyFile());if e==nil&&strings.EqualFold(strings.TrimSpace(string(b)),"auto"){return policyAuto}
    return policyManual
}
func savePolicy(p updatePolicy) {
    _=os.MkdirAll(targetDir,0755)
    value:="manual\r\n";if p==policyAuto{value="auto\r\n"}
    _=os.WriteFile(policyFile(),[]byte(value),0644)
}

func checkVersionWorker() {
    installed:=installedVersionFromDisk()
    git,err:=findGit();if err!=nil{setVersionStatus(installed,"unknown");return}
    latest:=latestGitHubVersion(git,configuredRepo())
    setVersionStatus(installed,latest)
    stateMu.Lock();auto:=policy==policyAuto;busy:=installing;stateMu.Unlock()
    if auto&&!busy&&installed!="none"&&latest!="unknown"&&compareSemver(parseSemver(installed),parseSemver(latest))<0 {
        procPostMessageW.Call(uintptr(hwndMain),WM_APP_AUTO,0,0)
    }
}

func progressText(step int) string {
    if step<0{step=0};if step>10{step=10}
    return fmt.Sprintf("INSTALLING VEK - [%s%s] %d/10",strings.Repeat("#",step),strings.Repeat(".",10-step),step)
}

func stage(n int, status, detail string) {
    publish(status,detail,n)
    time.Sleep(130*time.Millisecond)
}

func startInstall(mode installMode) {
    stateMu.Lock()
    if installing { stateMu.Unlock(); return }
    installing=true;installDone=false;installFailed=false;currentMode=mode;completeAt=time.Time{}
    stateMu.Unlock()
    go installWorkflow(mode)
}

func installWorkflow(mode installMode) {
    modeName:="LATEST / UPDATE"
    if mode==modeRepair{modeName="CLEAN REPAIR"}
    if mode==modeSourceOnly{modeName="SOURCE ONLY"}
    stage(1,"PRE-FLIGHT // "+modeName,"Checking C:\\vek, Git, update policy and repository configuration...")

    git,err:=findGit();if err!=nil{fail("Git for Windows was not found on PATH. Install Git and run VEK installer again.");return}
    repo:=configuredRepo()
    stage(2,"GITHUB // repository selected",repo)
    if err:=os.MkdirAll(targetDir,0755);err!=nil{fail(err.Error());return}

    repoDir:=filepath.Join(targetDir,"repo")
    if mode==modeRepair {
        stage(3,"REPAIR // removing managed source checkout","Only C:\\vek\\repo is replaced; unrelated folders are untouched.")
        if err:=os.RemoveAll(repoDir);err!=nil{fail("Could not remove old managed repo: "+err.Error());return}
    } else {
        stage(3,"SOURCE // preparing managed checkout",repoDir)
    }

    stage(4,"DOWNLOAD // synchronizing GitHub source","SPACE // jump over obstacles while Git is working")
    if err:=syncRepo(git,repo,repoDir);err!=nil{fail("GitHub clone/update failed: "+err.Error());return}

    sourceVersion:=readSourceVersion(repoDir)
    if sourceVersion==""{sourceVersion=version}

    if mode==modeSourceOnly {
        stage(5,"SOURCE // checkout verified","VEK source v"+sourceVersion+" is available at "+repoDir)
        stage(6,"SOURCE // no launcher changes requested","Executable/PATH installation skipped by Source Only mode.")
        stage(7,"VERIFY // checking .git metadata",filepath.Join(repoDir,".git"))
        if _,e:=os.Stat(filepath.Join(repoDir,".git"));e!=nil{fail("Managed Git clone verification failed.");return}
        stage(8,"METADATA // recording repository",repo)
        _=os.WriteFile(filepath.Join(targetDir,"REPOSITORY.txt"),[]byte(repo+"\r\n"),0644)
        stage(9,"FINALIZE // source download ready","Build or inspect VEK directly from C:\\vek\\repo")
        complete("SOURCE DOWNLOADED // VEK v"+sourceVersion+" // auto close in 5 seconds")
        return
    }

    stage(5,"RUNTIME // deploying launcher and installer","Installing package executables into C:\\vek")
    if err:=copyPackageFiles();err!=nil{fail(err.Error());return}

    stage(6,"METADATA // recording installed runtime","VEK v"+sourceVersion+" // "+repo)
    if err:=os.WriteFile(filepath.Join(targetDir,"REPOSITORY.txt"),[]byte(repo+"\r\n"),0644);err!=nil{fail(err.Error());return}
    if err:=os.WriteFile(filepath.Join(targetDir,"VERSION"),[]byte(sourceVersion+"\r\n"),0644);err!=nil{fail(err.Error());return}
    savePolicy(policy)

    stage(7,"WINDOWS // registering User PATH",targetDir)
    if err:=addUserPath(targetDir);err!=nil{fail("PATH update failed: "+err.Error());return}

    stage(8,"VERIFY // checking managed clone and runtime",filepath.Join(repoDir,".git"))
    if _,e:=os.Stat(filepath.Join(repoDir,".git"));e!=nil{fail("Git clone verification failed.");return}
    if _,e:=os.Stat(filepath.Join(targetDir,"vek.exe"));e!=nil{fail("vek.exe deployment verification failed.");return}

    stage(9,"UPDATE POLICY // "+policyName(policy),"Auto checks happen when VEK installer/launcher performs its scheduled update check.")
    complete("VEK INSTALLED // v"+sourceVersion+" // You may close this window or it will auto-close in 5 seconds")
}

func policyName(p updatePolicy) string { if p==policyAuto{return "AUTO UPDATE"};return "MANUAL UPDATE" }

func readSourceVersion(repoDir string) string {
    b,e:=os.ReadFile(filepath.Join(repoDir,"VERSION"));if e!=nil{return ""}
    s:=strings.TrimSpace(string(b));if parseSemver(s).ok{return strings.TrimPrefix(s,"v")}
    return ""
}

func complete(msg string) {
    stage(10,"COMPLETE // VEK is ready",msg)
    stateMu.Lock();installing=false;installDone=true;installFailed=false;completeAt=time.Now();stateMu.Unlock()
    setVersionStatus(installedVersionFromDisk(),latestVersion)
    procPostMessageW.Call(uintptr(hwndMain),WM_APP_DONE,0,0)
}

func fail(msg string) {
    stateMu.Lock();installing=false;installFailed=true;installDone=false;statusLine="INSTALL ERROR // VEK was not installed";detailLine=msg;stateMu.Unlock()
    procPostMessageW.Call(uintptr(hwndMain),WM_APP_UI,0,0)
}

type progressWriter struct{ mu sync.Mutex; buf string }
func (w *progressWriter) Write(p []byte)(int,error){
    w.mu.Lock();w.buf+=string(p);textv:=strings.ReplaceAll(w.buf,"\r","\n");lines:=strings.Split(textv,"\n");if len(lines)>1{w.buf=lines[len(lines)-1]};w.mu.Unlock()
    for i:=len(lines)-2;i>=0;i--{ln:=strings.TrimSpace(lines[i]);if ln!=""{if len(ln)>150{ln=ln[:150]};publish("","GIT // "+ln,-1);break}}
    return len(p),nil
}
func runGitProgress(git string,args ...string)error{
    cmd:=exec.Command(git,args...);cmd.Env=append(os.Environ(),"GIT_TERMINAL_PROMPT=0")
    pw:=&progressWriter{};cmd.Stdout=pw;cmd.Stderr=pw
    if err:=cmd.Run();err!=nil{pw.mu.Lock();last:=strings.TrimSpace(pw.buf);pw.mu.Unlock();if last!=""{return fmt.Errorf("%v: %s",err,last)};return err};return nil
}
func syncRepo(git,repo,dir string)error{
    if _,e:=os.Stat(filepath.Join(dir,".git"));e==nil{
        if e=runGitProgress(git,"-C",dir,"remote","set-url","origin",repo);e!=nil{return e}
        if e=runGitProgress(git,"-C",dir,"fetch","--progress","--tags","--prune","origin");e!=nil{return e}
        if e=runGitProgress(git,"-C",dir,"reset","--hard","origin/main");e!=nil{return e}
        return nil
    }
    if _,e:=os.Stat(dir);e==nil{if e=os.RemoveAll(dir);e!=nil{return e}}
    return runGitProgress(git,"clone","--progress","--branch","main",repo,dir)
}

func copyPackageFiles()error{
    exe,e:=os.Executable();if e!=nil{return e};src:=filepath.Dir(exe)
    for _,name:=range []string{"vek.exe","VekInstaller.exe","README.md","manifest.sha256","REPOSITORY.txt"}{
        from:=filepath.Join(src,name);to:=filepath.Join(targetDir,name)
        if samePath(from,to){continue}
        data,er:=os.ReadFile(from);if er!=nil{if name=="README.md"||name=="manifest.sha256"||name=="REPOSITORY.txt"{continue};return er}
        mode:=os.FileMode(0644);if strings.HasSuffix(strings.ToLower(name),".exe"){mode=0755}
        if er=os.WriteFile(to,data,mode);er!=nil{return er}
    }
    return nil
}
func samePath(a,b string)bool{aa,_:=filepath.Abs(a);bb,_:=filepath.Abs(b);return strings.EqualFold(filepath.Clean(aa),filepath.Clean(bb))}

func addUserPath(dir string) error {
    var key syscall.Handle
    sub,_:=syscall.UTF16PtrFromString(`Environment`)
    if err:=syscall.RegOpenKeyEx(syscall.HKEY_CURRENT_USER,sub,0,syscall.KEY_QUERY_VALUE|syscall.KEY_SET_VALUE,&key);err!=nil{return err}
    defer syscall.RegCloseKey(key)
    name,_:=syscall.UTF16PtrFromString("Path")
    var typ uint32
    var n uint32
    _=syscall.RegQueryValueEx(key,name,nil,&typ,nil,&n)
    buf:=make([]byte,n+2)
    if n>0{_=syscall.RegQueryValueEx(key,name,nil,&typ,&buf[0],&n)}
    current:=""
    if len(buf)>=2{u16:=(*[1<<20]uint16)(unsafe.Pointer(&buf[0]))[:len(buf)/2:len(buf)/2];current=syscall.UTF16ToString(u16)}
    for _,entry:=range strings.Split(current,";"){if strings.EqualFold(filepath.Clean(strings.TrimSpace(entry)),filepath.Clean(dir)){return nil}}
    if current!=""&&!strings.HasSuffix(current,";"){current+=";"}
    current+=dir
    data,_:=syscall.UTF16FromString(current)
    bytes:=unsafe.Slice((*byte)(unsafe.Pointer(&data[0])),len(data)*2)
    if typ!=syscall.REG_SZ&&typ!=syscall.REG_EXPAND_SZ{typ=syscall.REG_EXPAND_SZ}
    r,_,callErr:=procRegSetValueExW.Call(uintptr(key),uintptr(unsafe.Pointer(name)),0,uintptr(typ),uintptr(unsafe.Pointer(&bytes[0])),uintptr(len(bytes)))
    if r!=0{return callErr}
    return nil
}

func updateAnimation() {
    now:=time.Now();dt:=now.Sub(lastFrame).Seconds();lastFrame=now
    if dt<=0||dt>0.08{dt=1.0/60.0}
    angle+=dt*0.9
    gridOffset=math.Mod(gridOffset+dt*210,80)

    playerVy+=1650*dt;playerY+=playerVy*dt
    floor:=636.0
    if playerY>=floor{playerY=floor;playerVy=0;onGround=true;playerRot=0}else{playerRot+=dt*4.8}
    speed:=270.0
    for i:=range obstacles{
        obstacles[i].x-=speed*dt
        if obstacles[i].x+obstacles[i].w<0{obstacles[i].x+=1120+float64(i*85);score++}
        if collidePlayer(obstacles[i]){
            crashes++;score=0;playerY=floor;playerVy=0;onGround=true;playerRot=0
            obstacles[i].x=360+float64(i*250)
        }
    }

    stateMu.Lock();done:=installDone;doneAt:=completeAt;stateMu.Unlock()
    if done&&!doneAt.IsZero()&&time.Since(doneAt)>=5*time.Second{procPostQuitMessage.Call(0)}
}

func collidePlayer(o obstacle)bool{
    px:=86.0;size:=30.0;py:=playerY
    left,right,top,bottom:=px,px+size,py,py+size
    ol,or:=o.x,o.x+o.w;ot,ob:=672-o.h,672.0
    if right<=ol||left>=or||bottom<=ot||top>=ob{return false}
    // Forgiving upper spike tip.
    if bottom<ob-o.h*0.35{return false}
    return true
}

func jump(){if onGround{playerVy=-620;onGround=false}}

func rotate(v vec3,a float64)vec3{
    cy,sy:=math.Cos(a),math.Sin(a);cp,sp:=math.Cos(a*0.31),math.Sin(a*0.31)
    x:=v.x*cy+v.z*sy;z:=-v.x*sy+v.z*cy;y:=v.y
    return vec3{x,y*cp-z*sp,y*sp+z*cp}
}
func project(v vec3,center vec2,focal,camera float64)vec2{
    r:=rotate(v,angle);den:=camera-r.z;if den<0.2{den=0.2};s:=focal/den
    return vec2{center.x+r.x*s,center.y-r.y*s}
}

type seg3 struct{a,b vec3}
func vekSegments()[]seg3{
    // Wireframe block letters with front/back depth and connector edges.
    base:=[]seg3{
        {vec3{-3.6,1.2,0},vec3{-2.9,-1.2,0}},{vec3{-2.9,-1.2,0},vec3{-2.2,1.2,0}}, // V
        {vec3{-1.5,1.2,0},vec3{-1.5,-1.2,0}},{vec3{-1.5,1.2,0},vec3{-0.1,1.2,0}},{vec3{-1.5,0,0},vec3{-0.35,0,0}},{vec3{-1.5,-1.2,0},vec3{-0.1,-1.2,0}}, // E
        {vec3{0.7,1.2,0},vec3{0.7,-1.2,0}},{vec3{0.7,0,0},vec3{2.2,1.2,0}},{vec3{0.7,0,0},vec3{2.25,-1.2,0}}, // K
    }
    out:=make([]seg3,0,len(base)*3)
    for _,s:=range base{
        f:=s;f.a.z=0.28;f.b.z=0.28;b:=s;b.a.z=-0.28;b.b.z=-0.28
        out=append(out,f,b,seg3{f.a,b.a})
    }
    return out
}

func draw3DLogo(hdc syscall.Handle) {
    center:=vec2{555,135};segs:=vekSegments()
    withPen(hdc,rgb(10,70,44),7,func(){for _,s:=range segs{a:=project(s.a,center,150,5.4);b:=project(s.b,center,150,5.4);line(hdc,int(a.x+4),int(a.y+5),int(b.x+4),int(b.y+5))}})
    withPen(hdc,rgb(72,255,155),3,func(){for _,s:=range segs{a:=project(s.a,center,150,5.4);b:=project(s.b,center,150,5.4);line(hdc,int(a.x),int(a.y),int(b.x),int(b.y))}})
    text(hdc,"VEK // PROGRAMMING LANGUAGE",RECT{390,225,720,252},rgb(165,255,207),fontBody,DT_CENTER|DT_VCENTER|DT_SINGLELINE)
}

func drawLoadingCube(hdc syscall.Handle, step int) {
    if step<=0{return}
    c:=vec2{936,144};verts:=[]vec3{{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}}
    edges:=[][2]int{{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}}
    withPen(hdc,rgb(70,220,255),2,func(){for _,e:=range edges{a:=project(verts[e[0]],c,72,4.8);b:=project(verts[e[1]],c,72,4.8);line(hdc,int(a.x),int(a.y),int(b.x),int(b.y))}})
    text(hdc,fmt.Sprintf("3D LOAD // %d/10",step),RECT{850,225,1025,250},rgb(130,235,255),fontSmall,DT_CENTER|DT_SINGLELINE)
}

func drawButton(hdc syscall.Handle,r RECT,label,sub string,active bool) {
    bg:=rgb(13,27,22);border:=rgb(50,125,89);fg:=rgb(158,255,203)
    if active{bg=rgb(20,67,46);border=rgb(76,255,157);fg=rgb(235,255,244)}
    b,_,_:=procCreateSolidBrush.Call(bg);oldB:=selectObj(hdc,syscall.Handle(b))
    p,_,_:=procCreatePen.Call(PS_SOLID,2,border);oldP:=selectObj(hdc,syscall.Handle(p))
    procRoundRect.Call(uintptr(hdc),uintptr(r.Left),uintptr(r.Top),uintptr(r.Right),uintptr(r.Bottom),14,14)
    selectObj(hdc,oldP);selectObj(hdc,oldB);procDeleteObject.Call(p);procDeleteObject.Call(b)
    text(hdc,label,RECT{r.Left,r.Top+7,r.Right,r.Bottom-16},fg,fontButton,DT_CENTER|DT_VCENTER|DT_SINGLELINE)
    text(hdc,sub,RECT{r.Left,r.Bottom-20,r.Right,r.Bottom-3},rgb(91,190,139),fontSmall,DT_CENTER|DT_SINGLELINE)
}

func drawPolicyButton(hdc syscall.Handle,r RECT,label string,active bool) {
    drawButton(hdc,r,label,"UPDATE POLICY",active)
}

func drawRunner(hdc syscall.Handle) {
    top:=455;ground:=672
    fill(hdc,RECT{0,int32(top),windowW,windowH},rgb(4,10,14))
    withPen(hdc,rgb(15,44,50),1,func(){
        for y:=top+25;y<ground;y+=30{line(hdc,0,y,windowW,y)}
        for x:=-80+int(gridOffset);x<windowW+80;x+=80{line(hdc,x,ground,x+220,top)}
    })
    withPen(hdc,rgb(65,255,155),3,func(){line(hdc,0,ground,windowW,ground)})

    // Obstacles.
    for _,o:=range obstacles{
        pts:=[]POINT{{int32(o.x),int32(ground)},{int32(o.x+o.w/2),int32(float64(ground)-o.h)},{int32(o.x+o.w),int32(ground)}}
        b,_,_:=procCreateSolidBrush.Call(rgb(38,155,105));oldB:=selectObj(hdc,syscall.Handle(b))
        p,_,_:=procCreatePen.Call(PS_SOLID,2,rgb(110,255,192));oldP:=selectObj(hdc,syscall.Handle(p))
        procPolygon.Call(uintptr(hdc),uintptr(unsafe.Pointer(&pts[0])),uintptr(len(pts)))
        selectObj(hdc,oldP);selectObj(hdc,oldB);procDeleteObject.Call(p);procDeleteObject.Call(b)
    }

    // Rotating player square.
    cx,cy:=101.0,playerY+15;half:=15.0;c,s:=math.Cos(playerRot),math.Sin(playerRot)
    local:=[][2]float64{{-half,-half},{half,-half},{half,half},{-half,half}}
    pts:=make([]POINT,4)
    for i,p:=range local{pts[i]=POINT{int32(cx+p[0]*c-p[1]*s),int32(cy+p[0]*s+p[1]*c)}}
    b,_,_:=procCreateSolidBrush.Call(rgb(72,255,155));oldB:=selectObj(hdc,syscall.Handle(b))
    p,_,_:=procCreatePen.Call(PS_SOLID,2,rgb(218,255,235));oldP:=selectObj(hdc,syscall.Handle(p))
    procPolygon.Call(uintptr(hdc),uintptr(unsafe.Pointer(&pts[0])),4)
    selectObj(hdc,oldP);selectObj(hdc,oldB);procDeleteObject.Call(p);procDeleteObject.Call(b)

    text(hdc,"PLAY WHILE VEK DOWNLOADS // PRESS SPACE TO JUMP",RECT{38,466,630,492},rgb(177,255,213),fontBody,DT_LEFT|DT_SINGLELINE)
    text(hdc,fmt.Sprintf("SCORE %04d   CRASHES %02d",score,crashes),RECT{790,466,1060,492},rgb(114,224,255),fontBody,DT_RIGHT|DT_SINGLELINE)
}

func paint(hwnd syscall.Handle) {
    var ps PAINTSTRUCT
    hdcRaw,_,_:=procBeginPaint.Call(uintptr(hwnd),uintptr(unsafe.Pointer(&ps)));hdc:=syscall.Handle(hdcRaw)
    var rc RECT;procGetClientRect.Call(uintptr(hwnd),uintptr(unsafe.Pointer(&rc)))
    fill(hdc,rc,rgb(5,11,10))

    // Ambient moving background behind installer controls.
    withPen(hdc,rgb(10,32,28),1,func(){
        for x:=-120+int(gridOffset);x<windowW+160;x+=80{line(hdc,x,0,x+150,455)}
        for y:=20;y<455;y+=40{line(hdc,0,y,windowW,y)}
    })

    draw3DLogo(hdc)

    stateMu.Lock()
    st,det,ver,step,busy,done,failed,pol:=statusLine,detailLine,versionLine,progressStep,installing,installDone,installFailed,policy
    stateMu.Unlock()

    labels:=[]string{"LATEST / UPDATE","CLEAN REPAIR","SOURCE ONLY"}
    for i,r:=range downloadButtons{drawButton(hdc,r,"DOWNLOAD",labels[i],busy&&currentMode==installMode(i))}
    drawPolicyButton(hdc,autoRect,"AUTO",pol==policyAuto)
    drawPolicyButton(hdc,manualRect,"MANUAL",pol==policyManual)

    fill(hdc,RECT{38,360,1070,438},rgb(7,18,16))
    text(hdc,ver,RECT{52,369,1055,392},rgb(108,235,255),fontBody,DT_LEFT|DT_SINGLELINE)
    col:=rgb(119,255,179);if failed{col=rgb(255,110,110)};if done{col=rgb(185,255,210)}
    text(hdc,st,RECT{52,398,740,421},col,fontBody,DT_LEFT|DT_SINGLELINE)
    text(hdc,progressText(step),RECT{750,398,1055,421},rgb(119,255,179),fontBody,DT_RIGHT|DT_SINGLELINE)
    text(hdc,det,RECT{52,425,1055,452},rgb(103,178,145),fontSmall,DT_LEFT|DT_SINGLELINE)

    if busy||step>0{drawLoadingCube(hdc,step)}
    drawRunner(hdc)

    procEndPaint.Call(uintptr(hwnd),uintptr(unsafe.Pointer(&ps)))
}

func wndProc(hwnd syscall.Handle,msg uint32,wParam,lParam uintptr)uintptr{
    switch msg{
    case WM_DESTROY:
        procPostQuitMessage.Call(0);return 0
    case WM_CLOSE:
        stateMu.Lock();busy:=installing;stateMu.Unlock()
        if busy{publish("INSTALLING // close disabled until current Git operation finishes","You can keep playing the runner with SPACE.",-1);return 0}
    case WM_PAINT:
        paint(hwnd);return 0
    case WM_TIMER:
        if wParam==1{updateAnimation();procInvalidateRect.Call(uintptr(hwnd),0,0);return 0}
    case WM_KEYDOWN:
        if wParam==VK_SPACE{jump();return 0}
    case WM_LBUTTONDOWN:
        x,y:=lowWord(lParam),highWord(lParam)
        stateMu.Lock();busy:=installing;stateMu.Unlock()
        if !busy{
            for i,r:=range downloadButtons{if inside(r,x,y){startInstall(installMode(i));return 0}}
            if inside(autoRect,x,y){stateMu.Lock();policy=policyAuto;stateMu.Unlock();savePolicy(policyAuto);publish("UPDATE POLICY // AUTO","If an installed VEK is behind the latest GitHub release, the installer can begin the update automatically.",-1);go checkVersionWorker();return 0}
            if inside(manualRect,x,y){stateMu.Lock();policy=policyManual;stateMu.Unlock();savePolicy(policyManual);publish("UPDATE POLICY // MANUAL","VEK will report update status but wait for you to click DOWNLOAD.",-1);return 0}
        }
    case WM_APP_UI,WM_APP_VERSION:
        procInvalidateRect.Call(uintptr(hwnd),0,0);return 0
    case WM_APP_DONE:
        procInvalidateRect.Call(uintptr(hwnd),0,0);return 0
    case WM_APP_AUTO:
        stateMu.Lock();busy:=installing;auto:=policy==policyAuto;stateMu.Unlock()
        if !busy&&auto{publish("AUTO UPDATE // newer VEK detected","Starting GitHub update automatically...",0);startInstall(modeInstall)}
        return 0
    }
    r,_,_:=procDefWindowProcW.Call(uintptr(hwnd),uintptr(msg),wParam,lParam);return r
}

func main(){
    runtime.LockOSThread()
    policy=loadPolicy()
    fontSmall=createFont(14,400,"Consolas")
    fontBody=createFont(17,500,"Consolas")
    fontButton=createFont(19,700,"Consolas")
    fontTitle=createFont(28,700,"Consolas")

    hi,_,_:=procGetModuleHandleW.Call(0);hInst:=syscall.Handle(hi)
    className:=utf16("VEKInteractiveGithubInstaller")
    wc:=WNDCLASSEX{CbSize:uint32(unsafe.Sizeof(WNDCLASSEX{})),LpfnWndProc:syscall.NewCallback(wndProc),HInstance:hInst,LpszClassName:className}
    // Background is painted fully in WM_PAINT.
    if r,_,e:=procRegisterClassExW.Call(uintptr(unsafe.Pointer(&wc)));r==0{panic(e)}
    raw,_,e:=procCreateWindowExW.Call(0,uintptr(unsafe.Pointer(className)),uintptr(unsafe.Pointer(utf16("VEK // Interactive GitHub Installer"))),WS_OVERLAPPEDWINDOW|WS_VISIBLE,90,50,windowW,windowH,0,0,uintptr(hInst),0)
    if raw==0{panic(e)};hwndMain=syscall.Handle(raw)
    procShowWindow.Call(uintptr(hwndMain),SW_SHOW);procUpdateWindow.Call(uintptr(hwndMain));procSetTimer.Call(uintptr(hwndMain),1,16,0)

    go checkVersionWorker()
    var msg MSG
    for{
        r,_,_:=procGetMessageW.Call(uintptr(unsafe.Pointer(&msg)),0,0,0)
        if int32(r)<=0{break}
        procTranslateMessage.Call(uintptr(unsafe.Pointer(&msg)))
        procDispatchMessageW.Call(uintptr(unsafe.Pointer(&msg)))
    }
    procKillTimer.Call(uintptr(hwndMain),1)
}
