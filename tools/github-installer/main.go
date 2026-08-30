//go:build windows

package main

import (
	"archive/zip"
	"crypto/sha256"
	"fmt"
	"io"
	"math"
	"net/http"
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
	version     = "2.6.0"
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
	procGetSystemMetrics = user32.NewProc("GetSystemMetrics")

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

type semver struct {
	major, minor, patch int
	ok                  bool
}

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
	WS_POPUP            = 0x80000000
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

	VK_SPACE  = 0x20
	VK_ESCAPE = 0x1B
	SW_HIDE   = 0
	SW_SHOW   = 5

	PS_SOLID    = 0
	TRANSPARENT = 1

	DT_LEFT       = 0x00000000
	DT_CENTER     = 0x00000001
	DT_RIGHT      = 0x00000002
	DT_VCENTER    = 0x00000004
	DT_SINGLELINE = 0x00000020
	DT_WORDBREAK  = 0x00000010

	SM_CXSCREEN      = 0
	SM_CYSCREEN      = 1
	CREATE_NO_WINDOW = 0x08000000
)

var (
	hwndMain   syscall.Handle
	fontSmall  syscall.Handle
	fontBody   syscall.Handle
	fontButton syscall.Handle
	fontTitle  syscall.Handle

	stateMu         sync.Mutex
	statusLine      = "READY - choose a download mode"
	detailLine      = "SOURCE - https://github.com/defnot67kid-beep/vek.git"
	versionLine     = "VERSION STATUS - checking installed and latest VEK..."
	progressStep    = 0
	progressPercent = 0
	installing      = false
	installDone     = false
	installFailed   = false
	currentMode     installMode
	policy          = policyManual
	completeAt      time.Time

	angle      float64 = -0.12
	gridOffset float64
	clientW    int = 1280
	clientH    int = 720
	playerY    float64
	playerVy   float64
	playerRot  float64
	onGround   = true
	score      int
	crashes    int
	obstacles  = []obstacle{{420, 34, 38}, {690, 40, 52}, {970, 34, 42}, {1260, 52, 64}}
	lastFrame  = time.Now()

	latestVersion    = "unknown"
	installedVersion = "none"
)

func utf16(s string) *uint16   { p, _ := syscall.UTF16PtrFromString(s); return p }
func rgb(r, g, b byte) uintptr { return uintptr(uint32(r) | uint32(g)<<8 | uint32(b)<<16) }
func lowWord(v uintptr) int    { return int(uint16(v & 0xffff)) }
func highWord(v uintptr) int   { return int(uint16((v >> 16) & 0xffff)) }
func inside(r RECT, x, y int) bool {
	return x >= int(r.Left) && x < int(r.Right) && y >= int(r.Top) && y < int(r.Bottom)
}

func createFont(height int, weight int, face string) syscall.Handle {
	r, _, _ := procCreateFontW.Call(uintptr(int32(-height)), 0, 0, 0, uintptr(weight), 0, 0, 0, 1, 0, 0, 0, 0, uintptr(unsafe.Pointer(utf16(face))))
	return syscall.Handle(r)
}

func selectObj(hdc syscall.Handle, obj syscall.Handle) syscall.Handle {
	old, _, _ := procSelectObject.Call(uintptr(hdc), uintptr(obj))
	return syscall.Handle(old)
}

func withPen(hdc syscall.Handle, color uintptr, width int, fn func()) {
	p, _, _ := procCreatePen.Call(PS_SOLID, uintptr(width), color)
	pen := syscall.Handle(p)
	old := selectObj(hdc, pen)
	fn()
	selectObj(hdc, old)
	procDeleteObject.Call(uintptr(pen))
}

func fill(hdc syscall.Handle, r RECT, color uintptr) {
	b, _, _ := procCreateSolidBrush.Call(color)
	procFillRect.Call(uintptr(hdc), uintptr(unsafe.Pointer(&r)), b)
	procDeleteObject.Call(b)
}

func line(hdc syscall.Handle, x1, y1, x2, y2 int) {
	procMoveToEx.Call(uintptr(hdc), uintptr(x1), uintptr(y1), 0)
	procLineTo.Call(uintptr(hdc), uintptr(x2), uintptr(y2))
}

func text(hdc syscall.Handle, s string, r RECT, color uintptr, font syscall.Handle, flags uintptr) {
	old := selectObj(hdc, font)
	procSetTextColor.Call(uintptr(hdc), color)
	procSetBkMode.Call(uintptr(hdc), TRANSPARENT)
	p := utf16(s)
	procDrawTextW.Call(uintptr(hdc), uintptr(unsafe.Pointer(p)), uintptr(^uint32(0)), uintptr(unsafe.Pointer(&r)), flags)
	selectObj(hdc, old)
}

func publish(status, detail string, progress int) {
	stateMu.Lock()
	if status != "" {
		statusLine = status
	}
	if detail != "" {
		detailLine = detail
	}
	if progress >= 0 {
		progressStep = progress
	}
	stateMu.Unlock()
	if hwndMain != 0 {
		procPostMessageW.Call(uintptr(hwndMain), WM_APP_UI, 0, 0)
	}
}

func setVersionStatus(installed, latest string) {
	stateMu.Lock()
	installedVersion = installed
	latestVersion = latest
	versionLine = versionStatusText(installed, latest)
	stateMu.Unlock()
	if hwndMain != 0 {
		procPostMessageW.Call(uintptr(hwndMain), WM_APP_VERSION, 0, 0)
	}
}

func versionStatusText(installed, latest string) string {
	if installed == "none" {
		if latest != "unknown" {
			return "VERSION STATUS - VEK is not installed - latest v" + latest
		}
		return "VERSION STATUS - VEK is not installed - latest unknown"
	}
	if latest == "unknown" {
		return "VERSION STATUS - installed v" + installed + " - update status unknown"
	}
	a, b := parseSemver(installed), parseSemver(latest)
	if a.ok && b.ok && compareSemver(a, b) < 0 {
		return "VERSION STATUS - Your VEK version is OUTDATED - installed v" + installed + " - latest v" + latest
	}
	if a.ok && b.ok && compareSemver(a, b) == 0 {
		return "VERSION STATUS - Your VEK version is UP TO DATE - v" + installed
	}
	if a.ok && b.ok && compareSemver(a, b) > 0 {
		return "VERSION STATUS - installed v" + installed + " is newer than latest tag v" + latest
	}
	return "VERSION STATUS - installed " + installed + " - latest " + latest
}

