//
// Created by Michael on 09.10.2024.
//

#ifndef SEMESTRALKA_COMMANDS_H
#define SEMESTRALKA_COMMANDS_H

#include <stdint.h>

void help();

void wrongCommand(char *command);

int cp(char *arg1, char *arg2, const char *fileName, int *fatTable, int currentCluster);

int format(char *arg1, const char *fileName, int32_t **fatTable);

int mkdir(char *arg1, const char *fileName, int *fatTable, int currentCluster);

int ls(const char *fileName, int currentCluster, char *args1);

int cd(char *arg1, const char *fileName, int currentCluster);

int rmdir(char *arg1, const char *fileName, int *fatTable, int currentCluster);

void pwd(const char *fileName);

int incp(char *arg1, char *arg2, const char *fileName, int *fatTable, int currentCluster);

int outcp(char *arg1, char *arg2, const char *fileName, int *fatTable, int currentCluster);

int rm(char *arg1, const char *fileName, int *fatTable, int currentCluster);

int info(char *arg1, const char *fileName, int *fatTable, int currentCluster);

int mv(char *arg1, char *arg2, const char *fileName, int currentCluster);

int cat(char *arg1, const char *fileName, int *fatTable, int currentCluster);

int load(char *arg1, const char *fileName, int currentCluster, int needToLoadFatTable);

int bug(char *arg1, const char *fileName, int *fatTable, int currentCluster);

int check(const char *fileName, int *fatTable);

#endif //SEMESTRALKA_COMMANDS_H
