//
// Created by Michael on 10.10.2024.
//


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "constants.h"
#include "memoryDisk.h"
#include "FAT.h"

/** Vypise zpravu na konzoli bez jakehokoli predpony
 * @param message zprava, ktera bude vypsana na konzoli
 */
void writeToConsoleNormal(const char *message) {
    printf("%s",message);
}

/** Vypise chybovou zpravu na konzoli s predponou "===Error==="
 * @param message chybova zprava, ktera bude vypsana na konzoli
 */
void writeToConsoleError(const char *message) {
    printf("===Error=== %s", message);
}

/** Prevede velikost pameti z dane jednotky (B, KB, MB, GB) na byty
 * @param size velikost, ktera ma byt prevedena
 * @param unit jednotka velikosti (B, KB, MB, GB)
 * @return prevedena velikost v bytech nebo FAILURE_VALUE pri chybe
 */
long long convertToBytes(long long size, char *unit) {
    long long navrat = FAILURE_VALUE;
    if (strcmp(unit, "B") == 0) {
        navrat = size; // Bytes
    } else if (strcmp(unit, "KB") == 0) {
        navrat = size * 1024; // Kilobytes
    } else if (strcmp(unit, "MB") == 0) {
        navrat = size * 1024 * 1024; // Megabytes
    } else if (strcmp(unit, "GB") == 0) {
        navrat = size * 1024 * 1024 * 1024; // Gigabytes
    }

    return navrat;
}

/** Ziska velikost a jednotku z argumentu a prevadi na byty
 * @param argument retezec obsahujici velikost a jednotku (napr. "10MB")
 * @return velikost v bytech nebo FAILURE_VALUE pri chybe
 */
long long getSize(char *argument) {
    long long size;
    char unit[3]; // Pro jednotky (napr. MB, GB)
    long long navrat = FAILURE_VALUE;

        // Extrahujeme cislo a jednotky
    if (sscanf(argument, "%lld%2s", &size, unit) == 2) {
        // Prevest jednotky na velka pismena (pro kontrolu)
        for (int i = 0; unit[i]; i = i + 1) {
            unit[i] = toupper(unit[i]);
        }

        // Zkontrolujeme, zda jsou jednotky platne
        long long convertedSize = convertToBytes(size, unit);
        if (convertedSize != FAILURE_VALUE) {
            navrat = convertedSize;
        } else {
            writeToConsoleError("!(good format of memory)");
        }
    }

    return (navrat + 1);
}

/** Zapise FAT tabulku do souboru
 * @param file otevreny soubor, do ktereho se zapise FAT tabulka
 * @param fatTable ukazatel na pole obsahujici FAT tabulku
 */
void writeFatTable(FILE *file, int32_t *fatTable) {
    struct description FATdescription;
    fseek(file, 0, SEEK_SET);
    // Precteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    fseek(file, sizeof(FATdescription), SEEK_SET);
    fwrite(fatTable, sizeof(int32_t), FATdescription.clusterCount, file);
}

/**
 * Precte blok dat ze souboru a ulozi ho do bufferu
 * @param file Soubor, ze ktereho chceme cist
 * @param blockNumber Cislo bloku, ktery chceme cist
 * @param buffer Ukazatel na buffer, do ktereho se budou data ukladat
 * @param offset Posun v ramci bloku
 */
void readBlock(FILE *file, int blockNumber, void *buffer, int offset) {
    // Presuneme se na pozici zacatku pozadovaneho bloku
    fseek(file, (blockNumber * BLOCK_SIZE) + offset, SEEK_SET);

    // Precteme blok dat do bufferu
    fread(buffer, 1, BLOCK_SIZE, file);

}

/**
 * Precte data ze souboru o urcene velikosti a ulozi je do bufferu
 * @param file Soubor, ze ktereho chceme cist
 * @param blockNumber Cislo bloku, ktery chceme cist
 * @param buffer Ukazatel na buffer, do ktereho se budou data ukladat
 * @param size Velikost dat ke cteni
 */