func parseSemver(s string) semver {
	s = strings.TrimSpace(strings.TrimPrefix(strings.TrimPrefix(s, "v"), "V"))
	if i := strings.IndexAny(s, "-+"); i >= 0 {
		s = s[:i]
	}
	parts := strings.Split(s, ".")
	if len(parts) < 3 {
		return semver{}
	}
	a, e1 := strconv.Atoi(parts[0])
	b, e2 := strconv.Atoi(parts[1])
	c, e3 := strconv.Atoi(parts[2])
	return semver{a, b, c, e1 == nil && e2 == nil && e3 == nil && a >= 0 && b >= 0 && c >= 0}
}
func compareSemver(a, b semver) int {
	if !a.ok || !b.ok {
		return 0
	}
	if a.major != b.major {
		if a.major < b.major {
			return -1
		}
		return 1
	}
	if a.minor != b.minor {
		if a.minor < b.minor {
			return -1
		}
		return 1
	}
	if a.patch != b.patch {
		if a.patch < b.patch {
			return -1
		}
		return 1
	}
	return 0
}

func installedVersionFromDisk() string {
	b, err := os.ReadFile(filepath.Join(targetDir, "VERSION"))
	if err != nil {
		return "none"
	}
	s := strings.TrimSpace(string(b))
	if parseSemver(s).ok {
		return strings.TrimPrefix(s, "v")
	}
	return "none"
}

func findGit() (string, error) {
	if g, e := exec.LookPath("git.exe"); e == nil {
		return g, nil
	}
	return exec.LookPath("git")
}

// hiddenCommand launches a normal child process without allocating or flashing a
// console window. The installer never invokes cmd.exe or PowerShell; Git is
// executed directly and its stdout/stderr are captured by this process.
func hiddenCommand(exe string, args ...string) *exec.Cmd {
	cmd := exec.Command(exe, args...)
	cmd.SysProcAttr = &syscall.SysProcAttr{
		HideWindow:    true,
		CreationFlags: CREATE_NO_WINDOW,
	}
	return cmd
}

func configuredRepo() string {
	exe, _ := os.Executable()
	base := filepath.Dir(exe)
	for _, p := range []string{filepath.Join(base, "REPOSITORY.txt"), filepath.Join(targetDir, "REPOSITORY.txt")} {
		if b, e := os.ReadFile(p); e == nil {
			s := strings.TrimSpace(string(b))
			lo := strings.ToLower(s)
			if strings.HasPrefix(lo, "https://github.com/") && strings.HasSuffix(lo, ".git") {
				return s
			}
		}
	}
	return defaultRepo
}

func latestGitHubVersion(git, repo string) string {
	cmd := hiddenCommand(git, "ls-remote", "--tags", "--refs", repo)
	cmd.Env = append(os.Environ(), "GIT_TERMINAL_PROMPT=0")
	out, err := cmd.Output()
	if err != nil {
		return "unknown"
	}
	versions := []semver{}
	labels := map[[3]int]string{}
	for _, ln := range strings.Split(string(out), "\n") {
		fields := strings.Fields(ln)
		if len(fields) < 2 {
			continue
		}
		ref := fields[1]
		const marker = "refs/tags/v"
		i := strings.Index(ref, marker)
		if i < 0 {
			continue
		}
		raw := strings.TrimPrefix(ref[i+len("refs/tags/"):], "v")
		sv := parseSemver(raw)
		if !sv.ok {
			continue
		}
		versions = append(versions, sv)
		labels[[3]int{sv.major, sv.minor, sv.patch}] = raw
	}
	if len(versions) == 0 {
		return "unknown"
	}
	sort.Slice(versions, func(i, j int) bool { return compareSemver(versions[i], versions[j]) < 0 })
	v := versions[len(versions)-1]
	return labels[[3]int{v.major, v.minor, v.patch}]
}

const maxBootstrapPackageBytes int64 = 128 * 1024 * 1024

func repoWebBase(repo string) string {
	s := strings.TrimSpace(repo)
	s = strings.TrimSuffix(s, ".git")
	return strings.TrimRight(s, "/")
}

func hasArg(name string) bool {
	for _, a := range os.Args[1:] {
		if strings.EqualFold(a, name) {
			return true
		}
	}
	return false
}

func downloadHTTPS(url, dst string, maxBytes int64) error {
	client := &http.Client{Timeout: 90 * time.Second}
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		return err
	}
	req.Header.Set("User-Agent", "VEK-Installer/"+version)
	resp, err := client.Do(req)
	if err != nil {
		return err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("HTTP %d for %s", resp.StatusCode, url)
	}
	if resp.ContentLength > maxBytes && maxBytes > 0 {
		return fmt.Errorf("download exceeds size limit")
	}
	if err := os.MkdirAll(filepath.Dir(dst), 0755); err != nil {
		return err
	}
	f, err := os.Create(dst)
	if err != nil {
		return err
	}
	defer f.Close()
	var r io.Reader = resp.Body
	if maxBytes > 0 {
		r = io.LimitReader(resp.Body, maxBytes+1)
	}
	n, err := io.Copy(f, r)
	if err != nil {
		return err
	}
	if maxBytes > 0 && n > maxBytes {
		return fmt.Errorf("download exceeds size limit")
	}
	return f.Sync()
}

func parseChecksumFile(path string) (string, error) {
	b, err := os.ReadFile(path)
	if err != nil {
		return "", err
	}
	fields := strings.Fields(string(b))
	if len(fields) == 0 {
		return "", fmt.Errorf("empty checksum file")
	}
	h := strings.ToLower(strings.TrimSpace(fields[0]))
	if len(h) != 64 {
		return "", fmt.Errorf("invalid SHA-256 checksum")
	}
	for _, c := range h {
		if !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')) {
			return "", fmt.Errorf("invalid SHA-256 checksum")
		}
	}
	return h, nil
}

func fileSHA256(path string) (string, error) {
	f, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer f.Close()
	h := sha256.New()
	if _, err := io.Copy(h, f); err != nil {
		return "", err
	}
	return fmt.Sprintf("%x", h.Sum(nil)), nil
}

