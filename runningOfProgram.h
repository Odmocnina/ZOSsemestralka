//
// Created by Michael on 09.10.2024.
//

#ifndef SEMESTRALKA_RUNNINGOFPROGRAM_H
#define SEMESTRALKA_RUNNINGOFPROGRAM_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "constants.h"

void run(const char *fileName);

int processCommandsFromFile(const char *fileName, int needToLoadFatTable, FILE *loadFile, int currentCluster);

#endif //SEMESTRALKA_RUNNINGOFPROGRAM_H
