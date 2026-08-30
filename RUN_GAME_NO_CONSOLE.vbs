Option Explicit
Dim shell, fso, root, exePath
Set shell = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")
root = fso.GetParentFolderName(WScript.ScriptFullName)
exePath = root & "\build\Release\CustomVehicleGame.exe"

If fso.FileExists(exePath) Then
    shell.CurrentDirectory = root
    shell.Run Chr(34) & exePath & Chr(34), 0, False
Else
    MsgBox "Build the game first by double-clicking BUILD_WINDOWS.bat", 48, "Custom Vehicle Game"
End If
