//
// Created by Michael on 10.10.2024.
//

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "FAT.h"
#include "constants.h"
#include "memoryDisk.h"

/**
 * Inicializuje tabulku FAT tim, ze nastavi vsechny jeji polozky na hodnotu FAT_UNUSED.
 * @param fat Ukazatel na pole reprezentujici tabulku FAT.
 * @param fatSize Velikost tabulky FAT.
 */
void inicilizeFAT(int32_t *fat, int32_t fatSize) {
    for (int32_t i = 0; i < fatSize; i = i + 1) {
        fat[i] = FAT_UNUSED;
    }
}

/**
 * Hleda prvni volny cluster v tabulce FAT a vraci jeho index.
 * @param fat Ukazatel na pole reprezentujici tabulku FAT.
 * @param fatSize Velikost tabulky FAT.
 * @return Index prvniho volneho clusteru, nebo FAT_TABLE_FULL, pokud je tabulka plna.
 */
int32_t allocateCluster(int32_t *fat, int32_t fatSize) {
    for (int32_t i = 0; i < fatSize; i = i + 1) {
        if ((int)((fat)[i]) == FAT_UNUSED) {
            return i;  // Return the first available cluster
        }
    }
    return FAT_TABLE_FULL;  // No free clusters
}

/**
 * Pocita pocet volnych clusteru v tabulce FAT.
 * @param fat Ukazatel na pole reprezentujici tabulku FAT.
 * @param fatSize Velikost tabulky FAT.
 * @return Pocet volnych clusteru.
 */
int getNumberOfFreeClusters(int32_t *fat, int32_t fatSize) {
    int navrat = 0;
    for (int32_t i = 0; i < fatSize; i = i + 1) {
        if ((int)((fat)[i]) == FAT_UNUSED) {
            navrat = navrat + 1;
        }
    }
    return navrat;  // nejdou volne clustery
}

/**
 * Uvolni dany cluster v tabulce FAT nastavenim na hodnotu FAT_UNUSED.
 * @param fat Ukazatel na pole reprezentujici tabulku FAT.
 * @param clusterIndex Index clusteru, ktery ma byt uvolnen.
 */
void freeCluster(int32_t *fat, int32_t clusterIndex) {
    fat[clusterIndex] = FAT_UNUSED;
}

/**
 * Vytvori novou polozku adresare s danymi atributy.
 * @param item Ukazatel na strukturu directoryItem pro ulozeni nove polozky.
 * @param name Nazev polozky.
 * @param isFile Priznak, zda je polozka souborem (1) nebo adresarem (0).
 * @param startCluster Zacatecni cluster polozky.
 * @param size Velikost polozky (v bytech).
 */
void createDirectoryItem(struct directoryItem *item, const char *name, int isFile, int32_t startCluster, int size) {
    strncpy(item->itemName, name, sizeof(item->itemName) - 1);
    item->isFile = isFile;
    item->size = size;
    item->startCluster = startCluster;
}

/**
 * Vypise obsah tabulky FAT na konzoli. Pouzito pri debugovani.
 * @param fat Ukazatel na pole reprezentujici tabulku FAT.
 * @param fatSize Velikost tabulky FAT.
 */
void printFATTable(int32_t *fat, int32_t fatSize) {
    for (int i = 0; i < fatSize; i = i + 1) {
        printf("fat on %d: %d\n", i, fat[i]);
    }
}

/**
 * Vymaze vsechny clustery souboru z tabulky FAT, pocinaje zadanym clusterem.
 * @param fat Ukazatel na pole reprezentujici tabulku FAT.
 * @param cluster Zacatecni cluster souboru k vymazani.
 */
void clearFATTableFile(int32_t *fat, int cluster) {
    int FATContent;
    while (cluster != FAT_FILE_END) {
        FATContent = fat[cluster];
        fat[cluster] = FAT_UNUSED;
        cluster = FATContent;
    }
}

/**
 * Vypise vsechny clustery obsahujici dany soubor na konzoli.
 * @param fat Ukazatel na pole reprezentujici tabulku FAT.
 * @param cluster Zacatecni cluster souboru.
 */
void printClustersFile(int32_t *fat, int cluster) {
    writeToConsoleNormal("Clusters containing file:");
    int FATContent;
    while (cluster != FAT_FILE_END) {
        printf(" %d ", cluster);
        FATContent = fat[cluster];
        cluster = FATContent;
    }
}





