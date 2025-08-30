
cmake_minimum_required(VERSION 3.15)
project(firmware C CXX ASM)

# Toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# Directories
set(BUILD_DIR ${CMAKE_BINARY_DIR}/build/firmware)
set(OBJ_DIR ${BUILD_DIR}/obj)
set(ASM_LIST_DIR ${BUILD_DIR}/list)
file(MAKE_DIRECTORY ${BUILD_DIR})
file(MAKE_DIRECTORY ${OBJ_DIR})
file(MAKE_DIRECTORY ${ASM_LIST_DIR})

# Automatic download of STM32CubeF4 as a direct dependency
include(FetchContent)
FetchContent_Declare(
    STM32CubeF4
    GIT_REPOSITORY https://github.com/STMicroelectronics/STM32CubeF4.git
    GIT_TAG v1.28.1
)
FetchContent_MakeAvailable(STM32CubeF4)

# STM32CubeF4 paths

set(STM32CUBEF4_FOLDER ${stm32cubef4_SOURCE_DIR})
set(STM32CUBEF4_SYSTEM_C ${STM32CUBEF4_FOLDER}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c)
set(STM32CUBEF4_STARTUP_S ${STM32CUBEF4_FOLDER}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s)
set(STM32CUBEF4_SYSCALLS_C ${STM32CUBEF4_FOLDER}/Projects/STM324xG_EVAL/Examples/BSP/STM32CubeIDE/Example/User/syscalls.c)
set(STM32CUBEF4_SYSMEM_C ${STM32CUBEF4_FOLDER}/Projects/STM324xG_EVAL/Examples/BSP/STM32CubeIDE/Example/User/sysmem.c)
set(STM32CUBEF4_LDSCRIPT ${STM32CUBEF4_FOLDER}/Projects/STM32F4-Discovery/Examples/GPIO/GPIO_EXTI/STM32CubeIDE/STM32F407VGTX_FLASH.ld)

# Includes
include_directories(
    ${STM32CUBEF4_FOLDER}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${STM32CUBEF4_FOLDER}/Drivers/CMSIS/Core/Include
)

# Flags
set(MCU_FLAGS "-DSTM32F446xx -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(OPTIMIZATION_FLAGS "-O2")
set(CMAKE_C_FLAGS "${MCU_FLAGS} ${OPTIMIZATION_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++20 -fno-rtti ${MCU_FLAGS} ${OPTIMIZATION_FLAGS}")
set(CMAKE_ASM_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -c -x assembler-with-cpp -MMD -MP")

# Sources
set(C_SOURCES
    ${STM32CUBEF4_SYSTEM_C}
    ${STM32CUBEF4_SYSCALLS_C}
    ${STM32CUBEF4_SYSMEM_C}
)
set(CPP_SOURCES
    ${CMAKE_SOURCE_DIR}/main.cpp
)
set(ASM_SOURCES
    ${STM32CUBEF4_STARTUP_S}
)

add_executable(firmware.elf
    ${C_SOURCES}
    ${CPP_SOURCES}
    ${ASM_SOURCES}
)

# Linker script and flags
set_target_properties(firmware.elf PROPERTIES
    LINK_FLAGS "-T${STM32CUBEF4_LDSCRIPT} -march=armv7e-m -Wl,-Map=${BUILD_DIR}/firmware.map -Wl,--gc-sections -Wl,--start-group -Wl,--end-group"
)

# Objdump (custom target for listing)
add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND arm-none-eabi-objdump -S --demangle $<TARGET_FILE:firmware.elf> > ${ASM_LIST_DIR}/firmware.lst
    COMMENT "Generating assembly listing with objdump"
)

# Generate .lst for each object before linking

# Where to put the .lst files
# set(ASM_LIST_DIR "${CMAKE_BINARY_DIR}/build/firmware/list")

add_custom_target(objdumps ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${ASM_LIST_DIR}"
  # For each *.o under CMakeFiles, dump mixed source/asm into ${ASM_LIST_DIR}/<base>.lst
  COMMAND /bin/bash -c
          "find '${CMAKE_BINARY_DIR}/CMakeFiles' -name '*.o' \
             -exec /bin/bash -c 'f=\$\$1; \
                                  base=`basename \"\$\$f\" .o`; \
                                  base=`printf \"%s\" \"\$\$base\" | cut -d. -f1`; \
                                  arm-none-eabi-objdump -S --demangle \"\$\$f\" > \"${ASM_LIST_DIR}/\$\$base.lst\"' _ {} \\;"
  COMMENT "Generating .lst files for each object file found in CMakeFiles recursively"
  VERBATIM
)

# Make sure objects exist first (rename if your ELF target is different)
add_dependencies(objdumps firmware.elf)