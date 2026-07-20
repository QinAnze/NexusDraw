# NexusDraw Windows Setup
Add-Type -AssemblyName System.Windows.Forms,System.Drawing,System.IO.Compression.FileSystem

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Find archive (.7z preferred, .zip fallback)
$ArchiveFile = $null; $Is7z = $false
foreach ($ext in @(".7z", ".zip")) {
    $f = Join-Path $ScriptDir "NexusDraw-setup$ext"
    if (Test-Path $f) { $ArchiveFile = $f; $Is7z = ($ext -eq ".7z"); break }
}
if (-not $ArchiveFile) {
    [System.Windows.Forms.MessageBox]::Show("NexusDraw-setup.7z or .zip not found.", "Error", "OK", "Error")
    exit 1
}

# --- GUI ---
$Form = New-Object System.Windows.Forms.Form
$Form.Text = "NexusDraw Setup"
$Form.Size = New-Object System.Drawing.Size(460, 280)
$Form.StartPosition = "CenterScreen"
$Form.FormBorderStyle = "FixedDialog"
$Form.MaximizeBox = $false

$Label1 = New-Object System.Windows.Forms.Label
$Label1.Text = "Install to:"
$Label1.Location = New-Object System.Drawing.Point(20, 20)
$Label1.Size = New-Object System.Drawing.Size(60, 25)
$Form.Controls.Add($Label1)

$PathBox = New-Object System.Windows.Forms.TextBox
$PathBox.Text = "$env:LOCALAPPDATA\NexusDraw"
$PathBox.Location = New-Object System.Drawing.Point(80, 20)
$PathBox.Size = New-Object System.Drawing.Size(280, 25)
$Form.Controls.Add($PathBox)

$BrowseBtn = New-Object System.Windows.Forms.Button
$BrowseBtn.Text = "Browse..."
$BrowseBtn.Location = New-Object System.Drawing.Point(370, 18)
$BrowseBtn.Size = New-Object System.Drawing.Size(70, 28)
$BrowseBtn.Add_Click({
    $fd = New-Object System.Windows.Forms.FolderBrowserDialog
    if ($fd.ShowDialog() -eq "OK") { $PathBox.Text = $fd.SelectedPath }
})
$Form.Controls.Add($BrowseBtn)

$DesktopCheck = New-Object System.Windows.Forms.CheckBox
$DesktopCheck.Text = "Create Desktop shortcut"
$DesktopCheck.Location = New-Object System.Drawing.Point(80, 65)
$DesktopCheck.Size = New-Object System.Drawing.Size(200, 25)
$DesktopCheck.Checked = $true
$Form.Controls.Add($DesktopCheck)

$MenuCheck = New-Object System.Windows.Forms.CheckBox
$MenuCheck.Text = "Create Start Menu shortcut"
$MenuCheck.Location = New-Object System.Drawing.Point(80, 95)
$MenuCheck.Size = New-Object System.Drawing.Size(200, 25)
$MenuCheck.Checked = $true
$Form.Controls.Add($MenuCheck)

$Progress = New-Object System.Windows.Forms.ProgressBar
$Progress.Location = New-Object System.Drawing.Point(20, 145)
$Progress.Size = New-Object System.Drawing.Size(420, 25)
$Progress.Style = "Marquee"
$Progress.Visible = $false
$Form.Controls.Add($Progress)

$StatusLabel = New-Object System.Windows.Forms.Label
$StatusLabel.Text = ""
$StatusLabel.Location = New-Object System.Drawing.Point(20, 180)
$StatusLabel.Size = New-Object System.Drawing.Size(420, 25)
$Form.Controls.Add($StatusLabel)

$InstallBtn = New-Object System.Windows.Forms.Button
$InstallBtn.Text = "Install"
$InstallBtn.Location = New-Object System.Drawing.Point(300, 210)
$InstallBtn.Size = New-Object System.Drawing.Size(140, 30)
$InstallBtn.Add_Click({
    $InstallBtn.Enabled = $false; $Progress.Visible = $true
    $StatusLabel.Text = "Extracting..."; $Form.Refresh()
    $TargetDir = $PathBox.Text
    try {
        if (Test-Path $TargetDir) { Remove-Item -Recurse -Force $TargetDir -ErrorAction Stop }
        New-Item -ItemType Directory -Force -Path $TargetDir | Out-Null

        # Extract using 7-Zip if available (for .7z), otherwise use built-in (for .zip)
        if ($Is7z) {
            $sevenZip = @("$ScriptDir\7za.exe", "C:\Program Files\7-Zip\7z.exe", "C:\Program Files (x86)\7-Zip\7z.exe")
            $found = $null
            foreach ($p in $sevenZip) { if (Test-Path $p) { $found = $p; break } }
            if (-not $found) {
                throw "7-Zip not found. Please install 7-Zip or use the .zip version."
            }
            $StatusLabel.Text = "Extracting with 7-Zip..."
            & $found x $ArchiveFile -o"$TargetDir" -y | Out-Null
        } else {
            [System.IO.Compression.ZipFile]::ExtractToDirectory($ArchiveFile, $TargetDir)
        }

        $ExePath = Get-ChildItem -Path $TargetDir -Recurse -Name "NexusDraw.exe" -Depth 3 | Select-Object -First 1
        if (-not $ExePath) { throw "NexusDraw.exe not found in archive" }
        $ExePath = Join-Path $TargetDir $ExePath
        $WorkDir = Split-Path -Parent $ExePath

        if ($DesktopCheck.Checked) {
            $s = (New-Object -COM WScript.Shell).CreateShortcut("$([Environment]::GetFolderPath('Desktop'))\NexusDraw.lnk")
            $s.TargetPath = $ExePath; $s.WorkingDirectory = $WorkDir; $s.IconLocation = "$ExePath,0"; $s.Save()
        }
        if ($MenuCheck.Checked) {
            $md = "$([Environment]::GetFolderPath('Programs'))\NexusDraw"
            New-Item -ItemType Directory -Force -Path $md | Out-Null
            $s = (New-Object -COM WScript.Shell).CreateShortcut("$md\NexusDraw.lnk")
            $s.TargetPath = $ExePath; $s.WorkingDirectory = $WorkDir; $s.IconLocation = "$ExePath,0"; $s.Save()
        }
        try {
            $reg = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\NexusDraw"
            New-Item -Path $reg -Force | Out-Null
            Set-ItemProperty -Path $reg -Name "DisplayName" -Value "NexusDraw"
            Set-ItemProperty -Path $reg -Name "UninstallString" -Value "powershell -Command `"Remove-Item -Recurse -Force '$TargetDir'`""
            Set-ItemProperty -Path $reg -Name "DisplayIcon" -Value $ExePath
            Set-ItemProperty -Path $reg -Name "Publisher" -Value "NexusDraw"
            Set-ItemProperty -Path $reg -Name "NoModify" -Value 1; Set-ItemProperty -Path $reg -Name "NoRepair" -Value 1
        } catch {}
        $Progress.Visible = $false; $StatusLabel.Text = "Installation complete!"
        [System.Windows.Forms.MessageBox]::Show("NexusDraw installed to:`n$TargetDir", "Complete", "OK", "Information")
        $Form.Close()
    } catch {
        $Progress.Visible = $false; $StatusLabel.Text = "Error: $_"
        [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, "Error", "OK", "Error")
        $InstallBtn.Enabled = $true
    }
})
$Form.Controls.Add($InstallBtn)
$Form.ShowDialog() | Out-Null
