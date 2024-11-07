//
// Created by Michael on 09.10.2024.
//

#ifndef SEMESTRALKA_FAT_H
#define SEMESTRALKA_FAT_H

#include <stdint.h>
#include "constants.h"

struct description {
    char signature[7];
    int32_t diskSize;
    int32_t clusterSize;
    int32_t clusterCount;
    int32_t fat1StartAddress;
    int32_t dataStartAddress;
};




struct directoryItem {
    char itemName[14];
    int isFile;
    int32_t size;
    int32_t startCluster;
};

void inicilizeFAT(int32_t *fat, int32_t fatSize);

void createDirectoryItem(struct directoryItem *item, const char *name, int isFile, int32_t startCluster, int size);

int32_t allocateCluster(int32_t *fat, int32_t fatSize);

int getNumberOfFreeClusters(int32_t *fat, int32_t fatSize);

void printFATTable(int32_t *fat, int32_t fatSize);

void clearFATTableFile(int32_t *fat, int cluster);

void printClustersFile(int32_t *fat, int cluster);

#endif //SEMESTRALKA_FAT_H
