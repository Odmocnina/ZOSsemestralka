//
// Created by Michael on 28.10.2024.
//

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "commands.h"
#include "constants.h"
#include "memoryDisk.h"
#include "FAT.h"

/**
 * Struktura pro reprezentaci cesty
 */
struct catOnMars { //https://www.youtube.com/watch?v=ZTVn6Mse_xQ
    int cluster;
    char *name;
};

/** Zasobnik cesty, vrchni prvek je aktualni slozka **/
struct catOnMars catsOnMarsPathStack[MAXIMUM_DEAPTH];
/** soucasny index v zasobniku cesty **/
int indexInPathStack = 1;

/**
 * Funkce pro inicializaci zasobniku cesty na Marsu s korenovou slozkou
 */
void makeSpaceForSmallKittens() {
    for (int i = 0; i < MAXIMUM_DEAPTH; i = i + 1) {
        if (i == 0) {
            struct catOnMars cat;
            cat.cluster = 0;
            cat.name = "root";
            catsOnMarsPathStack[i] = cat;
        } else {
            struct catOnMars cat;
            cat.cluster = FAILURE_VALUE;
            cat.name = "";
            catsOnMarsPathStack[i] = cat;
        }
    }
    indexInPathStack = 1;
}

/**
 * Funkce pro pridani slozky nebo souboru do zasobniku cesty
 * @param cluster Cislo clusteru, kde se nachazi slozka nebo soubor
 * @param name Nazev slozky nebo souboru
 */
void addToPathStack(int cluster, char *name) {
    struct catOnMars cat;
    cat.cluster = cluster;
    cat.name = strdup(name);
    catsOnMarsPathStack[indexInPathStack] = cat;
    indexInPathStack = indexInPathStack + 1;
}

/**
 * Funkce pro odebrani posledni slozky nebo souboru ze zasobniku cesty
 */
void popFromPathStack() {
    struct catOnMars cat;
    cat.cluster = FAILURE_VALUE;
    cat.name = "";
    indexInPathStack = indexInPathStack - 1;
    catsOnMarsPathStack[indexInPathStack] = cat;
}

/**
 * Funkce pro ladici vypis obsahu zasobniku cesty na konzoli
 */
void writePathStackDebug() {
    printf("\n");
    for (int i = 0; i < MAXIMUM_DEAPTH; i = i + 1) {
        printf("i:%d -> cluster: %d, name: %s \n", i, catsOnMarsPathStack[i].cluster, catsOnMarsPathStack[i].name);
    }
}


/**
 * Funkce pro vypis cesty v zasobniku ve formatu prikazu
 */
void writePathStackCommand() {
    printf("\n");
    for (int i = 0; i < indexInPathStack; i = i + 1) {
        if (catsOnMarsPathStack[i].name != NULL) {
            printf("%s/", catsOnMarsPathStack[i].name);
        }
    }
    printf(">");
}

/**
 * Funkce pro uvolneni pameti alokovane pro nazvy slozek a souboru v zasobniku
 */
void vaporizePathStack() {
    for (int i = 0; i < MAXIMUM_DEAPTH; i = i + 1) {
        free(catsOnMarsPathStack[i].name);
    }
}

/**
 * Funkce pro nalezeni cesty na disku podle zadane cesty
 * @param file Ukazatel na soubor disku
 * @param path Zadavana cesta
 * @param currentCluster Aktualni cluster
 * @param changeCurrentCluster Priznak, zda menit aktualni cluster
 * @return Navratova hodnota s clusterem nebo FAILURE_VALUE pri chybe
 */