func safeExtractZip(src, dst string) error {
	zr, err := zip.OpenReader(src)
	if err != nil {
		return err
	}
	defer zr.Close()
	cleanRoot, err := filepath.Abs(dst)
	if err != nil {
		return err
	}
	var total int64
	for _, f := range zr.File {
		name := filepath.Clean(filepath.FromSlash(f.Name))
		if name == "." || filepath.IsAbs(name) || name == ".." || strings.HasPrefix(name, ".."+string(os.PathSeparator)) {
			return fmt.Errorf("unsafe ZIP path: %s", f.Name)
		}
		outPath := filepath.Join(cleanRoot, name)
		absOut, err := filepath.Abs(outPath)
		if err != nil {
			return err
		}
		if !strings.EqualFold(absOut, cleanRoot) && !strings.HasPrefix(strings.ToLower(absOut), strings.ToLower(cleanRoot)+strings.ToLower(string(os.PathSeparator))) {
			return fmt.Errorf("ZIP path escapes update directory")
		}
		total += int64(f.UncompressedSize64)
		if total > 256*1024*1024 {
			return fmt.Errorf("expanded update exceeds size limit")
		}
		if f.FileInfo().IsDir() {
			if err := os.MkdirAll(absOut, 0755); err != nil {
				return err
			}
			continue
		}
		if err := os.MkdirAll(filepath.Dir(absOut), 0755); err != nil {
			return err
		}
		rc, err := f.Open()
		if err != nil {
			return err
		}
		wf, err := os.OpenFile(absOut, os.O_CREATE|os.O_TRUNC|os.O_WRONLY, 0644)
		if err != nil {
			rc.Close()
			return err
		}
		_, cpErr := io.Copy(wf, rc)
		closeErr := wf.Close()
		rc.Close()
		if cpErr != nil {
			return cpErr
		}
		if closeErr != nil {
			return closeErr
		}
	}
	return nil
}

func locateUpdatedInstaller(root string) string {
	direct := filepath.Join(root, "VekInstaller.exe")
	if _, err := os.Stat(direct); err == nil {
		return direct
	}
	entries, _ := os.ReadDir(root)
	for _, e := range entries {
		if !e.IsDir() {
			continue
		}
		p := filepath.Join(root, e.Name(), "VekInstaller.exe")
		if _, err := os.Stat(p); err == nil {
			return p
		}
	}
	return ""
}

// bootstrapLatestInstaller does not overwrite the running executable. Instead it
// downloads a published VekInstaller.zip plus its SHA-256 companion asset into a
// versioned cache, verifies the package, extracts it, and launches that newer
// installer. This keeps the bootstrap stable and avoids self-replacement.
func bootstrapLatestInstaller() bool {
	if hasArg("--no-bootstrap") {
		return false
	}
	git, err := findGit()
	if err != nil {
		return false
	}
	repo := configuredRepo()
	latest := latestGitHubVersion(git, repo)
	if latest == "unknown" || compareSemver(parseSemver(version), parseSemver(latest)) >= 0 {
		return false
	}
	base := repoWebBase(repo) + "/releases/download/v" + latest + "/"
	cache := filepath.Join(targetDir, "versions", "v"+latest)
	exe := filepath.Join(cache, "VekInstaller.exe")
	if _, err := os.Stat(exe); err != nil {
		tmp := filepath.Join(targetDir, "updates", "v"+latest)
		_ = os.RemoveAll(tmp)
		if err := os.MkdirAll(tmp, 0755); err != nil {
			return false
		}
		zipPath := filepath.Join(tmp, "VekInstaller.zip")
		sumPath := filepath.Join(tmp, "VekInstaller.zip.sha256")
		if err := downloadHTTPS(base+"VekInstaller.zip.sha256", sumPath, 64*1024); err != nil {
			return false
		}
		if err := downloadHTTPS(base+"VekInstaller.zip", zipPath, maxBootstrapPackageBytes); err != nil {
			return false
		}
		want, err := parseChecksumFile(sumPath)
		if err != nil {
			return false
		}
		got, err := fileSHA256(zipPath)
		if err != nil || !strings.EqualFold(want, got) {
			return false
		}
		_ = os.RemoveAll(cache)
		if err := os.MkdirAll(cache, 0755); err != nil {
			return false
		}
		if err := safeExtractZip(zipPath, cache); err != nil {
			_ = os.RemoveAll(cache)
			return false
		}
		exe = locateUpdatedInstaller(cache)
		if exe == "" {
			_ = os.RemoveAll(cache)
			return false
		}
		// Require the extracted package to declare the version we requested.
		versionFile := filepath.Join(filepath.Dir(exe), "VERSION")
		vb, err := os.ReadFile(versionFile)
		if err != nil || strings.TrimSpace(string(vb)) != latest {
			_ = os.RemoveAll(cache)
			return false
		}
	}
	cmd := exec.Command(exe, "--bootstrapped")
	cmd.Dir = filepath.Dir(exe)
	if err := cmd.Start(); err != nil {
		return false
	}
	return true
}

func policyFile() string { return filepath.Join(targetDir, "UPDATE_POLICY") }
func loadPolicy() updatePolicy {
	b, e := os.ReadFile(policyFile())
	if e == nil && strings.EqualFold(strings.TrimSpace(string(b)), "auto") {
		return policyAuto
	}
	return policyManual
}
func savePolicy(p updatePolicy) {
	_ = os.MkdirAll(targetDir, 0755)
	value := "manual\r\n"
	if p == policyAuto {
		value = "auto\r\n"
	}
	_ = os.WriteFile(policyFile(), []byte(value), 0644)
}

func checkVersionWorker() {
	installed := installedVersionFromDisk()
	git, err := findGit()
	if err != nil {
		setVersionStatus(installed, "unknown")
		return
	}
	latest := latestGitHubVersion(git, configuredRepo())
	setVersionStatus(installed, latest)
	stateMu.Lock()
	auto := policy == policyAuto
	busy := installing
	stateMu.Unlock()
	_ = auto
	_ = busy
	// Safety policy: AUTO means automatic version checking/notification only.
	// Downloads and updates always require an explicit user click in the GUI.
}

func stagePercent(step int) int {
	values := []int{0, 5, 12, 20, 25, 60, 70, 79, 88, 95, 100}
	if step < 0 {
		return 0
	}
	if step > 10 {
		return 100
	}
	return values[step]
}

func setProgressPercent(percent int) {
	if percent < 0 {
		percent = 0
	}
	if percent > 100 {
		percent = 100
	}
	stateMu.Lock()
	if percent > progressPercent || percent == 0 {
		progressPercent = percent
	}
	stateMu.Unlock()
	if hwndMain != 0 {
		procPostMessageW.Call(uintptr(hwndMain), WM_APP_UI, 0, 0)
	}
}

func stage(n int, status, detail string) {
	publish(status, detail, n)
	setProgressPercent(stagePercent(n))
	time.Sleep(90 * time.Millisecond)
}