void readDataFromFile(FILE *file, int blockNumber, void *buffer, int size) {
    // Presuneme se na pozici zacatku pozadovaneho bloku
    fseek(file, (blockNumber * BLOCK_SIZE), SEEK_SET);

    // Precteme blok dat do bufferu
    fread(buffer, 1, size, file);

}

/**
 * Zapise data do souboru na urcene misto
 * @param file Soubor, do ktereho chceme zapisovat
 * @param blockNumber Cislo bloku, kam chceme zapisovat
 * @param data Ukazatel na data, ktera chceme zapsat
 * @param offset Posun v ramci bloku
 * @param size Velikost dat k zapisu
 */
void writeDataFromFile(FILE *file, int blockNumber, const void *data, int offset, int size) {
    // Presuneme se na pozici zacatku pozadovaneho bloku
    fseek(file, (blockNumber * BLOCK_SIZE) + offset, SEEK_SET);
    // Zapíseme data do souboru
    fwrite(data, 1, size, file);
}

/**
 * Vymaze data v souboru v urcitem bloku a nahradi je hodnotou 0xFF
 * @param file Soubor, ve kterem chceme data mazat
 * @param offset Posun v ramci bloku
 * @param cluster Cislo klastru, ktery chceme mazat
 */
void readDataFromDiskFile(FILE *file, int blockNumber, void *buffer, int size, int offsize) {
    //    // Presuneme se na pozici zacatku pozadovaného bloku
    fseek(file, (blockNumber * BLOCK_SIZE) + offsize, SEEK_SET);

    // Precteme blok dat do bufferu
    fread(buffer, 1, size, file);

}

/**
 * Vymaze obsah souboru od zacatku klastru az po jeho konec
 * @param file Soubor, ve kterem chceme data mazat
 * @param cluster Zacatecni klastry
 * @param fat Odkaz na FAT tabulku
 * @param offset Posun v ramci bloku
 */
void clearSpace(FILE *file, int offset, int cluster) {
    fseek(file, (cluster * BLOCK_SIZE) + offset, SEEK_SET);
    unsigned char buffer[BLOCK_SIZE];
    memset(buffer, 0xFF, sizeof(buffer));   // prepsani bloku na prazne znaky
    fwrite(buffer, 1, BLOCK_SIZE, file);
}

/**
 * Vymaze obsah souboru od zacatku klastru az po jeho konec
 * @param file Soubor, ve kterem chceme data mazat
 * @param cluster Zacatecni klastry
 * @param fat Odkaz na FAT tabulku
 * @param offset Posun v ramci bloku
 */
void clearFile(FILE *file, int cluster, int32_t *fat, int offset) {
    int FATContent;
    while (cluster != FAT_FILE_END) {
        clearSpace(file, offset, cluster);
        FATContent = fat[cluster];
        cluster = FATContent;
    }
}

/**
 * Inicializuje soubor jako disk o urcene velikosti
 * @param fileName Jmeno souboru, ktery ma slouzit jako disk
 * @param sizeOfDisc Velikost disku v bytech
 * @return Navratova hodnota uspesnosti
 */
int initializeDisk(const char *fileName, long long sizeOfDisc) {
    // Otevreme soubor pro binarni zapis
    if (fileName == NULL) {
        writeToConsoleError("Error opening file");
        return FAILURE_VALUE;
    }
    FILE *file = fopen(fileName, "wb+");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        return FAILURE_VALUE;
    }

    // Inicializujeme prazdny blok
    char emptyBlock[BLOCK_SIZE];memset(emptyBlock, 0xFF, BLOCK_SIZE);


    int numberOfClusters = (sizeOfDisc / BLOCK_SIZE) + 1;

    // Zapiseme prazdné bloky do souboru (formatovani disku)
    for (int i = 0; i < numberOfClusters; i = i + 1) {
        fwrite(emptyBlock, 1, BLOCK_SIZE, file);
    }

    // Zavreme soubor
    fclose(file);

    return SUCCES_VALUE;
}

