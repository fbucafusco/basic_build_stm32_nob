Example of building a STM32 project using [nob.h](https://github.com/tsoding/nob.h).
It is based on [this project](https://github.com/fbucafusco/basic_build_stm32_bash) where I did the same using bash.

# Usage

```bash
gcc mk.c -o mk
./mk
```

# Install ARM Toolchain

You can install the ARM GNU Toolchain either locally (for this project only) or system-wide. Choose the method that best fits your needs:

## Option 1: Local Installation (Recommended for quick setup)

1. Download the official ARM GNU Toolchain:

```bash
wget https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz
```	

2. Extract the toolchain in your project directory:

```bash
tar -xf arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz
```

3. Use the binaries by specifying their path in your build scripts or by temporarily adding them to your PATH:

```bash
export PATH="$PWD/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin:$PATH"
```

## Option 2: System-wide Installation (Requires sudo)

1. Create installation directory:
```bash
sudo mkdir -p /opt/arm-gnu-toolchain-14.3.rel1
```
2. Extract the toolchain to `/opt`:
```bash
sudo tar -xJf arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz -C /opt/arm-gnu-toolchain-14.3.rel1 --strip-components=1
```
3. Add the toolchain to your PATH (append to `~/.bashrc`):
```bash
echo 'export PATH=/opt/arm-gnu-toolchain-14.3.rel1/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

## Verify Installation

Run the following command to check the toolchain is available:
```bash
arm-none-eabi-g++ -v
```
