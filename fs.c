#include "fs.h"

int *bitmap = NULL;
int bitmap_size;

// Internal member variables
int free_block[BLOCKS];
int inode_counter[BLOCKS];
int dir_counter[BLOCKS];
struct fs_directory curr_dir;


int get_new_datablock() {
    if (bitmap == NULL) {
        printf("No disk mounted, please mount first\n");
        return -1;
    }

    union fs_block block;
    disk_read(0, block.data);

    //start at the first datablock instance
    for (int i = block.super.ninodeblocks + 1; i < bitmap_size; i++) {
        if (bitmap[i] == 0) {
            //zero it out
            memset(&bitmap[i], 0, sizeof(bitmap[0]));
            return i;
        }
    }

    printf("No more room left\n");
    return -1;
}

/**
 * Format du disque
 *
 * @return vrai en cas de succès, faux en cas d'échec
 */
int fs_format() {
    union fs_block block;

    if (disk_size() < 3) {
        printf("Disk to small, does not meet minimum node size\n");
        return 0;
    } else if (bitmap != NULL) {
        printf("Cannot format mounted image\n");
        return 0;
    }

    // Définir le SuperBlock avec les attributs appropriés.
    block.super.magic = FS_MAGIC;
    block.super.nblocks = disk_size();
    block.super.ninodeblocks = disk_size() / 10 + 1;
    block.super.ninodes = 128 * block.super.ninodeblocks;
    block.super.ndirblocks = disk_size() / 100 + 1;

    // Ecrire le SuperBlock dans le disque
    disk_write(0, block.data);

    // Effacer tous les blocs restants
    for (int inode_block = (block.super.ninodeblocks) + 1;
         inode_block < block.super.ninodeblocks - block.super.ndirblocks; inode_block++) {
        union fs_block zero;
        memset(zero.data, 0, BLOCK_SIZE);
        disk_write(inode_block, zero.data);
    }

    // Effacer tous les répertoires restants
    for (int i = block.super.nblocks - block.super.ndirblocks; i < block.super.nblocks; i++) {
        union fs_block zero;
        struct fs_directory dir;
        dir.inum = -1;
        dir.isvalid = 0;
        memset(dir.table, 0, sizeof(struct fs_dirent) * ENTRIES_PER_DIR);
        for (int j = 0; j < DIR_PER_BLOCK; j++) {
            zero.directories[j] = dir;
        }
        disk_write(i, zero.data);
    }

    // Définir la racine du système de fichiers
    struct fs_directory root;
    strcpy(root.name, "/");
    root.inum = 0;
    root.isvalid = 1;

    struct fs_dirent temp;
    memset(&temp, 0, sizeof(temp));
    temp.inum = 0;
    temp.type = 0;
    temp.isvalid = 1;
    char tstr1[] = ".";
    char tstr2[] = "..";
    strcpy(temp.name, tstr1);
    memcpy(&(root.table[0]), &temp, sizeof(struct fs_dirent));
    strcpy(temp.name, tstr2);
    memcpy(&(root.table[1]), &temp, sizeof(struct fs_dirent));

    union fs_block dirblock;
    memcpy(&(dirblock.directories[0]), &root, sizeof(root));
    disk_write(block.super.nblocks - 1, dirblock.data);

    return 1;
}


void print_bitmap() {
    if (bitmap != NULL) {
        for (int i = 0; i < bitmap_size; i++) {
            printf(" %d", bitmap[i]);
        }
        printf("\n");
    } else {
        printf("Bitmap NULL\n");
    }
}

/**
 * Déboguer le système de fichiers
 */