/**
 * Vytvori soubor jako disk o urcene velikosti
 * @param fileName Jmeno souboru, ktery ma slouzit jako disk
 * @param argument Velikost disku ve stringu
 * @return Navratova hodnota uspesnosti
 */
int diskCreation(const char *fileName, char *argument) {
    if (fileName == NULL) {
        writeToConsoleError("Error opening file");
        return FAILURE_VALUE;
    }
    int navrat = FAILURE_VALUE;
    int size = getSize(argument);
    if (size != FAILURE_VALUE) {
        navrat = initializeDisk(fileName, size);
    }

    return navrat;
}

/**
 * Testuje, zda soubor existuje
 * @param filename Jmeno souboru, ktery chceme testovat
 * @return TRUE pokud soubor existuje, jinak FALSE
 */
int fileExists(const char *filename) {
    FILE *file = fopen(filename, "r");
    int navrat = FALSE;
    if (file != NULL) {
        fclose(file);
        navrat = TRUE;
    }
    return navrat;
}



/**
 * Testuje, zda je jmeno prazdne
 * @param name Ukazatel na retezec s nazvem souboru
 * @return TRUE pokud je jmeno prazdne, jinak FALSE
 */
int isNameEmpty(char *name) {
    int navrat = TRUE;
    int i = 0;
    while (navrat == TRUE && i < 14) {
        if (name[i] != (char)0xFF) {
            navrat = FALSE;
        } else {
            i = i + 1;
        }
    }
    return navrat;
}

/**
 * Kontroluje, zda je misto v adresarove polozce prazdne nebo smazane
 * @param place Adresarova polozka ke kontrole
 * @return TRUE pokud je misto prazdne, jinak FALSE
 */
int isSpaceEmpty(struct directoryItem place) {
    int navrat = FALSE;
    if (place.isFile == DELETED_ITEM || isNameEmpty(place.itemName)) {
        navrat = TRUE;
    }
    return navrat;
}

/**
 * Zjisti, zda je cely klastr prazdny
 * @param file Soubor pro cteni klastru
 * @param blockNumber Cislo bloku
 * @param offset Posun v ramci bloku
 * @return TRUE pokud je cely klastr prazdny, jinak FALSE
 */
int isClusterEmpty(FILE *file, int blockNumber, int offset) {
    char *clusterContent;
    readBlock(file, blockNumber, &clusterContent, offset);
    int navrat = TRUE;
    int i = 0;
    while ((navrat == TRUE) && (i < MAX_NUMBER_OF_SUBDIRECTORIES)) {
        struct directoryItem place;
        readSpace(file, blockNumber, &place, offset, i);
        if (isSpaceEmpty(place) == FALSE) {
            navrat = FALSE;
        } else {
            i = i + 1;
        }
    }
    return navrat;
}

/**
 * Precte adresarovou polozku z daneho mista v souboru
 * @param file Soubor, ze ktereho chceme cist
 * @param blockNumber Cislo bloku
 * @param buffer Ukazatel na buffer pro nactena data
 * @param offset Posun v ramci bloku
 * @param index Index polozky
 */
void readSpace(FILE *file, int blockNumber, void *buffer, int offset, int index) {
    // Presuneme se na pozici zacatku pozadovaneho bloku
    fseek(file, (blockNumber * BLOCK_SIZE) + offset + index * sizeof(struct directoryItem), SEEK_SET);

    // Precteme blok dat do bufferu
    fread(buffer, 1, sizeof(struct directoryItem), file);

}

/**
 * Odstrani adresarovou polozku z adresare
 * @param file Soubor, ze ktereho chceme polozku odstranit
 * @param blockNumber Cislo bloku
 * @param data Data k odstraneni
 * @param offset Posun v ramci bloku
 * @param index Index polozky
 */
