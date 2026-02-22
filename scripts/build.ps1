param(
  [string]$Bloom = ".\examples\tile_stats.bloom",
  [string]$OutCpp = ".\out.cpp",
  [string]$Exe = ".\out.exe"
)

$ErrorActionPreference = "Stop"

# Ensure we are at repo root
Set-Location (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location ..

# Stabilize MSYS2 toolchain for this session
$msysUcrt = "D:\MSYS2\ucrt64\bin"
$msysUsr  = "D:\MSYS2\usr\bin"
$env:Path = "$msysUcrt;$msysUsr;$env:Path"

Write-Host "[1/4] Generate C++ from Bloom" -ForegroundColor Cyan
python .\bloomc.py $Bloom $OutCpp
if ($LASTEXITCODE -ne 0) { throw "bloomc.py failed (EXITCODE=$LASTEXITCODE)" }

Write-Host "[2/4] Compile" -ForegroundColor Cyan
Remove-Item $Exe -ErrorAction SilentlyContinue
g++ -std=c++17 -O2 -I. -I.\runtime $OutCpp -o $Exe 2>&1 | Tee-Object .\build.log | Out-Null
if ($LASTEXITCODE -ne 0) {
  Write-Host "Build failed. Showing build.log:" -ForegroundColor Red
  Get-Content .\build.log -Raw
  throw "g++ failed (EXITCODE=$LASTEXITCODE)"
}

Write-Host "[3/4] Run" -ForegroundColor Cyan
& $Exe
$runCode = $LASTEXITCODE
Write-Host "RUN_EXITCODE=$runCode"

Write-Host "[4/4] Artifacts" -ForegroundColor Cyan
if (Test-Path ".\out.json") {
  $info = Get-Item ".\out.json"
  Write-Host ("out.json: {0} bytes, {1}" -f $info.Length, $info.LastWriteTime)
} else {
  Write-Host "out.json not found (if you added JSON export, it should appear here)." -ForegroundColor Yellow
}