void fs_debug() {
    union fs_block block;

    // Lire SuperBlock et rapportez ses informations.
    disk_read(0, block.data);

    printf("superblock:\n");
    if (block.super.magic != FS_MAGIC) {
        printf("Cannot understand file system\n");
        return;
    }
    printf("    magic number is valid\n");
    printf("    %d blocks\n", block.super.nblocks);
    printf("    %d inode blocks\n", block.super.ninodeblocks);
    printf("    %d inodes\n", block.super.ninodes);

    // Lire la table des Inodes et rapporter des informations sur chaque Inode.
    union fs_block inode_block;
    struct fs_inode inode;
    for (int i = 1; i < block.super.ninodeblocks; i++) {

        disk_read(i, inode_block.data);

        for (int i_node = 0; i_node < INODES_PER_BLOCK; i_node++) {

            inode = inode_block.inode[i_node];

            if (inode.isvalid) {
                printf("inode %d:\n", i_node);
                printf("    size: %d\n", inode.size);
                printf("    direct blocks:");

                for (int d_blocks = 0; d_blocks * 4096 < inode.size && d_blocks < 5; d_blocks++) {
                    printf(" %d", inode.direct[d_blocks]);
                }

                printf("\n");

                if (inode.size > 5 * 4096) {

                    printf("    indirect: %d\n", inode.indirect);
                    union fs_block temp_block;
                    disk_read(inode.indirect, temp_block.data);

                    printf("    indirect data blocks:");
                    for (int indirect_block = 0; indirect_block < (double) inode.size / 4096 - 5; indirect_block++) {
                        printf(" %d", temp_block.pointers[indirect_block]);
                    }
                    printf("\n");
                }
            }
        }
    }
}

/**
 * Mount FileSystem
 *
 * @return vrai en cas de succès, faux en cas d'échec
 */
int fs_mount() {
    union fs_block block;
    // Lire et vérifier SuperBlock
    disk_read(0, block.data);

    // Alloue la mémoire pour le bitmap
    bitmap = calloc(block.super.nblocks, sizeof(int));
    bitmap_size = block.super.nblocks;

    // analyse le système de fichiers pour définir correctement le bitmap pour le système de fichiers donné
    union fs_block inode_block;
    struct fs_inode inode;
    for (int i = 1; i < block.super.ninodeblocks; i++) {

        disk_read(i, inode_block.data);

        for (int i_node = 0; i_node < INODES_PER_BLOCK; i_node++) {

            inode = inode_block.inode[i_node];

            if (inode.isvalid) {
                bitmap[i] = 1;
                for (int d_blocks = 0; d_blocks * 4096 < inode.size && d_blocks < 5; d_blocks++) {
                    bitmap[inode.direct[d_blocks]] = 1;
                }

                if (inode.size > 5 * 4096) {

                    bitmap[inode.indirect] = 1;

                    union fs_block temp_block;
                    disk_read(inode.indirect, temp_block.data);

                    for (int indirect_block = 0; indirect_block < (double) inode.size / 4096 - 5; indirect_block++) {
                        bitmap[temp_block.pointers[indirect_block]] = 1;
                    }
                }
            }
        }
    }

    memset(dir_counter, 0, sizeof(dir_counter));
    union fs_block dirblock;
    for (int dirs = 0; dirs < block.super.ndirblocks; dirs++) {
        disk_read(block.super.nblocks - 1 - dirs, dirblock.data);
        for (int offset = 0; offset < DIR_PER_BLOCK; offset++) {
            if (dirblock.directories[offset].isvalid == 1) {
                dir_counter[dirs]++;
            }
        }
        if (dirs == 0) {
            curr_dir = dirblock.directories[0];
        }
    }

    return 1;
}

/**
 * Allouer un Inode dans la table des Inodes du Système de Fichier
 *
 * @return Numéro d'Inode de l'Inode alloué en cas de succès, faux en cas d'échec
 */