void removeDirectory(FILE *file, int blockNumber, struct directoryItem *data, int offset, int index) {
    fseek(file, (blockNumber * BLOCK_SIZE) + offset + index * sizeof(struct directoryItem), SEEK_SET);
    fwrite(data, 1, sizeof(struct directoryItem), file);
}

/**
 * Najde volne misto pro zapis souboru v adresari
 * @param file Soubor, kde hledame volne misto
 * @param blockNumber Cislo bloku
 * @param offset Posun v ramci bloku
 * @return Index volneho mista nebo FAILURE_VALUE
 */
int findSpaceForKitten(FILE *file, int blockNumber, int offset) {
    int spaceForKittenFound = FAILURE_VALUE;
    int i = 0;
    while (spaceForKittenFound == FAILURE_VALUE && i < MAX_NUMBER_OF_SUBDIRECTORIES) {
        struct directoryItem place;
        readSpace(file, blockNumber, &place, offset, i);
        if (isSpaceEmpty(place)) {
            spaceForKittenFound = TRUE;
        } else {
            i = i + 1;
        }
    }
    return i;
}

/**
 * Zapise novou adresarovou polozku do volneho mista
 * @param file Soubor, kam zapisujeme data
 * @param blockNumber Cislo bloku
 * @param data Ukazatel na data k zapisu
 * @param offset Posun v ramci bloku
 * @return Velikost zapsanych dat nebo FAILURE_VALUE
 */
int writeNewKitten(FILE *file, int blockNumber, void *data, int offset) {
    int space = findSpaceForKitten(file, blockNumber, offset);
    int navrat = FAILURE_VALUE;
    if (space != FAILURE_VALUE) {
        fseek(file, (blockNumber * BLOCK_SIZE) + offset + space * sizeof(struct directoryItem), SEEK_SET);
        navrat = fwrite(data, 1, sizeof(struct directoryItem), file);
    }
    return navrat;
} //:3

/**
 * Vypise vsechny polozky v adresari do konzole
 * @param file Soubor, ze ktereho cteme adresar
 * @param blockNumber Cislo bloku
 * @param offset Posun v ramci bloku
 */
void writeAllKittensToConsole(FILE *file, int blockNumber, int offset) {
    writeToConsoleNormal("FILES\n");
    for (int i = 0; i < MAX_NUMBER_OF_SUBDIRECTORIES; i = i + 1) {
        struct directoryItem place;
        readSpace(file, blockNumber, &place, offset, i);
        if (isSpaceEmpty(place) == FALSE && place.isFile != DELETED_ITEM && place.isFile == TRUE) {
            writeToConsoleNormal(place.itemName);
            writeToConsoleNormal(" ");
        }
    }
    writeToConsoleNormal("\n");
    writeToConsoleNormal("FOLDERS\n");
    for (int i = 0; i < MAX_NUMBER_OF_SUBDIRECTORIES; i = i + 1) {
        struct directoryItem place;
        readSpace(file, blockNumber, &place, offset, i);
        if (isSpaceEmpty(place) == FALSE && place.isFile != DELETED_ITEM && place.isFile == FALSE) {
            writeToConsoleNormal(place.itemName);
            writeToConsoleNormal(" ");
        }

    }
}

/**
 * Najde adresarovou polozku podle jmena
 * @param file Soubor, kde hledame
 * @param blockNumber Cislo bloku
 * @param offset Posun v ramci bloku
 * @param name Jmeno souboru/adresare
 * @return Nalezena adresarova polozka nebo prazdna polozka
 */
struct directoryItem findKitten(FILE *file, int blockNumber, int offset, char *name) {
    int spaceForKittenFound = FAILURE_VALUE;
    int i = 0;
    struct directoryItem navrat;
    navrat.startCluster = FALSE;
    while (spaceForKittenFound == FAILURE_VALUE && i < MAX_NUMBER_OF_SUBDIRECTORIES) {
        struct directoryItem place;
        readSpace(file, blockNumber, &place, offset, i);
        if (!strcmp(name, place.itemName)) {
            navrat = place;
            spaceForKittenFound = TRUE;
        } else {
            i = i + 1;
        }
    }
    return navrat;
}