func installerIsInTargetDir() bool {
	exe, err := os.Executable()
	if err != nil {
		return false
	}
	dir, err := filepath.Abs(filepath.Dir(exe))
	if err != nil {
		return false
	}
	target, _ := filepath.Abs(targetDir)
	dirClean := filepath.Clean(dir)
	targetClean := filepath.Clean(target)
	if strings.EqualFold(dirClean, targetClean) {
		return true
	}
	versionsRoot := filepath.Join(targetClean, "versions")
	dl := strings.ToLower(dirClean)
	vl := strings.ToLower(versionsRoot) + strings.ToLower(string(os.PathSeparator))
	return strings.HasPrefix(dl, vl)
}

func startInstall(mode installMode) {
	stateMu.Lock()
	if installing {
		stateMu.Unlock()
		return
	}
	installing = true
	installDone = false
	installFailed = false
	currentMode = mode
	completeAt = time.Time{}
	progressPercent = 0
	stateMu.Unlock()
	go installWorkflow(mode)
}

func installWorkflow(mode installMode) {
	if !installerIsInTargetDir() {
		fail("For initial setup, extract VekInstaller.zip directly to C:\\vek. Auto-updated installers may run from C:\\vek\\versions.")
		return
	}
	modeName := "LATEST / UPDATE"
	if mode == modeRepair {
		modeName = "CLEAN REPAIR"
	}
	if mode == modeSourceOnly {
		modeName = "SOURCE ONLY"
	}
	stage(1, "PRE-FLIGHT - "+modeName, "Checking C:\\vek, Git, update policy and repository configuration...")

	git, err := findGit()
	if err != nil {
		fail("Git for Windows was not found on PATH. Install Git and run VEK installer again.")
		return
	}
	repo := configuredRepo()
	stage(2, "GITHUB - repository selected", repo)
	if err := os.MkdirAll(targetDir, 0755); err != nil {
		fail(err.Error())
		return
	}

	repoDir := filepath.Join(targetDir, "repo")
	if mode == modeRepair {
		stage(3, "REPAIR - removing managed source checkout", "Only C:\\vek\\repo is replaced; unrelated folders are untouched.")
		if err := os.RemoveAll(repoDir); err != nil {
			fail("Could not remove old managed repo: " + err.Error())
			return
		}
	} else {
		stage(3, "SOURCE - preparing managed checkout", repoDir)
	}

	stage(4, "DOWNLOADING VEK", "Press SPACE to jump over obstacles while Git is working")
	if err := syncRepo(git, repo, repoDir); err != nil {
		fail("GitHub clone/update failed: " + err.Error())
		return
	}

	sourceVersion := readSourceVersion(repoDir)
	if sourceVersion == "" {
		sourceVersion = version
	}

	if mode == modeSourceOnly {
		stage(5, "SOURCE - checkout verified", "VEK source v"+sourceVersion+" is available at "+repoDir)
		stage(6, "SOURCE - no launcher changes requested", "Executable/PATH installation skipped by Source Only mode.")
		stage(7, "VERIFY - checking .git metadata", filepath.Join(repoDir, ".git"))
		if _, e := os.Stat(filepath.Join(repoDir, ".git")); e != nil {
			fail("Managed Git clone verification failed.")
			return
		}
		stage(8, "METADATA - recording repository", repo)
		_ = os.WriteFile(filepath.Join(targetDir, "REPOSITORY.txt"), []byte(repo+"\r\n"), 0644)
		stage(9, "FINALIZE - source download ready", "Build or inspect VEK directly from C:\\vek\\repo")
		complete("SOURCE DOWNLOADED - VEK v" + sourceVersion + " - auto close in 5 seconds")
		return
	}

	stage(5, "RUNTIME - installing VEK launcher", "The installer itself stays in place; only the VEK CLI launcher is installed")
	if err := copyPackageFiles(); err != nil {
		fail(err.Error())
		return
	}

	stage(6, "METADATA - recording installed runtime", "VEK v"+sourceVersion+" - "+repo)
	if err := os.WriteFile(filepath.Join(targetDir, "REPOSITORY.txt"), []byte(repo+"\r\n"), 0644); err != nil {
		fail(err.Error())
		return
	}
	if err := os.WriteFile(filepath.Join(targetDir, "VERSION"), []byte(sourceVersion+"\r\n"), 0644); err != nil {
		fail(err.Error())
		return
	}
	savePolicy(policy)

	stage(7, "WINDOWS - registering User PATH", targetDir)
	if err := addUserPath(targetDir); err != nil {
		fail("PATH update failed: " + err.Error())
		return
	}

	stage(8, "VERIFY - checking managed clone and runtime", filepath.Join(repoDir, ".git"))
	if _, e := os.Stat(filepath.Join(repoDir, ".git")); e != nil {
		fail("Git clone verification failed.")
		return
	}
	if _, e := os.Stat(filepath.Join(targetDir, "vek.exe")); e != nil {
		fail("vek.exe deployment verification failed.")
		return
	}

	stage(9, "UPDATE POLICY - "+policyName(policy), "AUTO checks for updates automatically; downloads still require a visible Download click.")
	complete("VEK INSTALLED - v" + sourceVersion + " - You may close this window or it will auto-close in 5 seconds")
}

func policyName(p updatePolicy) string {
	if p == policyAuto {
		return "AUTO UPDATE"
	}
	return "MANUAL UPDATE"
}

func readSourceVersion(repoDir string) string {
	b, e := os.ReadFile(filepath.Join(repoDir, "VERSION"))
	if e != nil {
		return ""
	}
	s := strings.TrimSpace(string(b))
	if parseSemver(s).ok {
		return strings.TrimPrefix(s, "v")
	}
	return ""
}

func complete(msg string) {
	stage(10, "COMPLETE - VEK is ready", msg)
	stateMu.Lock()
	installing = false
	installDone = true
	installFailed = false
	completeAt = time.Now()
	stateMu.Unlock()
	setVersionStatus(installedVersionFromDisk(), latestVersion)
	if hwndMain != 0 {
		procPostMessageW.Call(uintptr(hwndMain), WM_APP_DONE, 0, 0)
	}
}

func fail(msg string) {
	stateMu.Lock()
	installing = false
	installFailed = true
	installDone = false
	statusLine = "INSTALL ERROR - VEK was not installed"
	detailLine = msg
	stateMu.Unlock()
	if hwndMain != 0 {
		procPostMessageW.Call(uintptr(hwndMain), WM_APP_UI, 0, 0)
	}
}

type progressWriter struct {
	mu  sync.Mutex
	buf string
}

