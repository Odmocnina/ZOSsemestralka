//
// Created by Michael on 09.10.2024.
//

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "commands.h"
#include "constants.h"
#include "memoryDisk.h"
#include "FAT.h"
#include "runningOfProgram.h"
#include "pathManager.h"

/**
 * Funkce pro zpracovani spatneho prikazu.
 * @param command - retezec prikazu, ktery byl zadany uzivatelem.
 */
void wrongCommand(char *command) {
    writeToConsoleError("Uknown command: ");
    writeToConsoleNormal(command);
    writeToConsoleNormal("\n");
}

/**
 * Funkce pro zpracovani help prikazu.
 * @param command - retezec prikazu, ktery byl zadany uzivatelem.
 */
void help() {
    writeToConsoleNormal("Cesta k soubrou je zadana formou s1/s2/s3/s4/s5.../soubor\n");
    writeToConsoleNormal("Od rootu se cesta zadava zacinajici /\n");
    writeToConsoleNormal("mkdir - vytvoreni nove slozky (nesmi stejne jmena "
                         "slozka/soubor jiz existovat v slozce)\n");
    writeToConsoleNormal("rmdir - smazani slozky (musi byt prazna)\n");
    writeToConsoleNormal("cd - zmeni aktualni slozku\n");
    writeToConsoleNormal("incp - zkopirovani souboru z pocitace do souboroveho systemu "
                         "(musi se vejit samozrejme, a nesmi stejne jmeny soubor v slozce existovat)\n");
    writeToConsoleNormal("outcp - vyndani souboru ze souborveho systemu do souboru na disku\n");
    writeToConsoleNormal("cp - zkopiruje soubor\n");
    writeToConsoleNormal("rm - odstraneni soubru ze souborveho systemu\n");
    writeToConsoleNormal("info - vypsani na kterych clusterech je soubor ulozen\n");
    writeToConsoleNormal("ls - vypsani obsahu slozky\n");
    writeToConsoleNormal("mv - premisti/prejmenuje soubor\n");
    writeToConsoleNormal("pwd - vypise aktualni cestu\n");
    writeToConsoleNormal("cat - vypise obsah souboru\n");
    writeToConsoleNormal("load - nacte soubor s prikazama a prikazy provede\n");
    writeToConsoleNormal("bug - poskodi soubor, coz poskodi cely souboruvy system\n");
    writeToConsoleNormal("check - zkontroluje jestli souborvy system neni poskozen\n");
}

/**
 * Funkce pro kopirovani souboru.
 * @param arg1 - zdrojovy soubor.
 * @param arg2 - cilovy soubor.
 * @param fileName - nazev souboru, ktery obsahuje FAT.
 * @param fatTable - tabulka FAT.
 * @param currentCluster - aktualni cluster.
 * @return - navratova hodnota urcujici uspesnost operace.
 */