/**
 * Najde index souboru v adresari
 * @param file Soubor, kde hledame
 * @param blockNumber Cislo bloku
 * @param offset Posun v ramci bloku
 * @return Index polozky nebo FAILURE_VALUE
 */
int findIndexOfFile(FILE *file, int blockNumber, int offset) {
    int spaceForKittenFound = FAILURE_VALUE;
    int i = 0;
    while (spaceForKittenFound == FAILURE_VALUE && i < MAX_NUMBER_OF_SUBDIRECTORIES) {
        struct directoryItem place;
        readSpace(file, blockNumber, &place, offset, i);
        if (isSpaceEmpty(place)) {
            spaceForKittenFound = TRUE;
        } else {
            i = i + 1;
        }
    }
    return i;
}

/**
 * Kopiruje soubor z jednoho klastru do druheho
 * @param file Soubor, kde se nachazi data
 * @param fatTable Ukazatel na FAT tabulku
 * @param sizeOfFile Velikost souboru
 * @param offset Posun v ramci bloku
 * @param clusterCountInFat Pocet klastru v FAT
 * @param startCluster Zacatecni klastr
 * @return Index volneho klastru nebo FAILURE_VALUE
 */
int copyFile(FILE *file, int *fatTable, long sizeOfFile, int offset, int clusterCountInFat, int startCluster) {
    int freeCluster = FAILURE_VALUE;
    int freeClusterPrevious = FAILURE_VALUE;
    freeCluster = allocateCluster(fatTable, clusterCountInFat);
    int navrat = freeCluster;
    int size;
    int cluster = startCluster;
    char *data;
    int numberOfClusters = sizeOfFile / BLOCK_SIZE;
    if ((sizeOfFile % BLOCK_SIZE) != 0) {
        numberOfClusters = numberOfClusters + 1;
    }
    for (int i = 0; i < numberOfClusters; i = i + 1) {
        if (sizeOfFile < BLOCK_SIZE) {
            size = sizeOfFile;
        } else {
            size = BLOCK_SIZE;
        }
        data = malloc(sizeof(char) * size);
        if (data == NULL) {
            writeToConsoleError("Couldnt allocate memory");
            return FAILURE_VALUE;
        }
        readDataFromDiskFile(file, cluster, data, size, offset);
        writeDataFromFile(file, freeCluster, data, offset, size);
        free(data);
        if (i >= (numberOfClusters - 1)) {
            fatTable[freeCluster] = FAT_FILE_END;
        } else {
            freeClusterPrevious = freeCluster;
            fatTable[freeClusterPrevious] = 2;
            freeCluster = allocateCluster(fatTable, clusterCountInFat);
            fatTable[freeClusterPrevious] = freeCluster;
            cluster = fatTable[cluster];
        }
    }
    return navrat;
}

/**
 * Ulozi soubor do disku
 * @param file Soubor, kam ukladame
 * @param fileToCopy Soubor, ktery kopirujeme
 * @param fatTable Ukazatel na FAT tabulku
 * @param numberOfClusters Pocet klastru
 * @param sizeOfFile Velikost souboru
 * @param offset Posun v ramci bloku
 * @param clusterCountInFat Pocet klastru v FAT
 * @return Index volneho klastru nebo FAILURE_VALUE
 */
