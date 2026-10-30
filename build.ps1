$ErrorActionPreference = "Stop"

$gcc = Get-Command arm-none-eabi-gcc -ErrorAction SilentlyContinue
$objcopy = Get-Command arm-none-eabi-objcopy -ErrorAction SilentlyContinue
$binPath = $null
if (-not $gcc -or -not $objcopy) {
  $toolchainRoot = Join-Path ([Environment]::GetFolderPath("ProgramFilesX86")) "Arm GNU Toolchain arm-none-eabi"
  if (Test-Path $toolchainRoot) {
    $install = Get-ChildItem $toolchainRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1
    if ($install) { $binPath = Join-Path $install.FullName "bin" }
  }
  if ($binPath -and (Test-Path (Join-Path $binPath "arm-none-eabi-gcc.exe"))) {
    $gccPath = Join-Path $binPath "arm-none-eabi-gcc.exe"
    $objcopyPath = Join-Path $binPath "arm-none-eabi-objcopy.exe"
  }
}
if ((-not $gcc -and -not $gccPath) -or (-not $objcopy -and -not $objcopyPath)) {
  throw "GNU Arm Embedded Toolchain is required. Install Arm.GnuArmEmbeddedToolchain and reopen PowerShell."
}

$root = $PSScriptRoot
$build = Join-Path $root "build"
New-Item -ItemType Directory -Force -Path $build | Out-Null
$elf = Join-Path $build "pumpkin-parade.elf"
$bin = Join-Path $build "pumpkin-parade.bin"
$rom = Join-Path $build "pumpkin-parade.gba"
$map = Join-Path $build "pumpkin-parade.map"

if ($gcc) { $gccPath = $gcc.Source }
if ($objcopy) { $objcopyPath = $objcopy.Source }

& $gccPath `
  -mcpu=arm7tdmi -marm -O2 -Wall -Wextra -Werror `
  -ffreestanding -fno-builtin -fno-common `
  -fno-unwind-tables -fno-asynchronous-unwind-tables `
  -ffunction-sections -fdata-sections -nostdlib `
  "-Wl,-T,$(Join-Path $root 'gba.ld')" `
  "-Wl,-Map,$map" "-Wl,--gc-sections" "-Wl,--build-id=none" `
  (Join-Path $root "startup.s") (Join-Path $root "main.c") `
  -lgcc -o $elf
if ($LASTEXITCODE -ne 0) { throw "ARM compilation failed with exit code $LASTEXITCODE" }

& $objcopyPath -O binary $elf $bin
if ($LASTEXITCODE -ne 0) { throw "ROM binary extraction failed with exit code $LASTEXITCODE" }

python (Join-Path $root "build_rom.py") $bin $rom
if ($LASTEXITCODE -ne 0) { throw "ROM packaging failed with exit code $LASTEXITCODE" }