int cp(char *arg1, char *arg2, const char *fileName, int *fatTable, int currentCluster) {
    if (arg1 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    if (arg2 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    int clusterOriginal = FAILURE_VALUE;                  //hledani prvniho argumentu, jestli vec vubec je
    char *nameOriginal = circumsisePath(arg1);
    if (nameOriginal != NULL) {// Tahle skurvena prasarna se snazi vyresit aby metoda na obrezani cesty neposrala zpusob jak hledam cestu
        clusterOriginal = findPathInDisk(file, arg1, currentCluster, FALSE);
    } else {
        nameOriginal = strdup(arg1);
        clusterOriginal = currentCluster;
    }

    if (clusterOriginal == FAILURE_VALUE) {
        free(nameOriginal);
        fclose(file);
        return FAILURE_VALUE;
    }

    int clusterNew = FAILURE_VALUE;                  //hledani druheho argumentu, jestli vec vubec je
    char *nameNew = circumsisePath(arg2);
    if (nameNew != NULL) {// Tahle skurvena prasarna se snazi vyresit aby metoda na obrezani cesty neposrala zpusob jak hledam cestu
        clusterNew = findPathInDisk(file, arg2, currentCluster, FALSE);
    } else {
        nameNew = strdup(arg2);
        clusterNew = currentCluster;
    }

    if (clusterNew == FAILURE_VALUE) {
        free(nameOriginal);
        free(nameNew);
        fclose(file);
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);
    // Přečteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    int offset = FATdescription.dataStartAddress;
    int navrat = FAILURE_VALUE;

    struct directoryItem item = findKitten(file, clusterOriginal, offset, nameOriginal);

    if (item.startCluster == FALSE) {
        writeToConsoleError("File not found");
        writeToConsoleNormal("\n");
        free(nameOriginal);
        free(nameNew);
        fclose(file);
        return FAILURE_VALUE;
    }

    struct directoryItem itemSameName = findKitten(file, clusterNew, offset, nameNew);
    if (itemSameName.startCluster != FALSE) {
        writeToConsoleError("File with same name already in directory");
        writeToConsoleNormal("\n");
        free(nameOriginal);
        free(nameNew);
        fclose(file);
        return FAILURE_VALUE;
    }

    int freeClusterAmount = getNumberOfFreeClusters(fatTable, FATdescription.clusterCount);

    if (freeClusterAmount >= ((item.size / BLOCK_SIZE) + 1)) {
        struct directoryItem itemNew;
        createDirectoryItem(&itemNew, nameNew, item.isFile,
                            allocateCluster(fatTable, FATdescription.clusterCount), item.size);
        navrat = writeNewKitten(file, clusterNew, &itemNew, offset);

        if (navrat != FAILURE_VALUE) {
            navrat = copyFile(file, fatTable, item.size, offset, FATdescription.clusterCount,
                              item.startCluster);
            writeFatTable(file, fatTable);
        }
    } else {
        writeToConsoleError("Not enough space");
        writeToConsoleNormal("\n");
    }

    free(nameOriginal);
    free(nameNew);
    fclose(file);
    return navrat;
}

/**
 * Funkce pro presun souboru.
 * @param arg1 - zdrojovy soubor.
 * @param arg2 - cilovy soubor.
 * @param fileName - nazev souboru, ktery obsahuje FAT.
 * @param currentCluster - aktualni cluster.
 * @return - navratova hodnota urcujici uspesnost operace.
 */
int mv(char *arg1, char *arg2, const char *fileName, int currentCluster) {
    if (arg1 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    if (arg2 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    int clusterOriginal = FAILURE_VALUE;                  //hledani prvniho argumentu, jestli vec vubec je
    char *nameOriginal = circumsisePath(arg1);
    if (nameOriginal != NULL) {// Tahle skurvena prasarna se snazi vyresit aby metoda na obrezani cesty neposrala zpusob jak hledam cestu
        clusterOriginal = findPathInDisk(file, arg1, currentCluster, FALSE);
    } else {
        nameOriginal = strdup(arg1);
        clusterOriginal = currentCluster;
    }

    if (clusterOriginal == FAILURE_VALUE) {
        fclose(file);
        free(nameOriginal);
        return FAILURE_VALUE;
    }

    int clusterNew = FAILURE_VALUE;                  //hledani druheho argumentu, jestli vec vubec je
    char *nameNew = circumsisePath(arg2);
    if (nameNew != NULL) {// Tahle skurvena prasarna se snazi vyresit aby metoda na obrezani cesty neposrala zpusob jak hledam cestu
        clusterNew = findPathInDisk(file, arg2, currentCluster, FALSE);
    } else {
        nameNew = strdup(arg2);
        clusterNew = currentCluster;
    }

    if (clusterNew == FAILURE_VALUE) {
        free(nameOriginal);
        free(nameNew);
        fclose(file);
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);
    // Přečteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    int offset = FATdescription.dataStartAddress;
    int navrat = FAILURE_VALUE;

    struct directoryItem item = findKitten(file, clusterOriginal, offset, nameOriginal);

    if (item.startCluster == FALSE) {
        writeToConsoleError("File not found");
        writeToConsoleNormal("\n");
        free(nameOriginal);
        free(nameNew);
        fclose(file);
        return FAILURE_VALUE;
    }

    struct directoryItem itemSameName = findKitten(file, clusterNew, offset, nameNew);
    if (itemSameName.startCluster != FALSE) {
        writeToConsoleError("File/folder with same name already in directory");
        writeToConsoleNormal("\n");
        free(nameOriginal);
        free(nameNew);
        fclose(file);
        return FAILURE_VALUE;
    }

    struct directoryItem itemNew;
    createDirectoryItem(&itemNew, nameNew, item.isFile, item.startCluster, item.size);
    navrat = writeNewKitten(file, clusterNew, &itemNew, offset);

    if (navrat != FAILURE_VALUE) {
        removeFileByName(file, nameOriginal, clusterOriginal, offset);
    }

    free(nameOriginal);
    free(nameNew);
    fclose(file);
    return navrat;
}

/**
 * Funkce pro formatovani disku.
 * @param arg1 - velikost disku.
 * @param fileName - nazev souboru pro disky.
 * @param fatTable - ukazatel na tabulku FAT.
 * @return - navratova hodnota urcujici uspesnost operace.
 */
int format(char *arg1,const char *fileName, int32_t **fatTable) {
    if (arg1 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }
    arg1 = trim(arg1);
    if (strcmp(arg1, "") == TRUE) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    if (fileExists(fileName)) {
        remove(fileName);
    }

    int result =  diskCreation(fileName, arg1);

    if (result == FAILURE_VALUE) {
        writeToConsoleError("Disk creation failed");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    // otevri pro ceteni
    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    // vytvor siganturu
    struct description FATdescription;
    strncpy(FATdescription.signature, "Misa:3", sizeof(FATdescription.signature) - 1);
    FATdescription.signature[sizeof(FATdescription.signature) - 1] = '\0';              // zapsani signatury
    FATdescription.diskSize = getSize(arg1);  // Use your getSize function to calculate size
    FATdescription.clusterSize = BLOCK_SIZE;  // Set the cluster size (same as block size)
    FATdescription.clusterCount = (FATdescription.diskSize / BLOCK_SIZE) + 1;
    FATdescription.fat1StartAddress = sizeof(struct description);  // FAT1 starts right after the description
    FATdescription.clusterSize = BLOCK_SIZE;
    FATdescription.dataStartAddress = FATdescription.fat1StartAddress + FATdescription.clusterCount * sizeof(int32_t);

    // Zapsani popisu
    fwrite(&FATdescription, 1, sizeof(FATdescription), file);

    // inicializuj FAT tabulku
    *fatTable = (int32_t *)malloc(FATdescription.clusterCount * sizeof(int32_t));
    inicilizeFAT(*fatTable, FATdescription.clusterCount);
    *fatTable[0] = 0; //tady musi bejt * abz to ukayovalo na to pole

    makeSpaceForSmallKittens();
    fclose(file);

    return SUCCES_VALUE;
}

/**
 * Funkce pro vytvoreni noveho adresare.
 * @param arg1 - nazev adresare.
 * @param fileName - nazev souboru, ktery obsahuje FAT.
 * @param fatTable - tabulka FAT.
 * @param currentCluster - aktualni cluster.
 * @return - navratova hodnota urcujici uspesnost operace.
 */
int mkdir(char *arg1, const char *fileName, int *fatTable, int currentCluster) {
    if (arg1 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }
    if (strcmp(arg1, "") == TRUE) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    int numberOfFolders = countSlashes(arg1);
    if (numberOfFolders >= MAXIMUM_DEAPTH) {
        fclose(file);
        writeToConsoleError("Folder too deap");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);

    // Přečteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);

    int fatSize = FATdescription.clusterCount;
    int cluster = FAILURE_VALUE;

    char *name = circumsisePath(arg1);
    if (name != NULL) {// Tahle skurvena prasarna se snazi vyresit aby metoda na obrezani cesty neposrala zpusob jak hledam cestu
        cluster = findPathInDisk(file, arg1, currentCluster, FALSE);
    } else {
        name = strdup(arg1);
        cluster = currentCluster;
    }

    if (cluster == FAILURE_VALUE) {
        fclose(file);
        free(name);
        return FAILURE_VALUE;
    }

    int freeIndex = allocateCluster(fatTable, fatSize);
    if (freeIndex == FAT_TABLE_FULL) {
        writeToConsoleError("No free cluster");
        writeToConsoleNormal("\n");
    } else {
        int offset = FATdescription.dataStartAddress;
        struct directoryItem item = findKitten(file, cluster, offset, name);
        if (item.startCluster == FALSE) {
            struct directoryItem kitten;

            createDirectoryItem(&kitten, name, FALSE, CLUSTER_IS_FOLDER, 0);
            fatTable[freeIndex] = CLUSTER_IS_FOLDER;
            writeNewKitten(file, cluster, &kitten, offset);
            writeFatTable(file, fatTable);
        } else {
            writeToConsoleError("Directory wih same name already in directory");
            writeToConsoleNormal("\n");
        }
    }
    free(name);
    fclose(file); //halelujah

    return SUCCES_VALUE;
}

/**
 * Funkce pro smazani adresare.
 * @param arg1 - nazev adresare k odstraneni.
 * @param fileName - nazev souboru, ktery obsahuje FAT.
 * @param fatTable - tabulka FAT.
 * @param currentCluster - aktualni cluster.
 * @return - navratova hodnota urcujici uspesnost operace.
 */
int rmdir(char *arg1, const char *fileName, int *fatTable, int currentCluster) {
    if (arg1 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }
    if (strcmp(arg1, "") == TRUE) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);

    // Precteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);

    int cluster = currentCluster;

    char *name = circumsisePath(arg1);
    if (name != NULL) {
        cluster = findPathInDisk(file, arg1, currentCluster, FALSE);
    } else {
        name = strdup(arg1);
        cluster = currentCluster;
    }

    if (cluster == FAILURE_VALUE) {
        free(name);
        fclose(file);
        return FAILURE_VALUE;
    }

    int offset = FATdescription.dataStartAddress;
    int result = removeDirectoryByName(file, name, cluster, offset);
    if (result == FAILURE_VALUE) {
        writeToConsoleError("Path not found");
        writeToConsoleNormal("\n");
    } else if (result == CLUSTER_NOT_EMPTY) {
        writeToConsoleError("Cluster not empty");
        writeToConsoleNormal("\n");
    } else {
        writeFatTable(file, fatTable);
        fatTable[result] = FAT_UNUSED;
    }
    free(name);
    fclose(file); //halelujah

    return SUCCES_VALUE;
}

/**
 * Funkce pro vypis obsahu adresare.
 * @param fileName - nazev souboru, ktery obsahuje FAT.
 * @param currentCluster - aktualni cluster.
 * @param args1 - dodatecne argumenty pro filtraci.
 * @return - navratova hodnota urcujici uspesnost operace.
 */
int ls(const char *fileName, int currentCluster, char *args1) {
    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);

    // Precteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    int offset = FATdescription.dataStartAddress;

    int cluster = currentCluster;
    if (args1 != NULL) {
        cluster = findPathInDisk(file, args1, currentCluster, FALSE);
    }

    if (cluster != FAILURE_VALUE) {
        writeAllKittensToConsole(file, cluster, offset);
    }

    fclose(file); //halelujah

    return SUCCES_VALUE;
}

/**
 * Funkce pro zmenu aktualniho adresare.
 * @param arg1 - nazev noveho adresare.
 * @param fileName - nazev souboru, ktery obsahuje FAT.
 * @param currentCluster - ukazatel na aktualni cluster.
 * @return - navratova hodnota urcujici uspesnost operace.
 */
int cd(char *arg1, const char *fileName, int currentCluster) {
    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }
    int navrat = FAILURE_VALUE;
    navrat = findPathInDisk(file, arg1, currentCluster, TRUE);
    if (navrat != FAILURE_VALUE) {
        currentCluster = navrat;
    }

    return navrat;
}

/**
 * Funkce pro vypis aktualni cesty.
 *
 * @param fileName - nazev souboru, pro kontrolu jestli je disk naformatovan
 */
void pwd(const char *fileName) {
    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return;
    }
    fclose(file);
    writePathCommand();
}

/**
 * Funkce pro kopirovani souboru z jednoho umisteni do druhe.
 * @param arg1 - zdrojovy soubor.
 * @param arg2 - cilovy soubor.
 * @param fileName - nazev souboru, ktery obsahuje FAT.
 * @param fatTable - tabulka FAT.
 * @param currentCluster - aktualni cluster.
 * @return - navratova hodnota urcujici uspesnost operace.
 */
int incp(char *arg1, char *arg2, const char *fileName, int *fatTable, int currentCluster) {
    if (arg1 == NULL) {   // zkontrolovani jestli byli zadany 2 parametry
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    if (arg2 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *fileToCopy = fopen(arg1, "r+b");
    if (fileToCopy == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        fclose(file);
        return FAILURE_VALUE;
    }

    int cluster = FAILURE_VALUE;
    char *name = circumsisePath(arg2);
    if (name != NULL) {// Tahle skurvena prasarna se snazi vyresit aby metoda na obrezani cesty neposrala zpusob jak hledam cestu
        cluster = findPathInDisk(file, arg2, currentCluster, FALSE);
    } else {
        name = strdup(arg2);
        cluster = currentCluster;
    }

    if (cluster == FAILURE_VALUE) {
        free(name);
        fclose(fileToCopy);
        fclose(file);
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);
    // Precteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    int offset = FATdescription.dataStartAddress;
    int numberOfFreeClusters = getNumberOfFreeClusters(fatTable, FATdescription.clusterCount);
    int navrat = FAILURE_VALUE;

    fseek(fileToCopy, 0, SEEK_END);
    long sizeOfFile = ftell(fileToCopy);
    fseek(fileToCopy, 0, SEEK_SET);  // Vrat ukazatel zpet na zacatek souboru
    int numberOfClusters = sizeOfFile / BLOCK_SIZE;
    if ((sizeOfFile % BLOCK_SIZE) != 0) {
        numberOfClusters = numberOfClusters + 1;
    }

    struct directoryItem item = findKitten(file, cluster, offset, name);
    if (item.startCluster != FALSE) {
        writeToConsoleError("File/folder with same name alredy in directory");
        writeToConsoleNormal("\n");
        free(name);
        fclose(fileToCopy);
        fclose(file);
        return FAILURE_VALUE;
    }

    if (numberOfFreeClusters < numberOfClusters) {
        writeToConsoleError("Not enough memory");
        writeToConsoleNormal("\n");
    } else {
        navrat = saveFile(file, fileToCopy, fatTable, numberOfClusters, sizeOfFile, offset, FATdescription.clusterCount);
        //navrat znaci v kterem clusteru zacina soubor, pokud je FAILURE_VALUE, neco se posralo
        if (navrat != FAILURE_VALUE) {
            struct directoryItem kitten;
            createDirectoryItem(&kitten, name, TRUE, navrat, sizeOfFile);
            writeNewKitten(file, cluster, &kitten, offset);
            writeFatTable(file, fatTable);
            //writeBlock(file, freeCluster, data, offset);§
        }
    }

    free(name);
    fclose(fileToCopy);
    fclose(file);
    return navrat;
}

/**
 * Funkce pro export souboru do urciteho umisteni.
 * @param arg1 - nazev souboru k exportu.
 * @param arg2 - nazev souboru pro export.
 * @param fileName - nazev souboru, ktery obsahuje FAT.
 * @param fatTable - tabulka FAT.
 * @param currentCluster - aktualni cluster.
 * @return - navratova hodnota urcujici uspesnost operace.
 */
int outcp(char *arg1, char *arg2, const char *fileName, int *fatTable, int currentCluster) {
    if (arg1 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    if (arg2 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *fileToCopy = fopen(arg2, "wb");
    if (fileToCopy == NULL) {
        fclose(file);
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    int cluster = FAILURE_VALUE;
    char *name = circumsisePath(arg1);
    if (name != NULL) {// Tahle skurvena prasarna se snazi vyresit aby metoda na obrezani cesty neposrala zpusob jak hledam cestu
        cluster = findPathInDisk(file, arg1, currentCluster, FALSE);
    } else {
        name = strdup(arg1);
        cluster = currentCluster;
    }

    if (cluster == FAILURE_VALUE) {
        fclose(fileToCopy);
        fclose(file);
        free(name);
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);
    // Precteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    int offset = FATdescription.dataStartAddress;
    int navrat = FAILURE_VALUE;

    struct directoryItem item = findKitten(file, cluster, offset, name);
    int clusterOfFile = item.startCluster;
    int size = item.size;

    if (clusterOfFile == FALSE) {
        free(name);
        fclose(file);
        fclose(fileToCopy);
        writeToConsoleError("File not found");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    navrat = extractFile(file, fileToCopy, fatTable, size, offset, clusterOfFile);

    free(name);
    fclose(fileToCopy);
    fclose(file);
    return navrat;
}

/**
 * Odstrani soubor z disku.
 *
 * @param arg1 Jmeno souboru k odstraneni.
 * @param fileName Nazev souboru s FAT tabulkou.
 * @param fatTable Ukazatel na tabulku FAT.
 * @param currentCluster Ukazatel na aktualni cluster.
 * @return Vraci SUCCESS_VALUE pri uspesnem odstraneni, jinak FAILURE_VALUE.
 */
int rm(char *arg1, const char *fileName, int *fatTable, int currentCluster) {
    if (arg1 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    int cluster = FAILURE_VALUE;
    char *name = circumsisePath(arg1);
    if (name != NULL) {// Tahle skurvena prasarna se snazi vyresit aby metoda na obrezani cesty neposrala zpusob jak hledam cestu
        cluster = findPathInDisk(file, arg1, currentCluster, FALSE);
    } else {
        name = strdup(arg1);
        cluster = currentCluster;
    }

    if (cluster == FAILURE_VALUE) {
        fclose(file);
        free(name);
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);
    // Precteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    int offset = FATdescription.dataStartAddress;
    int navrat = FAILURE_VALUE;

    struct directoryItem item = findKitten(file, cluster, offset, name);
    int clusterOfFile = item.startCluster;

    if (clusterOfFile == FALSE) {
        free(name);
        fclose(file);
        writeToConsoleError("File not found");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    clearFile(file, clusterOfFile, fatTable, offset);   //vymazani dat z disku

    int result = removeFileByName(file, name, cluster, offset);   //vymazani zaznamu o soubrou
    if (result == FAILURE_VALUE) {
        writeToConsoleError("Path not found");
        writeToConsoleNormal("\n");
    } else {
        clearFATTableFile(fatTable, result);               //vymazani odkazu z fat tabulky
        writeFatTable(file, fatTable);
    }

    free(name);
    fclose(file);
    return navrat;
}

/**
 * Zobrazi informace o souboru.
 *
 * @param arg1 Jmeno souboru, jehoz informace se maji zobrazit.
 * @param fileName Nazev souboru s FAT tabulkou.
 * @param fatTable Ukazatel na tabulku FAT.
 * @param currentCluster Ukazatel na aktualni cluster.
 * @return Vraci SUCCESS_VALUE pri uspesnem zobrazeni, jinak FAILURE_VALUE.
 */
int info(char *arg1, const char *fileName, int *fatTable, int currentCluster) {
    if (arg1 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    int cluster = FAILURE_VALUE;
    char *name = circumsisePath(arg1);
    if (name != NULL) {// Tahle skurvena prasarna se snazi vyresit aby metoda na obrezani cesty neposrala zpusob jak hledam cestu
        cluster = findPathInDisk(file, arg1, currentCluster, FALSE);
    } else {
        name = strdup(arg1);
        cluster = currentCluster;
    }

    if (cluster == FAILURE_VALUE) {
        fclose(file);
        free(name);
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);
    // Precteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    int offset = FATdescription.dataStartAddress;
    int navrat = FAILURE_VALUE;

    struct directoryItem item = findKitten(file, cluster, offset, name);
    int clusterOfFile = item.startCluster;

    if (clusterOfFile == FALSE) {
        free(name);
        writeToConsoleError("File not found");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    if (clusterOfFile == CLUSTER_IS_FOLDER) {
        free(name);
        writeToConsoleError("Folder lol");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    printClustersFile(fatTable, clusterOfFile);

    free(name);
    fclose(file);
    return navrat;
}

/**
 * Vypisuje obsah souboru na konzoli.
 *
 * @param arg1 Jmeno souboru, jehoz obsah se ma vypsat.
 * @param fileName Nazev souboru s FAT tabulkou.
 * @param fatTable Ukazatel na tabulku FAT.
 * @param currentCluster Ukazatel na aktualni cluster.
 * @return Vraci SUCCESS_VALUE pri uspesnem vypisu, jinak FAILURE_VALUE.
 */
int cat(char *arg1, const char *fileName, int *fatTable, int currentCluster) {
    if (arg1 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    int cluster = FAILURE_VALUE;
    char *name = circumsisePath(arg1);
    if (name != NULL) {// Tahle skurvena prasarna se snazi vyresit aby metoda na obrezani cesty neposrala zpusob jak hledam cestu
        cluster = findPathInDisk(file, arg1, currentCluster, FALSE);
    } else {
        name = strdup(arg1);
        cluster = currentCluster;
    }

    if (cluster == FAILURE_VALUE) {
        free(name);
        fclose(file);
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);
    // Precteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    int offset = FATdescription.dataStartAddress;
    int navrat = FAILURE_VALUE;

    struct directoryItem item = findKitten(file, cluster, offset, name);
    int clusterOfFile = item.startCluster;
    int size = item.size;

    if (clusterOfFile == FALSE) {
        fclose(file);
        free(name);
        writeToConsoleError("File not found");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }
    navrat = writeFileToConsole(file, fatTable, size, offset, clusterOfFile);

    free(name);
    fclose(file);
    return navrat;
}

/**
 * Nacte prikazy ze souboru.
 *
 * @param arg1 Nazev souboru s prikazy k nacteni.
 * @param fileName Nazev souboru s FAT tabulkou.
 * @param currentCluster Ukazatel na aktualni cluster.
 * @param needToLoadFatTable Ukazuje, zda je potreba nactit FAT tabulku.
 */
int load(char *arg1, const char *fileName, int currentCluster, int needToLoadFatTable) {
    if (arg1 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *loadFile = fopen(arg1, "r+b");
    if (loadFile == NULL) {
        writeToConsoleError("Error opening load file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    int navrat = processCommandsFromFile(fileName, needToLoadFatTable, loadFile, currentCluster);

    fclose(loadFile);

    return navrat;
}

/**
 * Oznaci soubor jako "chybovy" v tabulce FAT.
 *
 * @param arg1 Jmeno souboru, ktery ma byt oznacen jako chybovy.
 * @param fileName Nazev souboru s FAT tabulkou.
 * @param fatTable Ukazatel na tabulku FAT.
 * @param currentCluster Ukazatel na aktualni cluster.
 * @return Vraci SUCCESS_VALUE pri uspesnem oznaceni, jinak FAILURE_VALUE.
 */
int bug(char *arg1, const char *fileName, int *fatTable, int currentCluster) {
    if (arg1 == NULL) {
        writeToConsoleError("Wrong number of arguments");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    int cluster = FAILURE_VALUE;
    char *name = circumsisePath(arg1);
    if (name != NULL) {// Tahle skurvena prasarna se snazi vyresit aby metoda na obrezani cesty neposrala zpusob jak hledam cestu
        cluster = findPathInDisk(file, arg1, currentCluster, FALSE);
    } else {
        name = strdup(arg1);
        cluster = currentCluster;
    }

    if (cluster == FAILURE_VALUE) {
        fclose(file);
        free(name);
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);
    // Precteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    int offset = FATdescription.dataStartAddress;
    int navrat = SUCCES_VALUE;

    struct directoryItem item = findKitten(file, cluster, offset, name);
    int clusterOfFile = item.startCluster;

    if (clusterOfFile == FALSE) {
        free(name);
        fclose(file);
        writeToConsoleError("File not found");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    fatTable[clusterOfFile] = FAT_BAD_CLUSTER;
    writeFatTable(file, fatTable);

    free(name);
    fclose(file);
    return navrat;
}

/**
 * Kontroluje integritu tabulky FAT.
 *
 * @param fileName Nazev souboru s FAT tabulkou.
 * @param fatTable Ukazatel na tabulku FAT.
 * @return Vraci TRUE, pokud je vse v poradku, jinak FALSE.
 */
int check(const char *fileName, int *fatTable) {
    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
        writeToConsoleNormal("\n");
        return FAILURE_VALUE;
    }

    struct description FATdescription;

    fseek(file, 0, SEEK_SET);
    // Precteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);

    for (int i = 0; i < FATdescription.clusterCount; i = i + 1) {
        if (fatTable[i] == FAT_BAD_CLUSTER) {
            return FALSE;
        }
    }
    return TRUE;
}