int fs_create() {
    if (bitmap == NULL) {
        printf("No mounted disk\n");
        return 0;
    }

    union fs_block block;
    disk_read(0, block.data);

    // Recherchez la table Inode pour un inode libre.
    for (int inode_block_index = 1; inode_block_index < block.super.nblocks; inode_block_index++) {
        disk_read(inode_block_index, block.data);

        struct fs_inode inode;
        for (int inode_index = 0; inode_index < POINTERS_PER_BLOCK; inode_index++) {
            if (inode_index == 0 && inode_block_index == 1)
                inode_index = 1;

            // lire l'espace comme un inode, et vérifier le flag valide
            inode = block.inode[inode_index];

            if (inode.isvalid == 0) {

                // si l'inode est invalide, nous pouvons remplir l'espace en toute sécurité
                inode.isvalid = 1;
                inode.size = 0;
                memset(inode.direct, 0, sizeof(inode.direct));
                inode.indirect = 0;

                bitmap[inode_block_index] = 1;
                block.inode[inode_index] = inode;
                disk_write(inode_block_index, block.data);
                return inode_index + (inode_block_index - 1) * 128;
            }
        }
    }

    printf("Could not create inode, inode blocks are full");
    return 0;
}

/**
 * Suppression de l'Inode et des données associées du Système de Fichier
 * @param inumber Inode à supprimer.
 * @return vrai en cas de succès, faux en cas d'échec
 */
int fs_delete(int inumber) {
    // calculate correct inode block
    int inode_block_index = (inumber + 128 - 1) / 128;

    union fs_block block;
    disk_read(0, block.data);

    if (inode_block_index > block.super.ninodeblocks) {
        printf("Inode number outside limit\n");
        return 0;
    }

    // read the inode block and begin checking it for open spaces
    disk_read(inode_block_index, block.data);

    struct fs_inode inode = block.inode[inumber % 128];
    if (inode.isvalid) {
        //zero it out and return 1;
        inode = (struct fs_inode) {0};
        block.inode[inumber % 128] = inode;
        disk_write(inode_block_index, block.data);
        return 1;
    } else {
        //cannot delete invalid inode, I think...
        printf("Not a valid inode, cannot delete\n");
        return 0;
    }
}

int fs_getsize(int inumber) {
    union fs_block block;
    disk_read(0, block.data);

    // calculate correct inode block
    int inode_block_index = (inumber + 128 - 1) / 128;

    if (inode_block_index > block.super.ninodeblocks) {
        printf("Inode number outside limit\n");
        return 0;
    }

    disk_read(inode_block_index, block.data);
    struct fs_inode inode = block.inode[inumber % 128];

    if (inode.isvalid) {
        return inode.size;
    }

    printf("Invalid inode at inumber\n");
    return -1;
}

/**
 * Lit à partir de l'inode spécifié dans le tampon de données, la longeur du tampon en commençant par l'offset spécifié
 *
 * @param inumber Inode pour lire les données.
 * @param data Data buffer
 * @param length Nombre d'octets à lire.
 * @param offset Décalage de l'octet à partir duquel la lecture doit commencer.
 * @return Nombre d'octets lus (-1 en cas d'erreur).
 */
int fs_read(int inumber, char *data, int length, int offset) {
    memset(data, 0, length);
    union fs_block block;
    disk_read(0, block.data);
    if (inumber == 0 || inumber > block.super.ninodes) {
        printf("Cannot read; invalid inumber\n");
        return -1;
    }

    int total_bytes_read = 0;
    int inode_block_index = (inumber + 128 - 1) / 128;

    // Chargement des informations sur les inodes.
    disk_read(inode_block_index, block.data);

    struct fs_inode inode = block.inode[inumber % 128];
    if (!inode.isvalid || inode.size == 0) {
        printf("Invalid inode, cannot read\n");
    } else {
        if (offset >= inode.size)
            return -1;

        // Lire continuellement les blocs et copier les données dans la mémoire tampon.
        // Block direct en premier puis blocks indirects
        int direct_index = (int) floor(offset / 4096);
        union fs_block temp_block;

        int upper_limit = length;
        if (inode.size < length + offset)
            upper_limit = inode.size - offset;

        while (direct_index < 5 && total_bytes_read < upper_limit) {

            int chunk = 4096;
            if (chunk + total_bytes_read > upper_limit)
                chunk = upper_limit - total_bytes_read;

            disk_read(inode.direct[direct_index], temp_block.data);

            strncat(data, temp_block.data, chunk);
            total_bytes_read += chunk;
            direct_index++;
        }

        if (total_bytes_read < upper_limit) {

            //indirect block
            union fs_block ind_block;
            disk_read(inode.indirect, ind_block.data);

            for (int indirect_block = 0;
                 indirect_block < (double) inode.size / 4096 - 5 && total_bytes_read < upper_limit; indirect_block++) {

                disk_read(ind_block.pointers[indirect_block], temp_block.data);

                int chunk = 4096;
                if (chunk + total_bytes_read > upper_limit)
                    chunk = upper_limit - total_bytes_read;
                strncat(data, temp_block.data, chunk);

                total_bytes_read += chunk;
            }
        }
        return total_bytes_read;
    }

    return -1;
}