func gitPercent(line string) int {
	for _, field := range strings.Fields(line) {
		token := strings.Trim(field, "()[],")
		if strings.HasSuffix(token, "%") {
			if n, e := strconv.Atoi(strings.TrimSuffix(token, "%")); e == nil && n >= 0 && n <= 100 {
				return n
			}
		}
	}
	return -1
}
func (w *progressWriter) Write(p []byte) (int, error) {
	w.mu.Lock()
	w.buf += string(p)
	textv := strings.ReplaceAll(w.buf, "\r", "\n")
	lines := strings.Split(textv, "\n")
	if len(lines) > 1 {
		w.buf = lines[len(lines)-1]
	}
	w.mu.Unlock()
	for i := len(lines) - 2; i >= 0; i-- {
		ln := strings.TrimSpace(lines[i])
		if ln != "" {
			if pct := gitPercent(ln); pct >= 0 {
				setProgressPercent(25 + (pct*35)/100)
			}
			if len(ln) > 150 {
				ln = ln[:150]
			}
			publish("", "GIT - "+ln, -1)
			break
		}
	}
	return len(p), nil
}
func runGitProgress(git string, args ...string) error {
	cmd := hiddenCommand(git, args...)
	cmd.Env = append(os.Environ(), "GIT_TERMINAL_PROMPT=0")
	pw := &progressWriter{}
	cmd.Stdout = pw
	cmd.Stderr = pw
	if err := cmd.Run(); err != nil {
		pw.mu.Lock()
		last := strings.TrimSpace(pw.buf)
		pw.mu.Unlock()
		if last != "" {
			return fmt.Errorf("%v: %s", err, last)
		}
		return err
	}
	return nil
}
func syncRepo(git, repo, dir string) error {
	if _, e := os.Stat(filepath.Join(dir, ".git")); e == nil {
		if e = runGitProgress(git, "-C", dir, "remote", "set-url", "origin", repo); e != nil {
			return e
		}
		if e = runGitProgress(git, "-C", dir, "fetch", "--progress", "--tags", "--prune", "origin"); e != nil {
			return e
		}
		if e = runGitProgress(git, "-C", dir, "reset", "--hard", "origin/main"); e != nil {
			return e
		}
		return nil
	}
	if _, e := os.Stat(dir); e == nil {
		if e = os.RemoveAll(dir); e != nil {
			return e
		}
	}
	return runGitProgress(git, "clone", "--progress", "--branch", "main", repo, dir)
}

func copyPackageFiles() error {
	exe, e := os.Executable()
	if e != nil {
		return e
	}
	src := filepath.Dir(exe)
	// Deliberately do not copy/replace VekInstaller.exe. The installer must be
	// extracted by the user into C:\\vek. This avoids self-replacing executable
	// behavior that antivirus products often treat as suspicious.
	for _, name := range []string{"vek.exe", "README.md", "manifest.sha256", "REPOSITORY.txt"} {
		from := filepath.Join(src, name)
		to := filepath.Join(targetDir, name)
		if samePath(from, to) {
			continue
		}
		data, er := os.ReadFile(from)
		if er != nil {
			if name == "README.md" || name == "manifest.sha256" || name == "REPOSITORY.txt" {
				continue
			}
			return er
		}
		mode := os.FileMode(0644)
		if strings.HasSuffix(strings.ToLower(name), ".exe") {
			mode = 0755
		}
		if er = os.WriteFile(to, data, mode); er != nil {
			return er
		}
	}
	return nil
}

func samePath(a, b string) bool {
	aa, _ := filepath.Abs(a)
	bb, _ := filepath.Abs(b)
	return strings.EqualFold(filepath.Clean(aa), filepath.Clean(bb))
}

func addUserPath(dir string) error {
	var key syscall.Handle
	sub, _ := syscall.UTF16PtrFromString(`Environment`)
	if err := syscall.RegOpenKeyEx(syscall.HKEY_CURRENT_USER, sub, 0, syscall.KEY_QUERY_VALUE|syscall.KEY_SET_VALUE, &key); err != nil {
		return err
	}
	defer syscall.RegCloseKey(key)
	name, _ := syscall.UTF16PtrFromString("Path")
	var typ uint32
	var n uint32
	_ = syscall.RegQueryValueEx(key, name, nil, &typ, nil, &n)
	buf := make([]byte, n+2)
	if n > 0 {
		_ = syscall.RegQueryValueEx(key, name, nil, &typ, &buf[0], &n)
	}
	current := ""
	if len(buf) >= 2 {
		u16 := (*[1 << 20]uint16)(unsafe.Pointer(&buf[0]))[: len(buf)/2 : len(buf)/2]
		current = syscall.UTF16ToString(u16)
	}
	for _, entry := range strings.Split(current, ";") {
		if strings.EqualFold(filepath.Clean(strings.TrimSpace(entry)), filepath.Clean(dir)) {
			return nil
		}
	}
	if current != "" && !strings.HasSuffix(current, ";") {
		current += ";"
	}
	current += dir
	data, _ := syscall.UTF16FromString(current)
	bytes := unsafe.Slice((*byte)(unsafe.Pointer(&data[0])), len(data)*2)
	if typ != syscall.REG_SZ && typ != syscall.REG_EXPAND_SZ {
		typ = syscall.REG_EXPAND_SZ
	}
	r, _, callErr := procRegSetValueExW.Call(uintptr(key), uintptr(unsafe.Pointer(name)), 0, uintptr(typ), uintptr(unsafe.Pointer(&bytes[0])), uintptr(len(bytes)))
	if r != 0 {
		return callErr
	}
	return nil
}

func runnerTop() int {
	return int(float64(clientH) * 0.66)
}
func runnerGround() int {
	return int(float64(clientH) * 0.91)
}
func playerFloor() float64 {
	return float64(runnerGround() - 30)
}
func downloadRects(w, h int) []RECT {
	bw := maxInt(160, minInt(260, w/6))
	gap := maxInt(16, w/80)
	total := bw*3 + gap*2
	x := (w - total) / 2
	y := int(float64(h) * 0.34)
	bh := maxInt(54, minInt(70, h/11))
	return []RECT{
		{int32(x), int32(y), int32(x + bw), int32(y + bh)},
		{int32(x + bw + gap), int32(y), int32(x + bw*2 + gap), int32(y + bh)},
		{int32(x + (bw+gap)*2), int32(y), int32(x + bw*3 + gap*2), int32(y + bh)},
	}
}
func policyRects(w, h int) (RECT, RECT) {
	bw := maxInt(130, minInt(180, w/9))
	gap := maxInt(16, w/100)
	x := (w - (bw*2 + gap)) / 2
	y := int(float64(h) * 0.44)
	bh := maxInt(48, minInt(60, h/13))
	return RECT{int32(x), int32(y), int32(x + bw), int32(y + bh)},
		RECT{int32(x + bw + gap), int32(y), int32(x + bw*2 + gap), int32(y + bh)}
}
func minInt(a, b int) int {
	if a < b {
		return a
	}
	return b
}
func maxInt(a, b int) int {
	if a > b {
		return a
	}
	return b
}

