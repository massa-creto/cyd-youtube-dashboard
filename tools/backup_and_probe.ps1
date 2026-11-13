param([string]$Port = 'COM5', [string]$Python = 'py', [string]$Version = '-3.13')
$ErrorActionPreference = 'Stop'
if ($Port -notmatch '^COM\d{1,3}$') { throw 'Invalid COM port.' }
$root = Split-Path -Parent $PSScriptRoot
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$out = Join-Path $env:USERPROFILE "CYD_Private_Backups\$stamp"
Write-Host "CYD read-only backup. Port: $Port"
Write-Warning 'The backup may contain Wi-Fi passwords, API keys, and other secrets. Never upload it.'
Write-Warning 'The board may reboot. This script will NOT erase or write flash.'
$answer = Read-Host 'Type BACKUP to proceed'
if ($answer -cne 'BACKUP') { Write-Host 'Cancelled.'; exit 0 }
& $Python $Version -m esptool version
if ($LASTEXITCODE -ne 0) { throw 'esptool is unavailable. Install: py -3.13 -m pip install esptool' }
& $Python $Version (Join-Path $PSScriptRoot 'backup_flash_windows.py') --port $Port --out $out --confirm-read --expected-size-mb 4
if ($LASTEXITCODE -ne 0) { throw 'Backup failed. Do not flash the board.' }
Write-Host "Backup saved privately to: $out"
Write-Host 'Keep the .bin and JSON metadata together. Do not upload them.'
