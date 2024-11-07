//
// Created by Michael on 09.10.2024.
//

#ifndef SEMESTRALKA_CONSTANTS_H
#define SEMESTRALKA_CONSTANTS_H

#include <stdint.h>

#define LENGTH_OF_USER_INPUT 256
#define SUCCES_VALUE 1
#define FAILURE_VALUE -1
#define TRUE 1
#define FALSE 0
#define BLOCK_SIZE 4096  // Velikost jednoho bloku
#define FAT_UNUSED -2
#define FAT_FILE_END -1
#define FAT_BAD_CLUSTER -10
#define FAT_TABLE_FULL -3
#define MAX_NUMBER_OF_SUBDIRECTORIES 128
#define MAXIMUM_DEAPTH 64
#define DELETED_ITEM -2
#define CLUSTER_NOT_EMPTY -5
#define CLUSTER_IS_FOLDER -15

#endif //SEMESTRALKA_CONSTANTS_H