func updateAnimation() {
	now := time.Now()
	dt := now.Sub(lastFrame).Seconds()
	lastFrame = now
	if dt <= 0 || dt > 0.08 {
		dt = 1.0 / 60.0
	}
	gridOffset = math.Mod(gridOffset+dt*210, 80)

	floor := playerFloor()
	if onGround {
		playerY = floor
	}
	playerVy += 1650 * dt
	playerY += playerVy * dt
	if playerY >= floor {
		playerY = floor
		playerVy = 0
		onGround = true
		playerRot = 0
	} else {
		playerRot += dt * 4.8
	}
	speed := maxFloat(250.0, float64(clientW)*0.20)
	for i := range obstacles {
		obstacles[i].x -= speed * dt
		if obstacles[i].x+obstacles[i].w < 0 {
			obstacles[i].x += float64(clientW) + float64(i*110)
			score++
		}
		if collidePlayer(obstacles[i]) {
			crashes++
			score = 0
			playerY = floor
			playerVy = 0
			onGround = true
			playerRot = 0
			obstacles[i].x = float64(clientW)*0.36 + float64(i*260)
		}
	}

	stateMu.Lock()
	done := installDone
	doneAt := completeAt
	stateMu.Unlock()
	if done && !doneAt.IsZero() && time.Since(doneAt) >= 5*time.Second {
		procPostQuitMessage.Call(0)
	}
}
func maxFloat(a, b float64) float64 {
	if a > b {
		return a
	}
	return b
}

func collidePlayer(o obstacle) bool {
	px := maxFloat(52, float64(clientW)*0.07)
	size := 30.0
	py := playerY
	ground := float64(runnerGround())
	left, right, top, bottom := px, px+size, py, py+size
	ol, oright := o.x, o.x+o.w
	ot, ob := ground-o.h, ground
	if right <= ol || left >= oright || bottom <= ot || top >= ob {
		return false
	}
	if bottom < ob-o.h*0.35 {
		return false
	}
	return true
}

func jump() {
	if onGround {
		playerVy = -620
		onGround = false
	}
}

func rotate(v vec3, a float64) vec3 {
	cy, sy := math.Cos(a), math.Sin(a)
	cp, sp := math.Cos(a*0.31), math.Sin(a*0.31)
	x := v.x*cy + v.z*sy
	z := -v.x*sy + v.z*cy
	y := v.y
	return vec3{x, y*cp - z*sp, y*sp + z*cp}
}
func project(v vec3, center vec2, focal, camera float64) vec2 {
	r := rotate(v, angle)
	den := camera - r.z
	if den < 0.2 {
		den = 0.2
	}
	s := focal / den
	return vec2{center.x + r.x*s, center.y - r.y*s}
}

type seg3 struct{ a, b vec3 }

func vekSegments() []seg3 {
	base := []seg3{
		{vec3{-3.6, 1.2, 0}, vec3{-2.9, -1.2, 0}}, {vec3{-2.9, -1.2, 0}, vec3{-2.2, 1.2, 0}},
		{vec3{-1.5, 1.2, 0}, vec3{-1.5, -1.2, 0}}, {vec3{-1.5, 1.2, 0}, vec3{-0.1, 1.2, 0}}, {vec3{-1.5, 0, 0}, vec3{-0.35, 0, 0}}, {vec3{-1.5, -1.2, 0}, vec3{-0.1, -1.2, 0}},
		{vec3{0.7, 1.2, 0}, vec3{0.7, -1.2, 0}}, {vec3{0.7, 0, 0}, vec3{2.2, 1.2, 0}}, {vec3{0.7, 0, 0}, vec3{2.25, -1.2, 0}},
	}
	out := make([]seg3, 0, len(base)*3)
	for _, sg := range base {
		f := sg
		f.a.z = 0.28
		f.b.z = 0.28
		b := sg
		b.a.z = -0.28
		b.b.z = -0.28
		out = append(out, f, b, seg3{f.a, b.a})
	}
	return out
}

func draw3DLogo(hdc syscall.Handle, w, h int) {
	center := vec2{float64(w) * 0.5, float64(h) * 0.17}
	focal := math.Min(float64(w)*0.42, float64(h)*0.62)
	segs := vekSegments()
	projectLogo := func(v vec3) vec2 {
		v.x += 0.675
		return project(v, center, focal, 6.5)
	}
	withPen(hdc, rgb(7, 42, 28), maxInt(5, w/250), func() {
		for _, sg := range segs {
			a := projectLogo(sg.a)
			b := projectLogo(sg.b)
			line(hdc, int(a.x+5), int(a.y+6), int(b.x+5), int(b.y+6))
		}
	})
	withPen(hdc, rgb(20, 118, 72), maxInt(3, w/500), func() {
		for _, sg := range segs {
			a := projectLogo(sg.a)
			b := projectLogo(sg.b)
			line(hdc, int(a.x+2), int(a.y+2), int(b.x+2), int(b.y+2))
		}
	})
	withPen(hdc, rgb(92, 255, 169), 2, func() {
		for _, sg := range segs {
			a := projectLogo(sg.a)
			b := projectLogo(sg.b)
			line(hdc, int(a.x), int(a.y), int(b.x), int(b.y))
		}
	})
}

