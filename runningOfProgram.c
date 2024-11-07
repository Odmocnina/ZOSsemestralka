//
// Created by Michael on 09.10.2024.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "constants.h"
#include "commands.h"
#include "runningOfProgram.h"
#include "memoryDisk.h"
#include "pathManager.h"

/*
 * funkce, co zjisti jestli je nutno naformatovat disk, nebo je jiz disk (.dat soubor) existujici a muze se pracovat s
 * nim
 * @param fileName - nazev .dat souboru
 * @return TRUE (1), nebo FALSE (0) podle toho jestli soubor je nutno naformatovat
 **/
int toFormatOrNotThatIsTheQuestion(const char *fileName) {
    if (fileName == NULL) {
        return FAILURE_VALUE;
    }
    if (fileExists(fileName) == FALSE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * Procedura co nacte fatatbulku z jiz existujiciho souboru
 * @param fileName - nazev existujicho .dat souboru
 * @param fatTable - ukazatel na misto, kam ma byt nactena tabulka
 **/
void loadFatTable(const char *fileName, int32_t **fatTable) {
    FILE *file = fopen(fileName, "r+b");
    if (file == NULL) {
        writeToConsoleError("Error opening file");
    }

    struct description FATdescription;
    fseek(file, 0, SEEK_SET);
    // Precteme blok dat do bufferu
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    *fatTable = (int32_t *)malloc(FATdescription.clusterCount * sizeof(int32_t));
    if (*fatTable == NULL) {
        writeToConsoleError("Memory allocation failed");
        fclose(file);
        return;
    }
    // Prejdeme na zacatek oblasti, kde je ulozena FAT tabulka
    fseek(file, sizeof(FATdescription), SEEK_SET);
    // Nacteme FAT tabulku ze souboru do pameti
    fread(*fatTable, sizeof(int32_t), FATdescription.clusterCount, file);
    fclose(file);
    makeSpaceForSmallKittens();
}

/** fat tabulka udrzujici v sobe inforce o souborech a slozkach ulozene v .dat souboru **/
int32_t *fatTable = NULL;
/** promena urcujici jestli je .dat soubor v poradku **/
int isGood = TRUE;
/** promena urcujci jestli program beyi nebo ne **/
int isRunning = TRUE;

/*
 * funkce, co zpracovava prikazy od uzivatele, vola podporgramy jednotlivych prikazu a dava jim parametry
 * @param fileName - nazev .dat souboru
 * @param needToLoadFatTable - parametr urcujici jestli je nutno nacist fatatblulku z existujiciho .dat souboru
 **/
void processCommands(const char *fileName, int needToLoadFatTable) {
    char input[LENGTH_OF_USER_INPUT];

    int currentDirectory = -1;

    if (needToLoadFatTable == TRUE) {
        loadFatTable(fileName, &fatTable);
        currentDirectory = currentDirectory + 1;
        isGood = check(fileName, fatTable);
        if (isGood) {
            writeToConsoleNormal("File system is OK");
        } else {
            writeToConsoleNormal(
                    "File system is currupted, only format, help, exit and check commands can be used");
        }
        needToLoadFatTable = FALSE;
    }

    while (isRunning == TRUE) {
        writePathStackCommand();
        fgets(input, sizeof(input), stdin);  // Nacteme vstup od uzivatele
        input[strcspn(input, "\n")] = 0;  // Odstranení nového radku

        // Rozdelime vstup na prikaz a argumenty
        char *command = strtok(input, " ");
        char *arg1 = strtok(NULL, " ");
        char *arg2 = strtok(NULL, " ");

        // Porovname prikaz s pouzitim switch
        if (command != NULL) {
            if (strncmp(command, "cp", 2) == 0 && isGood) {
                cp(arg1, arg2, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "cat", 3) == 0 && isGood) {
                cat(arg1, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "cd", 2) == 0 && isGood) {
                int newDirectory = cd(arg1, fileName, currentDirectory);
                if (newDirectory != FAILURE_VALUE) {
                    currentDirectory = newDirectory;
                }
            } else if (strncmp(command, "mv", 2) == 0 && isGood) {
                mv(arg1, arg2, fileName, currentDirectory);
            } else if (strncmp(command, "mkdir", 5) == 0 && isGood) {
                mkdir(arg1, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "rmdir", 5) == 0 && isGood) {
                rmdir(arg1, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "rm", 2) == 0 && isGood) {
                rm(arg1, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "ls", 2) == 0 && isGood) {
                ls(fileName, currentDirectory, arg1);
            } else if (strncmp(command, "load", 4) == 0) {
                int newDirectory = load(arg1, fileName, currentDirectory, needToLoadFatTable);
                if (newDirectory != FAILURE_VALUE) {
                    currentDirectory = newDirectory;
                }
            } else if (strncmp(command, "outcp", 5) == 0 && isGood) {
                outcp(arg1, arg2, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "info", 4) == 0 && isGood) {
                info(arg1, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "incp", 4) == 0 && isGood) {
                incp(arg1, arg2, fileName, fatTable, currentDirectory);
            }  else if (strncmp(command, "pwd", 3) == 0 && isGood) {
                pwd(fileName);
            } else if (strncmp(command, "bug", 4) == 0 && isGood) {
                int result = bug(arg1, fileName, fatTable, currentDirectory);
                if (result == SUCCES_VALUE) {
                    isGood = FALSE;
                }
            } else if (strncmp(command, "format", 6) == 0) {
                if (fatTable != NULL) {
                    free(fatTable);
                }
                int formatSucces = format(arg1, fileName, &fatTable);
                if (formatSucces == SUCCES_VALUE) {
                    currentDirectory = -1;  //vyrestovani soucasneho adresare kdyz vyformatujem novy disk
                    writeToConsoleNormal("Format was succesful");
                }
                currentDirectory = currentDirectory + 1;  //posunuti se do root slozky
                isGood = TRUE;
            } else if (strncmp(command, "exit", 4) == 0) {
                writeToConsoleNormal("Exiting program...");
                isRunning = FALSE; // Ukonceni smycky
            } else if (strncmp(command, "help", 4) == 0) {
                help();
            } else if (strncmp(command, "check", 4) == 0) {
                isGood = check(fileName, fatTable);
                if (isGood) {
                    writeToConsoleNormal("File system is OK");
                } else {
                    writeToConsoleNormal(
                            "File system is currupted, only format, help, exit and check commands can be used");
                }
            } else {
                wrongCommand(command);
            }
        }

        command = NULL;
        arg1 = NULL;
        arg2 = NULL;
    }
    free(fatTable);
}

/*
 * funkce, co zpracovava prikazy ze souboru, vola podporgramy jednotlivych prikazu a dava jim parametry
 * @param fileName - nazev .dat souboru
 * @param needToLoadFatTable - parametr urcujici jestli je nutno nacist fatatblulku z existujiciho .dat souboru
 * @param loadFile - soubor s prikazy
 * @param currentDirectory - cluster (slozka), kde je prikaz load zavolan
 **/
int processCommandsFromFile(const char *fileName, int needToLoadFatTable, FILE *loadFile, int currentCluster) {
    char input[LENGTH_OF_USER_INPUT];

    int currentDirectory = currentCluster;

    int isRunningLoad = TRUE;

    if (needToLoadFatTable == TRUE) {
        loadFatTable(fileName, &fatTable);
        currentDirectory = currentDirectory + 1;
        isGood = check(fileName, fatTable);
        if (isGood) {
            writeToConsoleNormal("File system is OK");
        } else {
            writeToConsoleNormal(
                    "File system is currupted, only format, help, exit and check commands can be used");
        }
        needToLoadFatTable = FALSE;
    }

    fgets(input, sizeof(input), loadFile);

    while (isRunning == TRUE && isRunningLoad == TRUE) {
        input[strcspn(input, "\n")] = 0;  // Odstraneni noveho radku

        // Rozdelime vstup na prikaz a argumenty
        char *command = strtok(input, " ");
        char *arg1 = strtok(NULL, " ");
        char *arg2 = strtok(NULL, " ");

        // Porovname prikaz s pouzitim switch
        if (command != NULL) {
            if (strncmp(command, "cp", 2) == 0 && isGood) {
                cp(arg1, arg2, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "cat", 3) == 0 && isGood) {
                cat(arg1, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "cd", 2) == 0 && isGood) {
                int newDirectory = cd(arg1, fileName, currentDirectory);
                if (newDirectory != FAILURE_VALUE) {
                    currentDirectory = newDirectory;
                }
            } else if (strncmp(command, "mv", 2) == 0 && isGood) {
                mv(arg1, arg2, fileName, currentDirectory);
            } else if (strncmp(command, "mkdir", 5) == 0 && isGood) {
                mkdir(arg1, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "rmdir", 5) == 0 && isGood) {
                rmdir(arg1, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "rm", 2) == 0 && isGood) {
                rm(arg1, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "ls", 2) == 0 && isGood) {
                ls(fileName, currentDirectory, arg1);
            } else if (strncmp(command, "load", 4) == 0) {
                int newDirectory = load(arg1, fileName, currentDirectory, needToLoadFatTable);
                if (newDirectory != FAILURE_VALUE) {
                    currentDirectory = newDirectory;
                }
            } else if (strncmp(command, "outcp", 5) == 0 && isGood) {
                outcp(arg1, arg2, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "info", 4) == 0 && isGood) {
                info(arg1, fileName, fatTable, currentDirectory);
            } else if (strncmp(command, "incp", 4) == 0 && isGood) {
                incp(arg1, arg2, fileName, fatTable, currentDirectory);
            }  else if (strncmp(command, "pwd", 3) == 0 && isGood) {
                pwd(fileName);
            } else if (strncmp(command, "bug", 4) == 0 && isGood) {
                int result = bug(arg1, fileName, fatTable, currentDirectory);
                if (result == SUCCES_VALUE) {
                    isGood = FALSE;
                }
            } else if (strncmp(command, "format", 6) == 0) {
                if (fatTable != NULL) {
                    free(fatTable);
                }
                int formatSucces = format(arg1, fileName, &fatTable);
                if (formatSucces == SUCCES_VALUE) {
                    currentDirectory = -1;  //vyrestovani soucasneho adresare kdyz vyformatujem novy disk
                    writeToConsoleNormal("Format was succesful");
                }
                currentDirectory = currentDirectory + 1;  //posunuti se do root slozky
                isGood = TRUE;
            } else if (strncmp(command, "exit", 4) == 0) {
                writeToConsoleNormal("Exiting program...");
                isRunning = FALSE; // Ukonceni smycky
            } else if (strncmp(command, "help", 4) == 0) {
                help();
            } else if (strncmp(command, "check", 4) == 0) {
                isGood = check(fileName, fatTable);
                if (isGood && isGood != FAILURE_VALUE) {
                    writeToConsoleNormal("File system is OK");
                } else if (isGood != FAILURE_VALUE) {
                    writeToConsoleNormal(
                            "File system is currupted, only format, help, exit and check commands can be used");
                }
            } else {
                wrongCommand(command);
            }
        }

        command = NULL;
        arg1 = NULL;
        arg2 = NULL;

        if (fgets(input, sizeof(input), loadFile) == NULL) {
            // vsechny prikazy ze souboru precteny
            isRunningLoad = FALSE;  // Ukonecni programu
        }
    }

    return currentDirectory;
}

/*
 * procedura, co zjisti jestli je nutno naformatovat disk a spusti proceduru na zparcovavani prikazu
 * @param fileName - nazev .dat souboru
 **/
void run(const char *fileName) {
    if (fileName == NULL) {
        return;
    }

    int loadFatTable = TRUE;

    if (toFormatOrNotThatIsTheQuestion(fileName)) {
        writeToConsoleNormal(".dat file not found. .dot will need to be formated before most commands can be executed.\n");
        loadFatTable = FALSE;
    }

    processCommands(fileName, loadFatTable);
}