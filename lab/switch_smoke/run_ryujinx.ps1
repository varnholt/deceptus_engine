# Launches an .nro in Ryujinx, waits, captures the emulator window, then kills it.

param(
    [Parameter(Mandatory=$true)][string]$NroPath,
    [Parameter(Mandatory=$true)][string]$OutputPath,
    [int]$SettleSeconds = 25
)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

$ryujinx = "D:\games\ryujinx-1.3.2-win_x64\publish\Ryujinx.exe"
$stdout = "$env:TEMP\ryujinx_stdout.txt"
$stderr = "$env:TEMP\ryujinx_stderr.txt"

Write-Output "launching: $ryujinx $NroPath"
$process = Start-Process -FilePath $ryujinx -ArgumentList "`"$NroPath`"" -PassThru `
    -RedirectStandardOutput $stdout -RedirectStandardError $stderr

Start-Sleep -Seconds $SettleSeconds

if ($process.HasExited) {
    Write-Output "RYUJINX EXITED EARLY (code $($process.ExitCode))"
} else {
    $process.Refresh()
    Write-Output "window title: '$($process.MainWindowTitle)'"
    $handle = $process.MainWindowHandle

    if ($handle -ne 0) {
        [void][Win]::SetForegroundWindow($handle)
        Start-Sleep -Seconds 2
        $rect = New-Object Win+RECT
        [void][Win]::GetWindowRect($handle, [ref]$rect)
        $width = $rect.Right - $rect.Left
        $height = $rect.Bottom - $rect.Top
        Write-Output "capturing ${width}x${height}"
        $bitmap = New-Object System.Drawing.Bitmap $width, $height
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
        $graphics.Dispose(); $bitmap.Dispose()
        Write-Output "saved $OutputPath"
    } else {
        Write-Output "no main window handle"
    }
}

Write-Output "--- stdout (last 40 lines) ---"
if (Test-Path $stdout) { Get-Content $stdout -Tail 40 }
Write-Output "--- stderr (last 20 lines) ---"
if (Test-Path $stderr) { Get-Content $stderr -Tail 20 }

Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
Get-Process -Name "Ryujinx" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Write-Output "stopped"
