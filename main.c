#include <stdio.h>
#include <string.h>
#include "runningOfProgram.h"
#include "constants.h"
#include "memoryDisk.h"

/**
 * Hlavni funkce programu, ktera kontroluje pocet argumentu,
 * zda soubor ma spravnou priponu, a nasledne spusti program.
 * @param argc Pocet argumentu prikazoveho radku.
 * @param argv Pole argumentu prikazoveho radku.
 * @return Vraci FAILURE_VALUE, pokud pocet argumentu je nespravny
 *         nebo pokud soubor nema priponu .dat. Vraci SUCCES_VALUE pri uspesnem spusteni.
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Wrong number of arguments!");
        return FAILURE_VALUE;
    }

    // Kontrola, zda argument je soubor s priponou .dat
    if (strstr(argv[1], ".dat") == NULL) {
        writeToConsoleError("The file must have a .dat extension");
        return FAILURE_VALUE;
    }

    run(argv[1]);

    return SUCCES_VALUE;
}


