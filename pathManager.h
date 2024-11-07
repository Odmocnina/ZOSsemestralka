//
// Created by Michael on 28.10.2024.
//

#ifndef SEMESTRALKA_PATHMANAGER_H
#define SEMESTRALKA_PATHMANAGER_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "commands.h"
#include "constants.h"
#include "memoryDisk.h"
#include "FAT.h"

int findPathInDisk(FILE *file, char *path, int currentCluster, int changeCurrentCluster);

char *circumsisePath(char *path);

int countSlashes(const char *str);

char* trim(char* str);

void writePathCommand();

void writePathStackCommand();

void makeSpaceForSmallKittens();

#endif //SEMESTRALKA_PATHMANAGER_H
