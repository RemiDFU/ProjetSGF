

#include "disk.h"

#define DISK_MAGIC 0xdeadbeef

static FILE *diskfile;
static int nblocks = 0;
static int nreads = 0;
static int nwrites = 0;

/**
 *  Initialise le disque
 *
 * @param path Chemin d'accès à l'image disque à créer.
 * @return true si le disque est correctement initialisé
 */
int intialisation_disque(const char *path) {
    // Ouvre le descripteur de fichier vers le chemin spécifié.
    diskfile = fopen(path, "r+");
    if (!diskfile) diskfile = fopen(path, "w+");
    if (!diskfile) return 0;

    ftruncate(fileno(diskfile), BLOCKS * BLOCK_SIZE);

    nblocks = BLOCKS;
    nreads = 0;
    nwrites = 0;

    return 1;
}

/**
 * Effectuer un contrôle d'intégrité avant une opération de lecture ou d'écriture
 *
 * @param blocknum Pointeur vers la structure Disk.
 * @param data Data buffer
 */
static void disque_ready(int blocknum, const void *data) {

    // Vérifiez si le bloc est valide.
    if (blocknum >= nblocks || blocknum < 0) {
        abort();
    }

    // Vérifiez si les données sont valides.
    if (!data) {
        abort();
    }
}

/**
 * Lire les données du disque au bloc spécifié dans le tampon (buffer) de données.
 *
 * @param blocknum Pointeur vers la structure Disque.
 * @param data Data buffer
 */
void disque_read(int blocknum, char *data) {
    // Exécution du contrôle d'intégrité.
    disque_ready(blocknum, data);

    // Recherche d'un bloc spécifique.
    fseek(diskfile, blocknum * BLOCK_SIZE, SEEK_SET);

    // Lecture du bloc dans le tampon (buffer) de données
    if (fread(data, BLOCK_SIZE, 1, diskfile) == 1) {
        nreads++;
    } else {
        printf("Erreur d'accès au disque: %s\n", strerror(errno));
        abort();
    }
}

/**
 * Écriture de données sur le disque au bloc spécifié à partir du buffer de données
 *
 * @param blocknum
 * @param data
 */
void disque_write(int blocknum, const char *data) {
    // Exécution du contrôle d'intégrité.
    disque_ready(blocknum, data);

    // Recherche d'un bloc spécifique.
    fseek(diskfile, blocknum * BLOCK_SIZE, SEEK_SET);

    // Écriture d'un buffer de données sur un bloc de disque.
    if (fwrite(data, BLOCK_SIZE, 1, diskfile) == 1) {
        nwrites++;
    } else {
        printf("Erreur disque: %s\n", strerror(errno));
        abort();
    }
}

void disque_close() {
    //Fermeture du disque
    if (diskfile) {
        fclose(diskfile);
        diskfile = 0;
    }
}

// Donne la taille du disque
int disque_size() {
    return nblocks;
}