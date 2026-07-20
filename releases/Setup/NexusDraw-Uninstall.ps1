# NexusDraw Uninstaller
$AppName = "NexusDraw"
$Desktop = [Environment]::GetFolderPath("Desktop")
$Programs = [Environment]::GetFolderPath("Programs")
$MenuDir = "$Programs\$AppName"

Add-Type -AssemblyName System.Windows.Forms
$result = [System.Windows.Forms.MessageBox]::Show(
    "确定要卸载 $AppName 吗？`n这将删除所有文件和快捷方式。",
    "$AppName 卸载", "YesNo", "Question")

if ($result -ne "Yes") { exit }

# Remove shortcuts
Remove-Item "$Desktop\$AppName.lnk" -Force -ErrorAction SilentlyContinue
Remove-Item "$MenuDir" -Recurse -Force -ErrorAction SilentlyContinue

# Find installed location from registry or default path
$InstallDir = $null
try {
    $InstallDir = (Get-ItemProperty "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\$AppName").InstallLocation
} catch {}

if ($InstallDir -and (Test-Path $InstallDir)) {
    Remove-Item $InstallDir -Recurse -Force -ErrorAction Stop
    [System.Windows.Forms.MessageBox]::Show("$AppName 已成功卸载。", "完成", "OK", "Information")
} elseif (Test-Path "$env:LOCALAPPDATA\$AppName") {
    Remove-Item "$env:LOCALAPPDATA\$AppName" -Recurse -Force -ErrorAction Stop
    [System.Windows.Forms.MessageBox]::Show("$AppName 已成功卸载。", "完成", "OK", "Information")
} else {
    [System.Windows.Forms.MessageBox]::Show("未找到安装目录。快捷方式已清除。", "完成", "OK", "Information")
}

# Remove registry
Remove-Item "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\$AppName" -Force -ErrorAction SilentlyContinue
