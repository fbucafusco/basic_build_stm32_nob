#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdbool.h>

// ===================== STM32CubeF4 Macros =====================
#define STM32CUBEF4_FOLDER      "STM32CubeF4"
#define STM32CUBEF4_TAG         "v1.28.1"
#define STM32CUBEF4_REPO        "https://github.com/STMicroelectronics/" STM32CUBEF4_FOLDER

// Files inside STM32CubeF4
#define STM32CUBEF4_SYSTEM_C    STM32CUBEF4_FOLDER "/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c"
#define STM32CUBEF4_STARTUP_S   STM32CUBEF4_FOLDER "/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s"
#define STM32CUBEF4_SYSCALLS_C  STM32CUBEF4_FOLDER "/Projects/STM324xG_EVAL/Examples/BSP/STM32CubeIDE/Example/User/syscalls.c"
#define STM32CUBEF4_SYSMEM_C    STM32CUBEF4_FOLDER "/Projects/STM324xG_EVAL/Examples/BSP/STM32CubeIDE/Example/User/sysmem.c"
#define STM32CUBEF4_LDSCRIPT    STM32CUBEF4_FOLDER "/Projects/STM324xG_EVAL/Examples/BSP/STM32CubeIDE/STM32F407IGHX_FLASH.ld" 
#define STM32CUBEF4_INCLUDE     "-I./" STM32CUBEF4_FOLDER "/Drivers/CMSIS/Device/ST/STM32F4xx/Include"
#define STM32CUBEF4_CORE_INCLUDE "-I./" STM32CUBEF4_FOLDER "/Drivers/CMSIS/Core/Include"

// ===================== Build Macros =====================
#define BUILD_TARGET_NAME       "firmware"
#define OBJ_FOLDER              "obj"
#define ASM_LIST_FOLDER         "list"
#define BUILD_DIR_BASE          "build"
#define BUILD_DIR               BUILD_DIR_BASE "/" BUILD_TARGET_NAME
#define OBJ_DIR                 BUILD_DIR "/" OBJ_FOLDER
#define ASM_LIST_DIR            BUILD_DIR "/" ASM_LIST_FOLDER

// ===================== Toolchain Macros =====================
#define OBJ_DUMP                "arm-none-eabi-objdump"
#define COMPILER_C              "arm-none-eabi-gcc"
#define COMPILER_CPP            "arm-none-eabi-g++"
#define LINKER                  "arm-none-eabi-g++"

// ===================== Flags Macros =====================
#define MCU_FLAGS               "-DSTM32F446xx", "-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard"
#define OPTIMIZATION_FLAGS      "-O2"
#define CFLAGS                  ""
#define CPPFLAGS                "-std=c++20","-fno-rtti"
#define ASM_FLAGS               "-mcpu=cortex-m4", \
                                "-mthumb", \
                                "-mfpu=fpv4-sp-d16", \
                                "-mfloat-abi=hard", \
                                "-c", \
                                "-x","assembler-with-cpp", \
                                "-MMD","-MP"

// ===================== Linker Macros =====================
#define LDSCRIPT                STM32CUBEF4_LDSCRIPT
#define LDFLAGS                 "-T", LDSCRIPT,      \
                                "-march=armv7e-m",   \
                                "-Wl,-Map="BUILD_DIR"/" BUILD_TARGET_NAME ".map", \
                                "-Wl,--gc-sections", \
                                "-Wl,--start-group", \
                                "-Wl,--end-group"

// ===================== Includes Macros =====================
#define INCLUDES                STM32CUBEF4_INCLUDE, STM32CUBEF4_CORE_INCLUDE

