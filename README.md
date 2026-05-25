# CoreTST Ultra Burner

A lightweight CPU stress-testing utility for Windows that maximizes 
thermal load across all logical cores using AVX2 FMA instructions.

## Features
- Auto-detects logical core count
- AVX2 mode: 8 independent FMA chains per thread (64 floats/cycle)
- Scalar fallback for CPUs without AVX2 support
- Thread affinity pinning — one thread per physical core
- Safe overflow handling, denormal penalty prevention (DAZ/FTZ)

## Requirements
- Windows 10 / 11 / Server 2019 / 22
- MSVC (Visual Studio 2022+)
- CPU with AVX2 support (optional, auto-detected)
- Compile with `/arch:AVX2 /O2`

## Usage
Run the executable. Press **Enter** to stop the test and let the CPU cool down.

## Use Cases
- Thermal paste/cooler validation after installation
- Thermal throttling detection
- Stability testing after overclocking
- VRM (Voltage Regulator Module) power delivery stress-testing