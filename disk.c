

#include "disk.h"

#define DISK_MAGIC 0xdeadbeef

static FILE *diskfile;
static int nblocks = 0;
static int nreads = 0;
static int nwrites = 0;

/**
 *  Ouvre le disque au chemin spécifié avec le nombre de blocs spécifié
 *
 * @param path Chemin d'accès à l'image disque à créer.
 * @param blocks Nombre de blocs à allouer pour l'image disque.
 * @return vrai en cas de succès (faux en cas d'échec)
 */
int disk_init(const char *path, int blocks) {
    // Ouvre le descripteur de fichier vers le chemin spécifié.
    diskfile = fopen(path, "r+");
    if (!diskfile) diskfile = fopen(path, "w+");
    if (!diskfile) return 0;

    // Tronque le fichier à la taille désirée (blocs * BLOCK_SIZE).
    ftruncate(fileno(diskfile), blocks * BLOCK_SIZE);

    // Définit les attributs appropriés.
    nblocks = blocks;
    nreads = 0;
    nwrites = 0;

    return 1;
}

int disk_size() {
    return nblocks;
}

/**
 * Effectuer un contrôle d'intégrité avant une opération de lecture ou d'écriture
 *
 * @param blocknum Pointeur vers la structure Disk.
 * @param data Data buffer
 */
static void sanity_check(int blocknum, const void *data) {
    // Vérifiez si le disque est valide.
    if (blocknum < 0) {
        printf("ERROR: blocknum (%d) is negative!\n", blocknum);
        abort();
    }

    // Vérifiez si le bloc est valide.
    if (blocknum >= nblocks) {
        printf("ERROR: blocknum (%d) is too big!\n", blocknum);
        abort();
    }

    // Vérifiez si les données sont valides.
    if (!data) {
        printf("ERROR: null data pointer!\n");
        abort();
    }
}

/**
 * Lire les données du disque au bloc spécifié dans le tampon (buffer) de données.
 *
 * @param blocknum Pointeur vers la structure Disk.
 * @param data Data buffer
 */
void disk_read(int blocknum, char *data) {
    // Exécution du contrôle d'intégrité.
    sanity_check(blocknum, data);

    // Recherche d'un bloc spécifique.
    fseek(diskfile, blocknum * BLOCK_SIZE, SEEK_SET);

    // Lecture du bloc dans le tampon (buffer) de données
    if (fread(data, BLOCK_SIZE, 1, diskfile) == 1) {
        nreads++;
    } else {
        printf("ERROR: couldn't access simulated disk: %s\n", strerror(errno));
        abort();
    }
}

/**
 * Écriture de données sur le disque au bloc spécifié à partir du tampon de données
 *
 * @param blocknum
 * @param data
 */
void disk_write(int blocknum, const char *data) {
    // Exécution du contrôle d'intégrité.
    sanity_check(blocknum, data);

    // Recherche d'un bloc spécifique.
    fseek(diskfile, blocknum * BLOCK_SIZE, SEEK_SET);

    // Écriture d'un tampon (buffer) de données sur un bloc de disque.
    if (fwrite(data, BLOCK_SIZE, 1, diskfile) == 1) {
        nwrites++;
    } else {
        printf("ERROR: couldn't access simulated disk: %s\n", strerror(errno));
        abort();
    }
}

/**
 * Fermer la structure du disque
 */
void disk_close() {
    if (diskfile) {
        // Rapport sur le nombre de lectures et d'écritures sur le disque
        printf("%d disk block reads\n", nreads);
        printf("%d disk block writes\n", nwrites);
        // Fermer le descripteur de fichier du disque.
        fclose(diskfile);
        // Libération de la mémoire de la structure du disque.
        diskfile = 0;
    }
}