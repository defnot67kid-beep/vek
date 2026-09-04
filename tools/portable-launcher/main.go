package main

import (
	"crypto/sha256"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"syscall"
	"time"
)

const defaultRepo = "https://github.com/defnot67kid-beep/vek.git"

type semver struct {
	a, b, c int
	ok      bool
}

type releaseVersionMetadata struct {
	Version string `json:"version"`
	Tag     string `json:"tag"`
}

func main() {
	args := os.Args[1:]
	if len(args) == 0 {
		maybeAutoUpdate()
		launchInstaller()
		return
	}
	switch args[0] {
	case "--version", "version", "-v":
		maybeAutoUpdate()
		fmt.Println("VEK", buildVersion)
	case "bootstrap-version":
		fmt.Println(buildVersion)
	case "--help", "help", "-h":
		printHelp()
	case "--install", "install":
		launchInstaller()
	case "update", "--update":
		launchInstaller()
	case "check-update":
		checkUpdate(true)
	case "home":
		fmt.Println(root())
	case "doctor":
		doctor()
	case "verify":
		verify()
	default:
		fmt.Fprintf(os.Stderr, "Unknown VEK command: %s\n\n", args[0])
		printHelp()
		os.Exit(1)
	}
}
func root() string {
	p, e := os.Executable()
	if e != nil {
		return `C:\vek`
	}
	p, _ = filepath.Abs(p)
	return filepath.Dir(p)
}
func metadataVersion() string {
	if b, e := os.ReadFile(filepath.Join(root(), "VERSION")); e == nil {
		v := strings.TrimSpace(string(b))
		if parse(v).ok {
			return strings.TrimPrefix(v, "v")
		}
	}
	return "none"
}
func installedVersion() string {
	// This executable is the bootstrap launcher. Its --version must report the
	// version compiled into the binary, never blindly trust VERSION metadata.
	// A deployed full VEK runtime replaces this launcher at C:\vek\vek.exe.
	return buildVersion
}
func repo() string {
	if b, e := os.ReadFile(filepath.Join(root(), "REPOSITORY.txt")); e == nil {
		v := strings.TrimSpace(string(b))
		if v != "" {
			return v
		}
	}
	return defaultRepo
}
func policy() string {
	if b, e := os.ReadFile(filepath.Join(root(), "UPDATE_POLICY")); e == nil && strings.EqualFold(strings.TrimSpace(string(b)), "auto") {
		return "auto"
	}
	return "manual"
}
func launchInstaller() {
	p := filepath.Join(root(), "VekInstaller.exe")
	if _, err := os.Stat(p); err != nil {
		fallback := `C:\vek\VekInstaller.exe`
		if _, ferr := os.Stat(fallback); ferr == nil {
			p = fallback
		}
	}
	cmd := exec.Command(p)
	cmd.Dir = filepath.Dir(p)
	if e := cmd.Start(); e != nil {
		fmt.Fprintln(os.Stderr, "Could not start VEK installer:", e)
		os.Exit(3)
	}
}

func parse(s string) semver {
	s = strings.TrimSpace(strings.TrimPrefix(s, "v"))
	if i := strings.IndexAny(s, "-+"); i >= 0 {
		s = s[:i]
	}
	p := strings.Split(s, ".")
	if len(p) < 3 {
		return semver{}
	}
	a, e1 := strconv.Atoi(p[0])
	b, e2 := strconv.Atoi(p[1])
	c, e3 := strconv.Atoi(p[2])
	return semver{a, b, c, e1 == nil && e2 == nil && e3 == nil}
}
func cmp(x, y semver) int {
	if !x.ok || !y.ok {
		return 0
	}
	if x.a != y.a {
		if x.a < y.a {
			return -1
		}
		return 1
	}
	if x.b != y.b {
		if x.b < y.b {
			return -1
		}
		return 1
	}
	if x.c != y.c {
		if x.c < y.c {
			return -1
		}
		return 1
	}
	return 0
}
func gitPath() (string, error) {
	if g, e := exec.LookPath("git.exe"); e == nil {
		return g, nil
	}
	return exec.LookPath("git")
}

func hiddenCommand(exe string, args ...string) *exec.Cmd {
	cmd := exec.Command(exe, args...)
	cmd.SysProcAttr = &syscall.SysProcAttr{HideWindow: true, CreationFlags: 0x08000000}
	return cmd
}
func repoWebBase(repo string) string {
	s := strings.TrimSpace(repo)
	s = strings.TrimSuffix(s, ".git")
	return strings.TrimRight(s, "/")
}