/**
 * Ecrit à partir de l'inode spécifié dans le tampon de données, la longeur du tampon en commençant par l'offset spécifié
 *
 * @param inumber Inode pour lire les données.
 * @param data Data buffer
 * @param length Nombre d'octets à lire.
 * @param offset Décalage de l'octet à partir duquel la lecture doit commencer.
 * @return Nombre d'octets lus (-1 en cas d'erreur).
 */
int fs_write(int inumber, const char *data, int length, int offset) {
    union fs_block block;
    disk_read(0, block.data);
    if (inumber == 0 || inumber > block.super.ninodes) {
        printf("Cannot write; invalid inumber\n");
        return -1;
    }

    int total_bytes_wrote = 0;
    int inode_block_index = (inumber + 128 - 1) / 128;

    // Chargement des informations sur les inodes.
    disk_read(inode_block_index, block.data);

    // Copie en continu des données du tampon vers les blocs.
    struct fs_inode inode = block.inode[inumber % 128];
    if (!inode.isvalid) {
        printf("Invalid inode, cannot write\n");
    } else {

        int direct_index = (int) floor(offset / 4096);
        union fs_block temp_block;

        while (direct_index < 5 && total_bytes_wrote < length) {
            if (inode.direct[direct_index] == 0) {
                int index = get_new_datablock();
                if (index == -1) {
                    printf("Not enough space left, cannot write");
                    return -1;
                }
                inode.direct[direct_index] = index;
                bitmap[index] = 1;
            }

            int chunk = 4096;
            if (chunk + total_bytes_wrote > length)
                chunk = length - total_bytes_wrote;

            strncpy(temp_block.data, data, chunk);
            data += chunk;

            disk_write(inode.direct[direct_index], temp_block.data);
            inode.size += chunk;

            total_bytes_wrote += chunk;
            direct_index++;
        }

        if (total_bytes_wrote < length) {

            //indirect block
            union fs_block ind_block;

            if (inode.indirect == 0) {
                int index = get_new_datablock();
                if (index == -1) {
                    printf("Not enough space left, cannot write");
                    return -1;
                }
                inode.indirect = index;
                bitmap[index] = 1;
            }

            disk_read(inode.indirect, ind_block.data);

            for (int indirect_block = 0; total_bytes_wrote < length; indirect_block++) {
                if (ind_block.pointers[indirect_block] == 0) {
                    int index = get_new_datablock();
                    if (index == -1) {
                        printf("Not enough space left, cannot write");
                        return -1;
                    }
                    inode.direct[direct_index] = index;
                }

                int chunk = 4096;
                if (chunk + total_bytes_wrote > length)
                    chunk = length - total_bytes_wrote;

                strncpy(temp_block.data, data, chunk);
                data += chunk;

                disk_write(inode.direct[direct_index], temp_block.data);
                inode.size += chunk;

                total_bytes_wrote += chunk;
            }
        }
        block.inode[inumber % 128] = inode;
        disk_write(inode_block_index, block.data);
        return total_bytes_wrote;
    }

    return -1;
}
/**
 * Ajoute un répertoire
 *
 * @param dir Répertoire dans lequel l'entrée doit être ajoutée
 * @param inum Numéro d'inode
 * @param type type = 1 pour fichier , type = 0 pour répertoire
 * @param name Nom du fichier/répertoire
 * @return Répertoire avec une entrée ajoutée ou avec un bit valide mis à 0 en cas d'erreur.
 */