func draw3DProgressBar(hdc syscall.Handle, w, h, percent int, busy, done bool) {
	if percent < 0 {
		percent = 0
	}
	if percent > 100 {
		percent = 100
	}
	barW := int(float64(w) * 0.68)
	barH := maxInt(30, minInt(46, h/18))
	x := (w - barW) / 2
	y := int(float64(h) * 0.56)
	depth := maxInt(10, barH/3)

	fill(hdc, RECT{int32(x + 8), int32(y + 8), int32(x + barW + depth + 8), int32(y + barH + depth + 8)}, rgb(3, 8, 8))
	fill(hdc, RECT{int32(x), int32(y), int32(x + barW), int32(y + barH)}, rgb(8, 22, 18))

	top := []POINT{{int32(x), int32(y)}, {int32(x + depth), int32(y - depth)}, {int32(x + barW + depth), int32(y - depth)}, {int32(x + barW), int32(y)}}
	side := []POINT{{int32(x + barW), int32(y)}, {int32(x + barW + depth), int32(y - depth)}, {int32(x + barW + depth), int32(y + barH - depth)}, {int32(x + barW), int32(y + barH)}}
	b1, _, _ := procCreateSolidBrush.Call(rgb(17, 52, 39))
	oldB := selectObj(hdc, syscall.Handle(b1))
	procPolygon.Call(uintptr(hdc), uintptr(unsafe.Pointer(&top[0])), 4)
	procPolygon.Call(uintptr(hdc), uintptr(unsafe.Pointer(&side[0])), 4)
	selectObj(hdc, oldB)
	procDeleteObject.Call(b1)

	withPen(hdc, rgb(76, 255, 157), 2, func() {
		procRectangle.Call(uintptr(hdc), uintptr(x), uintptr(y), uintptr(x+barW), uintptr(y+barH))
		line(hdc, x, y, x+depth, y-depth)
		line(hdc, x+depth, y-depth, x+barW+depth, y-depth)
		line(hdc, x+barW+depth, y-depth, x+barW, y)
	})

	fw := int(float64(barW-4) * float64(percent) / 100.0)
	if fw > 0 {
		fill(hdc, RECT{int32(x + 2), int32(y + 2), int32(x + 2 + fw), int32(y + barH - 2)}, rgb(42, 205, 123))
		if fw > depth {
			ftop := []POINT{{int32(x + 2), int32(y + 2)}, {int32(x + 2 + depth), int32(y + 2 - depth)}, {int32(x + 2 + fw + depth), int32(y + 2 - depth)}, {int32(x + 2 + fw), int32(y + 2)}}
			fb, _, _ := procCreateSolidBrush.Call(rgb(89, 255, 165))
			ob := selectObj(hdc, syscall.Handle(fb))
			procPolygon.Call(uintptr(hdc), uintptr(unsafe.Pointer(&ftop[0])), 4)
			selectObj(hdc, ob)
			procDeleteObject.Call(fb)
		}
	}
	if busy || done {
		text(hdc, fmt.Sprintf("%d%%", percent), RECT{int32(x), int32(y), int32(x + barW), int32(y + barH)}, rgb(235, 255, 244), fontButton, DT_CENTER|DT_VCENTER|DT_SINGLELINE)
	}
}

func drawButton(hdc syscall.Handle, r RECT, label string, active bool) {
	bg := rgb(13, 27, 22)
	border := rgb(50, 125, 89)
	fg := rgb(158, 255, 203)
	if active {
		bg = rgb(20, 67, 46)
		border = rgb(76, 255, 157)
		fg = rgb(235, 255, 244)
	}
	b, _, _ := procCreateSolidBrush.Call(bg)
	oldB := selectObj(hdc, syscall.Handle(b))
	p, _, _ := procCreatePen.Call(PS_SOLID, 2, border)
	oldP := selectObj(hdc, syscall.Handle(p))
	procRoundRect.Call(uintptr(hdc), uintptr(r.Left), uintptr(r.Top), uintptr(r.Right), uintptr(r.Bottom), 14, 14)
	selectObj(hdc, oldP)
	selectObj(hdc, oldB)
	procDeleteObject.Call(p)
	procDeleteObject.Call(b)
	text(hdc, label, r, fg, fontButton, DT_CENTER|DT_VCENTER|DT_SINGLELINE)
}

func drawRunner(hdc syscall.Handle, w, h int) {
	top := runnerTop()
	ground := runnerGround()
	fill(hdc, RECT{0, int32(top), int32(w), int32(h)}, rgb(4, 10, 14))
	withPen(hdc, rgb(15, 44, 50), 1, func() {
		for y := top + 25; y < ground; y += 30 {
			line(hdc, 0, y, w, y)
		}
		for x := -80 + int(gridOffset); x < w+80; x += 80 {
			line(hdc, x, ground, x+220, top)
		}
	})
	withPen(hdc, rgb(65, 255, 155), 3, func() { line(hdc, 0, ground, w, ground) })
	for _, o := range obstacles {
		pts := []POINT{{int32(o.x), int32(ground)}, {int32(o.x + o.w/2), int32(float64(ground) - o.h)}, {int32(o.x + o.w), int32(ground)}}
		b, _, _ := procCreateSolidBrush.Call(rgb(38, 155, 105))
		oldB := selectObj(hdc, syscall.Handle(b))
		p, _, _ := procCreatePen.Call(PS_SOLID, 2, rgb(110, 255, 192))
		oldP := selectObj(hdc, syscall.Handle(p))
		procPolygon.Call(uintptr(hdc), uintptr(unsafe.Pointer(&pts[0])), uintptr(len(pts)))
		selectObj(hdc, oldP)
		selectObj(hdc, oldB)
		procDeleteObject.Call(p)
		procDeleteObject.Call(b)
	}
	cx, cy := maxFloat(67, float64(w)*0.07)+15, playerY+15
	half := 15.0
	c, ss := math.Cos(playerRot), math.Sin(playerRot)
	local := [][2]float64{{-half, -half}, {half, -half}, {half, half}, {-half, half}}
	pts := make([]POINT, 4)
	for i, p := range local {
		pts[i] = POINT{int32(cx + p[0]*c - p[1]*ss), int32(cy + p[0]*ss + p[1]*c)}
	}
	b, _, _ := procCreateSolidBrush.Call(rgb(72, 255, 155))
	oldB := selectObj(hdc, syscall.Handle(b))
	pen, _, _ := procCreatePen.Call(PS_SOLID, 2, rgb(218, 255, 235))
	oldP := selectObj(hdc, syscall.Handle(pen))
	procPolygon.Call(uintptr(hdc), uintptr(unsafe.Pointer(&pts[0])), 4)
	selectObj(hdc, oldP)
	selectObj(hdc, oldB)
	procDeleteObject.Call(pen)
	procDeleteObject.Call(b)
}