bool dir_exists(const char *path) 
{
    struct stat sb;
    return (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
}

size_t basename(const char *path, char *dst, size_t dstsz) 
{
    NOB_ASSERT(path != NULL);
    NOB_ASSERT(dst != NULL);
    NOB_ASSERT(dstsz > 0);

    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    const char *dot = strrchr(base, '.');
    size_t len = dot && dot != base ? (size_t)(dot - base) : strlen(base);
    if (dstsz > 0) 
    {
        size_t to_copy = len < dstsz - 1 ? len : dstsz - 1;
        memcpy(dst, base, to_copy);
        dst[to_copy] = '\0';
    }
    return len;
}
 
int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    Cmd cmd = {0};
 
    //CREATE DIRECTORIES
    nob_mkdir_if_not_exists(BUILD_DIR_BASE);
    nob_mkdir_if_not_exists(BUILD_DIR);         //TODO create nob_mkdir_if_not_exists( ... , .rec=true) to create parent dirs if not exist
    nob_mkdir_if_not_exists(OBJ_DIR);
    nob_mkdir_if_not_exists(ASM_LIST_DIR);

    //DOWNLOAD STM32CUBEF4    
    if (!dir_exists(STM32CUBEF4_FOLDER)) 
    {
        nob_log(INFO, "Directory %s does not exist. Cloning from %s...", STM32CUBEF4_FOLDER, STM32CUBEF4_REPO);
        nob_cmd_append(&cmd, "git", "clone", "--depth", "1", "--branch", STM32CUBEF4_TAG, STM32CUBEF4_REPO);
        if (!nob_cmd_run(&cmd)) return 1;
        nob_cmd_append(&cmd, "git", "-C", STM32CUBEF4_FOLDER, "submodule", "update", "--init", "--recursive");
        if (!nob_cmd_run(&cmd)) return 1;
    }

    const char * c_files[] = 
    { 
        STM32CUBEF4_FOLDER "/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c",
        STM32CUBEF4_FOLDER "/Projects/STM324xG_EVAL/Examples/BSP/STM32CubeIDE/Example/User/syscalls.c",
        STM32CUBEF4_FOLDER "/Projects/STM324xG_EVAL/Examples/BSP/STM32CubeIDE/Example/User/sysmem.c"
    };
    
    const char * cpp_files[] = 
    { 
        "main.cpp" 
    };

    const char * asm_files[] = 
    {
        STM32CUBEF4_FOLDER "/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s"
    };

    const char ** obj_files = malloc( NOB_ARRAY_LEN(c_files) * sizeof(char*)    +
                                      NOB_ARRAY_LEN(cpp_files) * sizeof(char*)  +
                                      NOB_ARRAY_LEN(asm_files) * sizeof(char*));

    size_t obj_files_count = 0;

    //compile c files
    for (size_t i =0; i < sizeof(c_files) / sizeof(c_files[0]); i++) 
    {  
        nob_log(INFO, "Compiling C file: %s", c_files[i]);
        char file_basename[256] = {0};
        size_t file_basename_len = basename(c_files[i],  file_basename, sizeof(file_basename));     

        Nob_String_Builder sb = {0};
        nob_sb_append_cstr(&sb, OBJ_DIR "/");
        nob_sb_append_cstr(&sb, file_basename);
        nob_sb_append_cstr(&sb, "_c.o");
        nob_sb_append_null(&sb);

        cmd_append(&cmd, COMPILER_C, "-c", c_files[i],  CFLAGS , MCU_FLAGS, OPTIMIZATION_FLAGS , INCLUDES , "-o", sb.items );
        // cmd_append(&cmd, COMPILER_C, "-c", c_files[i],    MCU_FLAGS, OPTIMIZATION_FLAGS , INCLUDES , "-o", sb.items );
        
        if (!cmd_run(&cmd)) return 1;

        obj_files[obj_files_count++] = sb.items;
    }

    //compile cpp files
    for (size_t i =0; i < sizeof(cpp_files) / sizeof(cpp_files[0]); i++) 
    {  
        nob_log(INFO, "Compiling C++ file: %s", cpp_files[i]);
        char file_basename[256] = {0};
        size_t file_basename_len = basename(cpp_files[i],   file_basename, sizeof(file_basename));     

        Nob_String_Builder sb = {0};
        nob_sb_append_cstr(&sb, OBJ_DIR "/");
        nob_sb_append_cstr(&sb, file_basename);
        nob_sb_append_cstr(&sb, "_cpp.o");
        nob_sb_append_null(&sb);

        cmd_append(&cmd, COMPILER_CPP, "-v",  "-c", cpp_files[i],   CPPFLAGS , MCU_FLAGS, OPTIMIZATION_FLAGS , INCLUDES ,  "-o", sb.items );
        if (!cmd_run(&cmd)) return 1;

        obj_files[obj_files_count++] = sb.items;
    }

    //Assembly file compilation
    for (size_t i =0; i < sizeof(asm_files) / sizeof(asm_files[0]); i++) 
    {  
        nob_log(INFO, "Compiling ASM file: %s", asm_files[i]);
        char file_basename[256] = {0};
        size_t file_basename_len = basename(asm_files[i],   file_basename, sizeof(file_basename));     

        Nob_String_Builder sb = {0};
        nob_sb_append_cstr(&sb, OBJ_DIR "/");
        nob_sb_append_cstr(&sb, file_basename);
        nob_sb_append_cstr(&sb, "_asm.o");
        nob_sb_append_null(&sb);

        cmd_append(&cmd, COMPILER_C, ASM_FLAGS , "-c", asm_files[i],  INCLUDES , "-o", sb.items );
        if (!cmd_run(&cmd)) return 1;

        obj_files[obj_files_count++] = sb.items;
    }

    //LINKING
    nob_log(INFO, "Linking...");
    cmd_append(&cmd, LINKER);

    for (size_t i = 0; i < obj_files_count; ++i)
    {
        cmd_append(&cmd, obj_files[i]);
    }

    cmd_append(&cmd, LDFLAGS, MCU_FLAGS,  "-o", BUILD_DIR "/" BUILD_TARGET_NAME ".elf");  
    
    if (!cmd_run(&cmd)) return 1;

    //OBJ DUMP
    for (size_t i = 0; i < obj_files_count; ++i)
    {
        char file_basename[256] = {0};
        size_t file_basename_len = basename(obj_files[i],   file_basename, sizeof(file_basename));     

        Nob_String_Builder sb = {0};
        nob_sb_append_cstr(&sb, ASM_LIST_DIR "/");
        nob_sb_append_cstr(&sb, file_basename);
        nob_sb_append_cstr(&sb, ".lst");
        nob_sb_append_null(&sb);

        cmd_append(&cmd, OBJ_DUMP, "-S", "--demangle", obj_files[i]  );
        if (!cmd_run(&cmd ,  .stdout_path = sb.items   )) return 1;
    } 

        // OBJ DUMP del ELF completo
        nob_log(INFO, "Generating full ELF lst...");
        cmd_append(&cmd, OBJ_DUMP, "-S", "--demangle", BUILD_DIR "/" BUILD_TARGET_NAME ".elf");
        if (!cmd_run(&cmd, .stdout_path = ASM_LIST_DIR "/firmware.lst")) return 1;

    free(obj_files);


    return 0;
}