struct fs_directory fs_add_dir_entry(struct fs_directory dir, int inum, int type, char name[]) {
    struct fs_directory tempdir = dir;

    // Trouver un Dirent libre dans le tableau à modifier
    int idx = 0;
    for (; idx < ENTRIES_PER_DIR; idx++) {
        if (tempdir.table[idx].isvalid == 0) {
            break;
        }
    }

    // Si aucun Dirent n'est trouvé, erreur
    if (idx == ENTRIES_PER_DIR) {
        printf("Directory entry limit reached..exiting\n");
        tempdir.isvalid = 0;
        return tempdir;
    }

    // Dirent libre trouvé, ajouter le nouveau à la table
    tempdir.table[idx].inum = inum;
    tempdir.table[idx].type = type;
    tempdir.table[idx].isvalid = 1;
    strcpy(tempdir.table[idx].name, name);

    return tempdir;
}

/**
 * Ecrit le répertoire sur le disque.
 *
 * @param dir Répertoire valide à réécrire
 */
void fs_write_dir_back(struct fs_directory dir) {
    // Obtenir l'offset et l'index du bloc
    int block_idx = (dir.inum / DIR_PER_BLOCK);
    int block_offset = (dir.inum % DIR_PER_BLOCK);

    // Lire le bloc
    union fs_block block0, block;
    disk_read(0, block0.data);
    disk_read(block0.super.nblocks - 1 - block_idx, block.data);
    block.directories[block_offset] = dir;

    // Ecire le Dirblock
    disk_write(block0.super.nblocks - 1 - block_idx, block.data);
}

/**
 * Trouve une entrée valide avec le même nom.
 *
 * @param dir Répertoire de recherche
 * @param name Nom du fichier/répertoire
 * @return décalage dans la table. -1 en cas d'erreur
 */
int fs_dir_lookup(struct fs_directory dir, char name[]) {
    int offset = 0;
    for (; offset < ENTRIES_PER_DIR; offset++) {
        if (
                (dir.table[offset].isvalid == 1) &&
                (streq(dir.table[offset].name, name))) {
            break;
        }
    }
    if (offset == ENTRIES_PER_DIR) {
        return -1;
    }

    return offset;
}

/**
 * Lit le répertoire à partir du décalage du Dirent
 *
 * @param offset Décalage de la table qui doit être lue.
 * @return Retourne le répertoire avec bit valide, un bit=0 en cas d'erreur.
 */
struct fs_directory fs_read_dir_from_offset(int offset) {
    if (
            (offset < 0) ||
            (offset >= ENTRIES_PER_DIR) ||
            (curr_dir.table[offset].isvalid == 0) ||
            (curr_dir.table[offset].type != 0)) {
        struct fs_directory temp;
        temp.isvalid = 0;
        return temp;
    }

    // Obtenir l'offset et l'index
    int inum = curr_dir.table[offset].inum;
    int block_idx = (inum / DIR_PER_BLOCK);
    int block_offset = (inum % DIR_PER_BLOCK);

    // lire le Block
    union fs_block block0, blk;
    disk_read(0, block0.data);
    disk_read(block0.super.nblocks - 1 - block_idx, blk.data);
    return (blk.directories[block_offset]);
}

/**
 * Crée un fichier de taille 0
 *
 * @param name Nom du fichier
 * @return vrai en cas de succès, erreur en cas d'échec
 */
int fs_touch(char name[NAMESIZE]) {
    if (bitmap == NULL) {
        printf("No disk mounted, please mount first\n");
        return -1;
    }

    for (int offset = 0; offset < ENTRIES_PER_DIR; offset++) {
        if (curr_dir.table[offset].isvalid) {
            if (streq(curr_dir.table[offset].name, name)) {
                printf("File already exists\n");
                return -1;
            }
        }
    }
    int new_node_idx = fs_create();
    if (new_node_idx == -1) {
        printf("Error creating new inode\n");
        return 0;
    }

    struct fs_directory temp = fs_add_dir_entry(curr_dir, new_node_idx, 1, name);
    if (temp.isvalid == 0) {
        printf("Error adding new file\n");
        return -1;
    }
    curr_dir = temp;

    fs_write_dir_back(curr_dir);

    return 1;
}