int saveFile(FILE *file, FILE *fileToCopy, int *fatTable, int numberOfClusters, long sizeOfFile, int offset,
             int clusterCountInFat) {
    int freeCluster = FAILURE_VALUE;
    int freeClusterPrevious = FAILURE_VALUE;
    freeCluster = allocateCluster(fatTable, clusterCountInFat);
    int navrat = freeCluster;
    int size;
    char *data;
    for (int i = 0; i < numberOfClusters; i = i + 1) {
        if (sizeOfFile < BLOCK_SIZE) {
            size = sizeOfFile;
        } else {
            size = BLOCK_SIZE;
        }
        data = malloc(sizeof(char) * size);
        if (data == NULL) {
            writeToConsoleError("Couldnt allocate memory");
            return FAILURE_VALUE;
        }
        readDataFromFile(fileToCopy, i, data, size);
        writeDataFromFile(file, freeCluster, data, offset, size);
        free(data);
        if (i >= (numberOfClusters - 1)) {
            fatTable[freeCluster] = FAT_FILE_END;
        } else {
            freeClusterPrevious = freeCluster;
            fatTable[freeClusterPrevious] = 2;
            freeCluster = allocateCluster(fatTable, clusterCountInFat);
            fatTable[freeClusterPrevious] = freeCluster;
        }
        sizeOfFile = sizeOfFile - BLOCK_SIZE;
    }
    return navrat;
}
/**
 * Vynda soubor z disku
 * @param file Soubor, kam ukladame
 * @param fileToCopy Soubor, do ktreho kopirujeme
 * @param fatTable Ukazatel na FAT tabulku
 * @param numberOfClusters Pocet klastru
 * @param sizeOfFile Velikost souboru
 * @param offset Posun v ramci bloku
 * @param clusterCountInFat Pocet klastru v FAT
 * @return SUCCES_VALUE nebo FAILURE_VALUE
 */
int extractFile(FILE *file, FILE *fileToCopy, int *fatTable, long sizeOfFile, int offset, int cluster) {
    int navrat = SUCCES_VALUE;
    int size;
    char *data;
    int numberOfClusters = sizeOfFile / BLOCK_SIZE;
    if ((sizeOfFile % BLOCK_SIZE) != 0) {
        numberOfClusters = numberOfClusters + 1;
    }
    for (int i = 0; i < numberOfClusters; i = i + 1) {
        if (sizeOfFile < BLOCK_SIZE) {
            size = sizeOfFile;
            sizeOfFile = 0;
        } else {
            size = BLOCK_SIZE;
        }
        data = malloc(sizeof(char) * size);
        if (data == NULL) {
            writeToConsoleError("Couldnt allocate memory");
            return FAILURE_VALUE;
        }
        readDataFromDiskFile(file, cluster, data, size, offset);
        writeDataFromFile(fileToCopy, i, data, 0, size);
        free(data);
        cluster = fatTable[cluster];
        sizeOfFile = sizeOfFile - BLOCK_SIZE;
    }

    return navrat;
}

/**
 * Napise soubor do konzole
 * @param file Soubor, kam ukladame
 * @param fileToCopy Soubor, ktery kopirujeme
 * @param fatTable Ukazatel na FAT tabulku
 * @param numberOfClusters Pocet klastru
 * @param sizeOfFile Velikost souboru
 * @param offset Posun v ramci bloku
 * @param clusterCountInFat Pocet klastru v FAT
 * @return SUCCES_VALUE nebo FAILURE_VALUE
 */
int writeFileToConsole(FILE *file, int *fatTable, long sizeOfFile, int offset, int cluster) {
    int navrat = SUCCES_VALUE;
    int size;
    char *data;
    int numberOfClusters = sizeOfFile / BLOCK_SIZE;
    if ((sizeOfFile % BLOCK_SIZE) != 0) {
        numberOfClusters = numberOfClusters + 1;
    }
    if (data == NULL) {
        writeToConsoleError("Couldnt allocate memory");
        return FAILURE_VALUE;
    }
    for (int i = 0; i < numberOfClusters; i = i + 1) {
        if (sizeOfFile < BLOCK_SIZE) {
            size = sizeOfFile;
            sizeOfFile = 0;
        } else {
            size = BLOCK_SIZE;
        }
        data = malloc(sizeof(char) * (size + 1));
        memset(data, '\0', size + 1);
        if (data == NULL) {
            writeToConsoleError("Couldnt allocate memory");
            return FAILURE_VALUE;
        }
        readDataFromDiskFile(file, cluster, data, size, offset);
        writeToConsoleNormal(data);
        free(data);
        cluster = fatTable[cluster];
        sizeOfFile = sizeOfFile - BLOCK_SIZE;
    }

    return navrat;
}