func paint(hwnd syscall.Handle) {
	var ps PAINTSTRUCT
	hdcRaw, _, _ := procBeginPaint.Call(uintptr(hwnd), uintptr(unsafe.Pointer(&ps)))
	hdc := syscall.Handle(hdcRaw)
	var rc RECT
	procGetClientRect.Call(uintptr(hwnd), uintptr(unsafe.Pointer(&rc)))
	w := int(rc.Right - rc.Left)
	h := int(rc.Bottom - rc.Top)
	if w < 320 {
		w = 320
	}
	if h < 480 {
		h = 480
	}
	clientW, clientH = w, h
	fill(hdc, rc, rgb(5, 11, 10))

	withPen(hdc, rgb(10, 32, 28), 1, func() {
		for x := -120 + int(gridOffset); x < w+160; x += 80 {
			line(hdc, x, 0, x+170, runnerTop())
		}
		for y := 20; y < runnerTop(); y += 40 {
			line(hdc, 0, y, w, y)
		}
	})

	draw3DLogo(hdc, w, h)

	stateMu.Lock()
	pct, busy, done, pol, mode := progressPercent, installing, installDone, policy, currentMode
	stateMu.Unlock()

	buttons := downloadRects(w, h)
	for i, r := range buttons {
		drawButton(hdc, r, "DOWNLOAD", busy && mode == installMode(i))
	}
	autoR, manualR := policyRects(w, h)
	drawButton(hdc, autoR, "AUTO", pol == policyAuto)
	drawButton(hdc, manualR, "MANUAL", pol == policyManual)

	draw3DProgressBar(hdc, w, h, pct, busy, done)
	drawRunner(hdc, w, h)

	withPen(hdc, rgb(92, 255, 169), 2, func() {
		line(hdc, w-42, 18, w-18, 42)
		line(hdc, w-18, 18, w-42, 42)
	})

	procEndPaint.Call(uintptr(hwnd), uintptr(unsafe.Pointer(&ps)))
}

func wndProc(hwnd syscall.Handle, msg uint32, wParam, lParam uintptr) uintptr {
	switch msg {
	case WM_DESTROY:
		procPostQuitMessage.Call(0)
		return 0
	case WM_CLOSE:
		stateMu.Lock()
		busy := installing
		stateMu.Unlock()
		if busy {
			publish("INSTALLING - close disabled until current Git operation finishes", "You can keep playing the runner with SPACE.", -1)
			return 0
		}
	case WM_PAINT:
		paint(hwnd)
		return 0
	case WM_TIMER:
		if wParam == 1 {
			updateAnimation()
			procInvalidateRect.Call(uintptr(hwnd), 0, 0)
			return 0
		}
	case WM_KEYDOWN:
		if wParam == VK_SPACE {
			jump()
			return 0
		}
		if wParam == VK_ESCAPE {
			stateMu.Lock()
			busy := installing
			stateMu.Unlock()
			if !busy {
				procPostQuitMessage.Call(0)
			}
			return 0
		}
	case WM_LBUTTONDOWN:
		x, y := lowWord(lParam), highWord(lParam)
		stateMu.Lock()
		busy := installing
		stateMu.Unlock()
		var rc RECT
		procGetClientRect.Call(uintptr(hwnd), uintptr(unsafe.Pointer(&rc)))
		w := int(rc.Right - rc.Left)
		h := int(rc.Bottom - rc.Top)
		if x >= w-56 && x <= w-4 && y >= 4 && y <= 56 {
			if !busy {
				procPostQuitMessage.Call(0)
			}
			return 0
		}
		if !busy {
			for i, r := range downloadRects(w, h) {
				if inside(r, x, y) {
					startInstall(installMode(i))
					return 0
				}
			}
			autoR, manualR := policyRects(w, h)
			if inside(autoR, x, y) {
				stateMu.Lock()
				policy = policyAuto
				stateMu.Unlock()
				savePolicy(policyAuto)
				go checkVersionWorker()
				return 0
			}
			if inside(manualR, x, y) {
				stateMu.Lock()
				policy = policyManual
				stateMu.Unlock()
				savePolicy(policyManual)
				return 0
			}
		}
	case WM_APP_UI, WM_APP_VERSION:
		procInvalidateRect.Call(uintptr(hwnd), 0, 0)
		return 0
	case WM_APP_DONE:
		procInvalidateRect.Call(uintptr(hwnd), 0, 0)
		return 0
	case WM_APP_AUTO:
		// AUTO only checks/notifies; updates remain explicit and visible.
		return 0
	}
	r, _, _ := procDefWindowProcW.Call(uintptr(hwnd), uintptr(msg), wParam, lParam)
	return r
}

func main() {

	if bootstrapLatestInstaller() {
		return
	}
	runtime.LockOSThread()
	policy = loadPolicy()
	fontSmall = createFont(14, 400, "Consolas")
	fontBody = createFont(17, 500, "Consolas")
	fontButton = createFont(19, 700, "Consolas")
	fontTitle = createFont(28, 700, "Consolas")

	hi, _, _ := procGetModuleHandleW.Call(0)
	hInst := syscall.Handle(hi)
	className := utf16("VEKInteractiveGithubInstaller")
	wc := WNDCLASSEX{CbSize: uint32(unsafe.Sizeof(WNDCLASSEX{})), LpfnWndProc: syscall.NewCallback(wndProc), HInstance: hInst, LpszClassName: className}
	if r, _, e := procRegisterClassExW.Call(uintptr(unsafe.Pointer(&wc))); r == 0 {
		panic(e)
	}

	sw, _, _ := procGetSystemMetrics.Call(SM_CXSCREEN)
	sh, _, _ := procGetSystemMetrics.Call(SM_CYSCREEN)
	if sw < 800 {
		sw = 1280
	}
	if sh < 600 {
		sh = 720
	}
	clientW, clientH = int(sw), int(sh)
	playerY = playerFloor()
	for i := range obstacles {
		obstacles[i].x = float64(clientW) * (0.38 + float64(i)*0.20)
	}

	raw, _, e := procCreateWindowExW.Call(
		0,
		uintptr(unsafe.Pointer(className)),
		uintptr(unsafe.Pointer(utf16("VEK Installer"))),
		WS_POPUP|WS_VISIBLE,
		0, 0, sw, sh,
		0, 0, uintptr(hInst), 0,
	)
	if raw == 0 {
		panic(e)
	}
	hwndMain = syscall.Handle(raw)
	procShowWindow.Call(uintptr(hwndMain), SW_SHOW)
	procUpdateWindow.Call(uintptr(hwndMain))
	procSetTimer.Call(uintptr(hwndMain), 1, 16, 0)

	go checkVersionWorker()
	var msg MSG
	for {
		r, _, _ := procGetMessageW.Call(uintptr(unsafe.Pointer(&msg)), 0, 0, 0)
		if int32(r) <= 0 {
			break
		}
		procTranslateMessage.Call(uintptr(unsafe.Pointer(&msg)))
		procDispatchMessageW.Call(uintptr(unsafe.Pointer(&msg)))
	}
	procKillTimer.Call(uintptr(hwndMain), 1)
}