/**
 * Lister tous les Dirent curr_dir.
 *
 * @return vrai en cas de succès, faux en cas d'échec
 */
int fs_ls() {
    if (bitmap == NULL) {
        printf("No disk mounted, please mount first\n");
        return -1;
    }
    char name[] = ".";
    return fs_dir(name);
}

/**
 * Liste le répertoire donné par le nom. Appelé par ls pour imprimer current_dir. Imprime un tableau des répertoires.
 * @param name Nom du répertoire
 * @return vrai en cas de succès, erreur en cas d'échec
 */
int fs_dir(char name[]) {
    if (bitmap == NULL) {
        printf("No disk mounted, please mount first\n");
        return -1;
    }
    int offset = fs_dir_lookup(curr_dir, name);
    if (offset == -1) {
        printf("No such Directory\n");
        return -1;
    }

    struct fs_directory dir = fs_read_dir_from_offset(offset);

    if (dir.isvalid == 0) {
        printf("Directory Invalid\n");
        return -1;
    }

    printf("   inum    |       name       | type\n");
    for (int idx = 0; idx < ENTRIES_PER_DIR; idx++) {
        struct fs_dirent temp = dir.table[idx];
        if (temp.isvalid == 1) {
            if (temp.type == 1)
                printf("%-10u | %-16s | %-5s\n", temp.inum, temp.name, "file");
            else
                printf("%-10u | %-16s | %-5s\n", temp.inum, temp.name, "dir");
        }
    }
    return 1;
}

/**
 * Crée un répertoire vide avec le nom donné Ajoute 2 Dirent , '.' et '..' dans sa table.
 *
 * @param name Nom du répertoire
 * @return
 */
int fs_mkdir(char name[NAMESIZE]) {
    if (bitmap == NULL) {
        printf("No disk mounted, please mount first\n");
        return -1;
    }

    union fs_block zero;
    disk_read(0, zero.data);

    // trouver dirBlock vide
    int block_idx = 0;
    for (; block_idx < zero.super.nblocks; block_idx++)
        if (dir_counter[block_idx] < DIR_PER_BLOCK)
            break;

    if (block_idx == zero.super.ndirblocks) {
        printf("Directory limit reached\n");
        return -1;
    }

    union fs_block block;
    disk_read(zero.super.nblocks - 1 - block_idx, block.data);

    // Trouve un repertoire vide dans dirBlok
    int offset = 0;
    for (; offset < DIR_PER_BLOCK; offset++)
        if (block.directories[offset].isvalid == 0)
            break;

    if (offset == DIR_PER_BLOCK) {
        printf("Error in creating directory.\n");
        return -1;
    }

    // crée un nouveau repertoire
    struct fs_directory new_dir, temp;
    memset(&new_dir, 0, sizeof(struct fs_directory));
    new_dir.inum = block_idx * DIR_PER_BLOCK + offset;
    new_dir.isvalid = 1;
    strcpy(new_dir.name, name);

    // Créer deux entrées "." et ".."
    char tstr1[] = ".", tstr2[] = "..";
    temp = new_dir;
    temp = fs_add_dir_entry(temp, temp.inum, 0, tstr1);
    temp = fs_add_dir_entry(temp, curr_dir.inum, 0, tstr2);
    if (temp.isvalid == 0) {
        printf("Error creating new directory\n");
        return 0;
    }
    new_dir = temp;

    temp = fs_add_dir_entry(curr_dir, new_dir.inum, 0, new_dir.name);
    if (temp.isvalid == 0) {
        printf("Error adding new directory\n");
        return 0;
    }
    curr_dir = temp;

    // Réécrire le nouveau répertoire sur le disque
    fs_write_dir_back(new_dir);

    // Réécrire le répertoire courant sur le disque.
    fs_write_dir_back(curr_dir);

    // Incrémenter le compteur
    dir_counter[block_idx]++;

    return 1;
}