//////////////////////////////////

/**
 * Smaze slozku podle zadaneho nazvu
 * @param file Soubor, kam ukladame
 * @param fatTable Ukazatel na FAT tabulku
 * @param numberOfClusters Pocet klastru
 * @param sizeOfFile Velikost souboru
 * @param offset Posun v ramci bloku
 * @param clusterCountInFat Pocet klastru v FAT
 * @return TRUE jestli smazane, nebo CLUSTER_NOT_EMPTY jestli je slozka neprazna
 */
int removeDirectoryByName(FILE *file, char *name, int cluster, int offset) {
    fseek(file, 0, SEEK_SET);
    fseek(file, offset + cluster * BLOCK_SIZE, SEEK_SET);
    int i = 0;
    int navrat = TRUE;
    while (navrat == TRUE && i < MAX_NUMBER_OF_SUBDIRECTORIES) {
        struct directoryItem item;
        readSpace(file, cluster, &item, offset, i);
        if (!strcmp(item.itemName, name)) {
            int clusterOkToDelete = isClusterEmpty(file, item.startCluster, offset);
            if (clusterOkToDelete == TRUE) {
                struct directoryItem itemRewrite;
                itemRewrite.isFile = DELETED_ITEM;
                removeDirectory(file, cluster, &itemRewrite, offset, i);
                navrat = item.startCluster;
            } else {
                navrat = CLUSTER_NOT_EMPTY;
            }
        } else {
            i = i + 1;
        }
    }
    return navrat;
}

/**
 * Smaze soubor podle zadaneho nazvu
 * @param file Soubor, kam ukladame
 * @param fatTable Ukazatel na FAT tabulku
 * @param numberOfClusters Pocet klastru
 * @param sizeOfFile Velikost souboru
 * @param offset Posun v ramci bloku
 * @param clusterCountInFat Pocet klastru v FAT
 * @return item.startcluster jestli smazane, nebo CLUSTER_NOT_EMPTY jestli je slozka neprazna
 */
int removeFileByName(FILE *file, char *name, int cluster, int offset) {
    fseek(file, 0, SEEK_SET);
    fseek(file, offset + cluster * BLOCK_SIZE, SEEK_SET);
    int i = 0;
    int navrat = FAILURE_VALUE;
    while (navrat == FAILURE_VALUE && i < MAX_NUMBER_OF_SUBDIRECTORIES) {
        struct directoryItem item;
        readSpace(file, cluster, &item, offset, i);
        if (!strcmp(item.itemName, name)) {
            struct directoryItem itemRewrite;
            itemRewrite.isFile = DELETED_ITEM;
            removeDirectory(file, cluster, &itemRewrite, offset, i);
            navrat = item.startCluster;
        } else {
            i = i + 1;
        }
    }
    return navrat;
}

/**
 * Najde cluster hledaneho souboru/slozky
 * @param file Soubor, kam ukladame
 * @param name Nazev souboru/slozky
 * @param cluster cluster slozky ve ktere soubor/slozka je
 * @param offset zacatek dat v disku
 * @return SUCCES_VALUE nebo FAILURE_VALUE
 */
int findKittenCluser(FILE *file, char *name, int cluster, int offset) {
    fseek(file, 0, SEEK_SET);
    fseek(file, offset + cluster * BLOCK_SIZE, SEEK_SET);
    int found = FALSE;
    int i = 0;
    int navrat = FAILURE_VALUE;
    while (found == FALSE && i < MAX_NUMBER_OF_SUBDIRECTORIES) {
        struct directoryItem kitten;
        readSpace(file, cluster, &kitten, offset, i);\
        if (!strcmp(kitten.itemName, name)) {
            navrat = kitten.startCluster;
            found = TRUE;
        } else {
            i = i + 1;
        }
    }
    return navrat;
}