func latest() (string, error) {
	url := repoWebBase(repo()) + "/releases/latest/download/release-version.json"
	client := &http.Client{Timeout: 20 * time.Second}
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		return "", err
	}
	req.Header.Set("User-Agent", "VEK-Bootstrap/"+buildVersion)
	resp, err := client.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return "", fmt.Errorf("HTTP %d resolving latest published VEK release", resp.StatusCode)
	}
	b, err := io.ReadAll(io.LimitReader(resp.Body, 256*1024))
	if err != nil {
		return "", err
	}
	var meta releaseVersionMetadata
	if err := json.Unmarshal(b, &meta); err != nil {
		return "", err
	}
	v := strings.TrimPrefix(strings.TrimSpace(meta.Version), "v")
	if !parse(v).ok {
		return "", fmt.Errorf("latest release metadata has invalid version %q", meta.Version)
	}
	if meta.Tag != "" && meta.Tag != "v"+v {
		return "", fmt.Errorf("release tag/version mismatch")
	}
	return v, nil
}

func checkUpdate(printResult bool) bool {
	l, e := latest()
	if e != nil {
		if printResult {
			fmt.Println("VEK update status: unknown -", e)
		}
		return false
	}
	cur := installedVersion()
	out := cmp(parse(cur), parse(l)) < 0
	if printResult {
		if out {
			fmt.Printf("Your VEK version is OUTDATED: installed v%s, latest v%s\n", cur, l)
		} else {
			fmt.Printf("Your VEK version is UP TO DATE: v%s\n", cur)
		}
	}
	return out
}
func maybeAutoUpdate() {
	// Runtime AUTO mode performs an update check only. Installer bootstrap updates
	// are handled by VekInstaller.exe itself using verified GitHub release assets.
	if policy() != "auto" {
		return
	}
	if checkUpdate(false) {
		fmt.Fprintln(os.Stderr, "VEK update available. Run: vek --install")
	}
}

func verify() {
	manifest := filepath.Join(root(), "manifest.sha256")
	data, e := os.ReadFile(manifest)
	if e != nil {
		fmt.Fprintln(os.Stderr, "VEK verify: manifest.sha256 not found")
		os.Exit(2)
	}
	fail := 0
	checked := 0
	for _, ln := range strings.Split(string(data), "\n") {
		f := strings.Fields(strings.TrimSpace(ln))
		if len(f) < 2 {
			continue
		}
		expected := strings.ToLower(f[0])
		name := f[1]
		if strings.Contains(name, "..") || filepath.IsAbs(name) {
			fmt.Println("[FAIL] unsafe manifest path", name)
			fail++
			continue
		}
		b, e := os.ReadFile(filepath.Join(root(), filepath.FromSlash(name)))
		if e != nil {
			fmt.Println("[FAIL]", name)
			fail++
			continue
		}
		got := fmt.Sprintf("%x", sha256.Sum256(b))
		checked++
		if got == expected {
			fmt.Println("[OK]  ", name)
		} else {
			fmt.Println("[FAIL]", name)
			fail++
		}
	}
	fmt.Printf("Checked %d files; failures: %d\n", checked, fail)
	if fail > 0 {
		os.Exit(3)
	}
}

func doctor() {
	fmt.Println("VEK Doctor\n----------")
	fail := 0
	for _, f := range []string{"VekInstaller.exe", "REPOSITORY.txt", "VERSION"} {
		if _, e := os.Stat(filepath.Join(root(), f)); e == nil {
			fmt.Println("[OK]  ", f)
		} else {
			fmt.Println("[FAIL]", f)
			fail++
		}
	}
	if g, e := gitPath(); e == nil {
		fmt.Println("[OK]   Git:", g)
	} else {
		fmt.Println("[FAIL] Git not found")
		fail++
	}
	meta := metadataVersion()
	fmt.Println("[INFO] Bootstrap binary:", buildVersion)
	fmt.Println("[INFO] VERSION metadata:", meta)
	if meta != "none" && meta != buildVersion {
		fmt.Println("[WARN] VERSION metadata differs from this bootstrap binary. Run VekInstaller.exe to repair/update.")
	}
	fmt.Println("[INFO] Update policy:", policy())
	if fail > 0 {
		os.Exit(4)
	}
}
func printHelp() {
	fmt.Printf(`VEK Bootstrap %s

Usage:
  vek --version             Show installed VEK version
  vek --install             Open the graphical installer
  vek update                Open installer for update/repair
  vek check-update          Compare installed VEK with GitHub releases
  vek home                  Show VEK installation folder
  vek doctor                Check local installation
  vek verify                Verify packaged installer files
  vek --help                Show this help

Automatic or manual updating is selected inside the graphical installer.

`, installedVersion())
}