/**
 * Change le répertoire actuel pour le nom de répertoire donné.
 * @param name Nom du répertoire
 * @return vrai en cas de succès, erreur en cas d'échec
 */
int fs_cd(char name[NAMESIZE]) {
    if (bitmap == NULL) {
        printf("No disk mounted, please mount first\n");
        return -1;
    }
    // Lire le dirblock sur le disque
    int offset = fs_dir_lookup(curr_dir, name);
    if ((offset == -1) || (curr_dir.table[offset].type == 1)) {
        printf("No such directory\n");
        return -1;
    }

    struct fs_directory temp = fs_read_dir_from_offset(offset);
    if (temp.isvalid == 0) {
        return -1;
    }
    curr_dir = temp;
    return 1;
}

/**
 * Fonction d'aide pour supprimer un répertoire du répertoire parent.
 *
 * @param parent Répertoire à partir duquel l'autre répertoire doit être supprimé.
 * @param name Nom du répertoire à supprimer
 * @return
 */
struct fs_directory rmdir_helper(struct fs_directory parent, char name[]) {
    // initializations
    struct fs_directory dir, temp;
    int inum, blk_idx, blk_off;
    union fs_block blk, zero;
    disk_read(0, zero.data);

    if (bitmap == NULL) {
        printf("No disk mounted, please mount first\n");
        return dir;
    }

    // Obtenir offset du répertoire à supprimer
    int offset = fs_dir_lookup(parent, name);
    if (offset == -1) {
        dir.isvalid = 0;
        return dir;
    }

    // Obtenir le block et l'offset
    inum = parent.table[offset].inum;
    blk_idx = inum / DIR_PER_BLOCK;
    blk_off = inum % DIR_PER_BLOCK;
    disk_read(zero.super.nblocks - 1 - blk_idx, blk.data);

    dir = blk.directories[blk_off];
    if (dir.isvalid == 0) {
        return dir;
    }

    // Vérifiez si c'est le répertoire racine
    if (streq(dir.name, curr_dir.name)) {
        printf("Current Directory cannot be removed.\n");
        dir.isvalid = 0;
        return dir;
    }

    // Supprimer tous les Dirent dans le répertoire à supprimer
    for (int ii = 0; ii < ENTRIES_PER_DIR; ii++) {
        if ((ii > 1) && (dir.table[ii].isvalid == 1)) {
            temp = rm_helper(dir, dir.table[ii].name);
            if (temp.isvalid == 0)
                return temp;
            dir = temp;
        }
        dir.table[ii].isvalid = 0;
    }
    // Relisez le bloc, car il peut avoir été modifié par le Dirent.
    disk_read(zero.super.nblocks - 1 - blk_idx, blk.data);

    // Réécris-le
    dir.isvalid = 0;
    blk.directories[blk_off] = dir;
    disk_write(zero.super.nblocks - 1 - blk_idx, blk.data);

    // Retirez-le du parent
    parent.table[offset].isvalid = 0;
    fs_write_dir_back(parent);

    // Mettre à jour le compteur
    dir_counter[blk_idx]--;

    return parent;
}

/**
 * Fonction d'aide pour supprimer un fichier/répertoire du répertoire parent.
 *
 * @param dir Répertoire parent qui contient le nom
 * @param name Fichier/répertoire à supprimer
 * @return Retourne le répertoire valide avec un bit=0, un bit valide en cas d'erreur.
 */
struct fs_directory rm_helper(struct fs_directory dir, char name[]) {
    if (bitmap == NULL) {
        printf("No disk mounted, please mount first\n");
        dir.isvalid = 0;
        return dir;
    }
    // Obtenir le décalage pour la suppression
    int offset = fs_dir_lookup(dir, name);
    if (offset == -1) {
        printf("No such file/directory\n");
        dir.isvalid = 0;
        return dir;
    }

    // Vérifiez si le répertoire
    if (dir.table[offset].type == 0) {
        return rmdir_helper(dir, name);
    }

