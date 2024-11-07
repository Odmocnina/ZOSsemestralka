//
// Created by Michael on 10.10.2024.
//

#ifndef SEMESTRALKA_MEMORYDISK_H
#define SEMESTRALKA_MEMORYDISK_H

#include <stdio.h>
#include "FAT.h"

void writeFatTable(FILE *file, int32_t *fatTable);

int fileExists(const char *filename);

int diskCreation(const char *fileName, char *argument);

long long getSize(char *argument);

void readBlock(FILE *file, int blockNumber, void *buffer, int offset);

void writeToConsoleError(const char *message);

void writeToConsoleNormal(const char *message);

int writeNewKitten(FILE *file, int blockNumber, void *data, int offset);

void writeAllKittensToConsole(FILE *file, int blockNumber, int offset);

void readSpace(FILE *file, int blockNumber, void *buffer, int offset, int index);

void removeDirectory(FILE *file, int blockNumber, struct directoryItem *data, int offset, int index);

int isClusterEmpty(FILE *file, int blockNumber, int offset);

void readDataFromFile(FILE *file, int blockNumber, void *buffer, int size);

void writeDataFromFile(FILE *file, int blockNumber, const void *data, int offset, int size);

int saveFile(FILE *file, FILE *fileToCopy, int *fatTable, int numberOfClusters, long sizeOfFile, int offset,
             int clusterCountInFat);

struct directoryItem findKitten(FILE *file, int blockNumber, int offset, char *name);

int extractFile(FILE *file, FILE *fileToCopy, int *fatTable, long sizeOfFile, int offset, int cluster);

void clearFile(FILE *file, int cluster, int32_t *fat, int offset);

int findIndexOfFile(FILE *file, int blockNumber, int offset);

int copyFile(FILE *file, int *fatTable, long sizeOfFile, int offset, int clusterCountInFat, int startCluster);

int writeFileToConsole(FILE *file, int *fatTable, long sizeOfFile, int offset, int cluster);

int removeDirectoryByName(FILE *file, char *name, int cluster, int offset);

int removeFileByName(FILE *file, char *name, int cluster, int offset);

int findKittenCluser(FILE *file, char *name, int cluster, int offset);

#endif //SEMESTRALKA_MEMORYDISK_H
