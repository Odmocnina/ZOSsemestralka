cmake_minimum_required(VERSION 3.28)
project(Semestralka C)

set(CMAKE_C_STANDARD 11)

add_executable(Semestralka main.c
        constants.h
        runningOfProgram.h
        runningOfProgram.c
        commands.h
        commands.c
        FAT.h
        memoryDisk.h
        memoryDisk.c
        FAT.c
        stack.h
        stack.c
        stackTools.h
        pathManager.c
        pathManager.h
)