int findPathInDisk(FILE *file, char *path, int currentCluster, int changeCurrentCluster) {
    if (path == NULL) {
        return 0;
    }

    struct description FATdescription;
    fseek(file, 0, SEEK_SET);
    fread(&FATdescription, sizeof(FATdescription), 1, file);
    int offset = FATdescription.dataStartAddress;

    struct catOnMars saveStack[MAXIMUM_DEAPTH];

    for (int i = 0; i < MAXIMUM_DEAPTH; i = i + 1) {
        saveStack[i] = catsOnMarsPathStack[i];
    }

    int pathIndexSave = indexInPathStack;
    int navrat = FAILURE_VALUE;
    int cluster = currentCluster;

    if ((char)path[0] == '/') { //absolutni cesta
        makeSpaceForSmallKittens();
        cluster = 0;
        navrat = cluster;
        path = path + 1; //smazani prvniho znaku
    } //relativni cesta
    char *elementaryParts = strtok(path, "/");
    int errorOnThePath = FALSE;
    while (elementaryParts != NULL && errorOnThePath == FALSE) {
        if (!strcmp("..", elementaryParts)) {      //bylo nalzeno .., jdeme do vyzsi slozky
            if (cluster != 0) {                    //nejsme v root
                popFromPathStack();
                cluster = catsOnMarsPathStack[indexInPathStack - 1].cluster;
                navrat = cluster;
                elementaryParts = strtok(NULL, "/");
            } else {                               //jsme v root, nemuzem vys
                writeToConsoleError("Wrong path - cant go outside of root");
                writeToConsoleNormal("\n");
                errorOnThePath = TRUE;
            }
        } else {
            cluster = findKittenCluser(file, elementaryParts, cluster, offset);  //pokud neni ..
            if (cluster != FAILURE_VALUE) {                                            //slozka nenalezena
                addToPathStack(cluster, elementaryParts);
                // Dalsi volani strtok pro ziskani dalsi casti
                navrat = cluster;                                                      //nastaveni soucasne slozky
                elementaryParts = strtok(NULL, "/");
            } else {                                                 // pokud je cast cesty zadana spatne
                writeToConsoleError("Wrong path");
                writeToConsoleNormal("\n");
                navrat = FAILURE_VALUE;
                errorOnThePath = TRUE;
            }
        }
    }

    if (changeCurrentCluster == FALSE || errorOnThePath == TRUE) {
        for (int i = 0; i < MAXIMUM_DEAPTH; i = i + 1) { //vraceni zasobniku do puvodniho stavu
            catsOnMarsPathStack[i] = saveStack[i];
        }
        indexInPathStack = pathIndexSave;
    }//navrat zasobniku do puvodniho stavu pokud se nemeni cesta (pokud byla chyba na ceste, nebo se nema slozka zmenit)
    return navrat;
}

/**
 * Funkce pro orezani cesty na posledni slozku nebo soubor
 * @param path Zadavana cesta
 * @return Nazev posledni slozky nebo souboru
 */
char *circumsisePath(char *path) { //Tahle kokotina se snazi pokud je cesta zadana vcene veci, ktera zatim neexistuje,
    // Najdeme poslední výskyt '/'  // tak se snazi cestu rozdelat na 2 casti, tu cast, kterou uskenku, nazev slozky
    // napriklad a na cast kterou najdem slozku, kde to cheme vytvorit

    char *name = malloc(sizeof(char) * 14);  //Za tenhle zbytecnej malloc mam chut vypit celou flasku redidla, ale
    // nejak to problem resi, jen pak nezapomenou tu mrdku freenout

    if (strchr(path, '/') == NULL) { // pokud v path není /
        // Dalsi logika
        name = NULL;
    } else {
        // Najdeme poslední vyskyt '/'
        char *last_slash = strrchr(path, '/');

        if (last_slash != NULL) {
            // Odrezeme cast za '/'
            strcpy(name, last_slash + 1);

            // Zkontrolujeme, jestli je to absolutni cesta se slozkou '/' na zacatku
            if (last_slash == path) {
                // Pokud posledni '/' je na zacatku, znamena to, ze mame jen koren
                *path = '/';    // Vratime '/';
                *(path + 1) = '\0';  // a orizneme ostatni.
            } else {
                // Jinak normalne odrizneme posledni cast
                *last_slash = '\0';
            }   // tohle ze funguje je cistej zazrak
        }
    }

    return name;
}

/**
 * Funkce pro spocitani lomitek v retezci
 * @param str Vstupni retezec
 * @return Pocet lomitek
 */
int countSlashes(const char *str) {
    int count = 0;

    // Kontrola, zda retezec neni NULL
    if (str == NULL) {
        return count;
    }

    // Prochazeni retezce od druheho znaku
    if (str[0] != '\0') {
        for (int i = 0; str[i] != '\0'; i = i + 1) {
            if (str[i] == '/') {
                count = count + 1;
            }
        }
        count = count + indexInPathStack;
    } else {
        for (int i = 1; str[i] != '\0'; i = i + 1) {
            if (str[i] == '/') {
                count = count + 1;
            }
        }
    }

    return count;
}


/**
 * Funkce pro orezani bilich znaku na zacatku a konci retezce
 * @param str Vstupni retezec
 * @return Orezany retezec
 */
char* trim(char* str) {
    char* end;

    // Orezani bilych znaku na zacatku
    while(isspace((unsigned char)*str)) {
        str = str + 1;
    }

    // Pokud je retezec prázdný, vracime prazdny retezec
    if(*str == 0) {
        return str;
    }

    // Najdeme konec retezce
    end = str + strlen(str) - 1;

    // Ořezání bílých znaků na konci
    while(end > str && (isspace((unsigned char)*end))) {
        end = end - 1;
    }

    // Ukonceni retezce
    *(end+1) = '\0';

    return str;
}

/**
 * Funkce pro vypis cesty ve formatu prikazu
 */
void writePathCommand() {
    writeToConsoleNormal("Path\n");
    writeToConsoleNormal("----\n");
    for (int i = 0; i < indexInPathStack; i = i + 1) {
        writeToConsoleNormal(catsOnMarsPathStack[i].name);
        if (i < (indexInPathStack - 1)) {
            writeToConsoleNormal("/");
        }
    }
}