    // Obtenir le numéro d'entrée
    int inum = dir.table[offset].inum;
    printf("%u\n", inum);
    // Suppression de l'inode
    if (!fs_delete(inum)) {
        printf("Failed to remove Inode\n");
        dir.isvalid = 0;
        return dir;
    }
    //Supprimer l'entrée
    dir.table[offset].isvalid = 0;

    //Réécrivez les changements
    fs_write_dir_back(dir);

    return dir;
}

/**
 * Supprime le répertoire dont le nom est donné. Supprime également tous les répertoires de sa table.
 *
 * @param name Nom du répertoire à supprimer
 * @return
 */
int fs_rmdir(char name[NAMESIZE]) {
    struct fs_directory temp = rmdir_helper(curr_dir, name);
    if (temp.isvalid == 0) {
        curr_dir = temp;
        return 1;
    }
    return 0;
}

/**
 * Supprime le fichier/répertoire donné.
 *
 * @param name Entrée à supprimer
 * @return
 */
int fs_rm(char name[]) {
    struct fs_directory temp = rm_helper(curr_dir, name);
    if (temp.isvalid == 1) {
        curr_dir = temp;
        return 1;
    }
    return 0;
}

/**
 * Copie le fichier avec le chemin fourni dans le répertoire courant.
 * Imprime la quantité d'octets copiés.
 * Crée un nouveau fichier si le fichier n'existe pas.
 * Ecrase le fichier s'il existe déjà.
 *
 * @param path Chemin pour stocker le fichier à l'extérieur.
 * @param name Nom du fichier à copier dans
 * @return
 */
int copyin(const char *path, char name[]) {
    if (bitmap == NULL) {
        printf("No disk mounted, please mount first\n");
        return 0;
    }

    fs_touch(name);
    int offset = fs_dir_lookup(curr_dir, name);
    if (offset == -1) {
        return 0;
    }

    if (curr_dir.table[offset].type == 0) {
        return 0;
    }

    // Obtenir l'inode du fichier créé
    uint32_t inum = curr_dir.table[offset].inum;
    FILE *file;
    // Ouvrir le fichier pour la lecture
    file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Unable to open %s: %s\n", path, strerror(errno));
        return 0;
    }

    // Lire le fichier et obtenir les données
    char buffer[4 * BUFSIZ] = {0};
    offset = 0;
    while (1) {
        int result = fread(buffer, 1, sizeof(buffer), file);
        if (result <= 0) {
            break;
        }

        // Sauvegarder le fichier
        int actual = fs_write(inum, buffer, result, offset);
        if (actual < 0) {
            fprintf(stderr, "fs.write returned invalid result %d\n", actual);
            break;
        }

        offset += actual;
        if (actual != result) {
            fprintf(stderr, "fs.write only wrote %d bytes, not %d bytes\n", actual, result);
            break;
        }
    }

    printf("%d bytes copied\n", offset);
    fclose(file);
    return 1;
}

/**
 * Copie le fichier dans le répertoire courant vers le chemin fourni.
 *
 * @param name
 * @param path
 * @return
 */
int copyout(char name[], const char *path) {
    if (bitmap == NULL) {
        printf("No disk mounted, please mount first\n");
        return 0;
    }

    //Obtenir le décalage de la table pour le nom de fichier
    int offset = fs_dir_lookup(curr_dir, name);
    if (offset == -1) {
        return 0;
    }

    if (curr_dir.table[offset].type == 0) {
        return 0;
    }

    uint32_t inum = curr_dir.table[offset].inum;

    // Ouvrir le fichier pour le copier
    FILE *file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "Unable to open %s: %s\n", path, strerror(errno));
        return 0;
    }

    // Lire depuis l'inode et l'écrire dans le fichier
    char buffer[4 * BUFSIZ] = {0};
    offset = 0;
    while (1) {
        int result = fs_read(inum, buffer, sizeof(buffer), offset);
        if (result <= 0) {
            break;
        }
        fwrite(buffer, 1, result, file);
        offset += result;
    }

    printf("%d bytes copied\n", offset);
    fclose(file);
    return 1;
}
