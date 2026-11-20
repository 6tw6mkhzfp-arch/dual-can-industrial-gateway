$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent $MyInvocation.MyCommand.Path
$fw = Join-Path $env:USERPROFILE 'STM32Cube\Repository\STM32Cube_FW_F1_V1.8.7'
$ide = 'D:\STM32CUBE\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins'
$gcc = Get-ChildItem -LiteralPath $ide -Filter 'arm-none-eabi-gcc.exe' -Recurse -File |
    Select-Object -First 1 -ExpandProperty FullName
$objcopy = Join-Path (Split-Path -Parent $gcc) 'arm-none-eabi-objcopy.exe'
$size = Join-Path (Split-Path -Parent $gcc) 'arm-none-eabi-size.exe'
if (-not (Test-Path -LiteralPath $fw)) { throw "Missing STM32CubeF1 package: $fw" }

$halSrc = Join-Path $fw 'Drivers\STM32F1xx_HAL_Driver\Src'
$cmsis = Join-Path $fw 'Drivers\CMSIS'
$template = Join-Path $fw 'Projects\STM32F103RB-Nucleo\Templates'
$build = Join-Path $project 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null

$includes = @(
    "-I$project"
    "-I$(Join-Path $template 'Inc')"
    "-I$(Join-Path $fw 'Drivers\STM32F1xx_HAL_Driver\Inc')"
    "-I$(Join-Path $cmsis 'Include')"
    "-I$(Join-Path $cmsis 'Device\ST\STM32F1xx\Include')"
)
$common = @('-mcpu=cortex-m3','-mthumb','-O2','-g3',
    '-ffunction-sections','-fdata-sections','-DSTM32F103xB','-DUSE_HAL_DRIVER')
$sources = @(
    (Join-Path $project 'bench_main.c'),
    (Join-Path $project 'can_test.c'),
    (Join-Path $template 'Src\system_stm32f1xx.c'),
    (Join-Path $halSrc 'stm32f1xx_hal.c'),
    (Join-Path $halSrc 'stm32f1xx_hal_can.c'),
    (Join-Path $halSrc 'stm32f1xx_hal_adc.c'),
    (Join-Path $halSrc 'stm32f1xx_hal_adc_ex.c'),
    (Join-Path $halSrc 'stm32f1xx_hal_rcc.c'),
    (Join-Path $halSrc 'stm32f1xx_hal_rcc_ex.c'),
    (Join-Path $halSrc 'stm32f1xx_hal_gpio.c'),
    (Join-Path $halSrc 'stm32f1xx_hal_cortex.c'),
    (Join-Path $template 'SW4STM32\startup_stm32f103xb.s')
)
$objects = @()
foreach ($source in $sources) {
    $object = Join-Path $build (([IO.Path]::GetFileNameWithoutExtension($source)) + '.o')
    & $gcc @common @includes -c $source -o $object
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    $objects += $object
}
$linker = Join-Path $fw 'Projects\STM32F103RB-Nucleo\Templates_LL\SW4STM32\NUCLEO-F103RB\STM32F103RBTx_FLASH.ld'
$elf = Join-Path $build 'can_bench.elf'
$bin = Join-Path $build 'can_bench.bin'
& $gcc @common @objects ('-T' + $linker) '-Wl,--gc-sections' '--specs=nano.specs' '--specs=nosys.specs' -o $elf
if ($LASTEXITCODE -ne 0) { throw 'Link failed' }
& $objcopy -O binary $elf $bin
if ($LASTEXITCODE -ne 0) { throw 'objcopy failed' }
& $size $elf
Get-FileHash -LiteralPath $bin -Algorithm SHA256 | Select-Object Path,Hash
