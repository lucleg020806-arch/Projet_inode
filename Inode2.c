#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ftw.h>
#include <errno.h>

#define DEMO_DIR "filesystem_demo"
#define BLOCK_SIZE 4096ULL

static unsigned long long total_taille_logique = 0;
static unsigned long long total_blocs_linux = 0;
static unsigned long long total_fichiers = 0;

unsigned long long nb_blocs(unsigned long long t, unsigned long long t_bloc) {
        if (t == 0) {
                return 0;
        } else {
                return (t + t_bloc - 1) / t_bloc;
        }
}

int creer_gros_f(const char *path, unsigned long long t) {
        FILE *f = fopen(path, "wb");
        if (f == NULL) {
                return -1;
        }
        unsigned char buffer[64 * 1024];
        memset(buffer, 0, sizeof(buffer));
        unsigned long long restant = t;
        while (restant > 0) {
                unsigned long long t_ecriture = restant < sizeof(buffer) ? restant : sizeof(buffer);
                if (fwrite(buffer, 1, (size_t)t_ecriture, f) != (size_t)t_ecriture) {
                        fclose(f);
                        return -1;
                }
                restant -= t_ecriture;
        }
        fflush(f);
        if (fsync(fileno(f)) != 0) {
                fclose(f);
                return -1;
        }
        fclose(f);
        return 0;
}

int t_log(const char *path, unsigned long long *out) {
        struct stat metadata;
        if (stat(path, &metadata) != 0) {
                return -1;
        }
        *out = (unsigned long long)metadata.st_size;
        return 0;
}

int blocs_alloues(const char *path, unsigned long long *out) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "stat -c %%b '%s'", path);

        FILE *pipe = popen(cmd, "r");
        if (pipe == NULL) {
                return -1; 
        }

        char text[256];
        if (fgets(text, sizeof(text), pipe) == NULL) {
                pclose(pipe);
                return -1;
        }
        int status = pclose(pipe);
        if (status != 0) {
                return -1; 
        }

        
        text[strcspn(text, "\n")] = '\0';

        char *endptr;
        unsigned long long val = strtoull(text, &endptr, 10);
        if (endptr == text) {
                return -1; 
        }
        *out = val;
        return 0;
}

unsigned long long esp_the(unsigned long long t, unsigned long long t_bloc) {
        return nb_blocs(t, t_bloc) * t_bloc;
}

int gros_f(void) {
        printf("1\n");

        if (mkdir(DEMO_DIR, 0755) != 0 && errno != EEXIST) {
                return -1;
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s/big_f.jpg", DEMO_DIR);

        unsigned long long t = 100ULL * 1024 * 1024;

        if (creer_gros_f(path, t) != 0) {
                return -1;
        }

        unsigned long long t_reelle_log;
        if (t_log(path, &t_reelle_log) != 0) {
                return -1;
        }

        unsigned long long esp_theorique = esp_the(t_reelle_log, BLOCK_SIZE);

        unsigned long long blocs_linux;
        if (blocs_alloues(path, &blocs_linux) != 0) {
                return -1;
        }

        unsigned long long esp_reel = blocs_linux * 512;

        printf("Taille logique        %llu octets\n", t_reelle_log);
        printf("Taille bloc           %llu octets\n", BLOCK_SIZE);
        printf("Espace theorique      %llu octets\n", esp_theorique);
        printf("Blocs alloues (512o linux) %llu\n",
                blocs_linux);
        printf("Espace reel alloue    %llu octets\n",
                esp_reel);

        return 0;
}
static int compter_cb(const char *fpath,
                      const struct stat *sb,
                      int typeflag,
                      struct FTW *ftwbuf)
{
        (void)fpath;
        (void)ftwbuf;

        if (typeflag == FTW_F) {
                total_fichiers++;
                total_taille_logique += (unsigned long long)sb->st_size;
                total_blocs_linux += (unsigned long long)sb->st_blocks;
        }

        return 0;
}

int inode(void)
{
        if (mkdir(DEMO_DIR, 0755) != 0 && errno != EEXIST) {
                return -1;
        }

        char dossier[1024];

        snprintf(dossier,
                 sizeof(dossier),
                 "%s/inode_d",
                 DEMO_DIR);

        if (mkdir(dossier, 0755) != 0 && errno != EEXIST) {
                return -1;
        }

        unsigned long long nb_f = 10000ULL;

        printf("creation de %llu files\n", nb_f);

        for (unsigned long long i = 0; i < nb_f; i++) {

                char path[1200];

                snprintf(path,
                         sizeof(path),
                         "%s/fichier_%06llu.txt",
                         dossier,
                         i);

                FILE *f = fopen(path, "w");

                if (f == NULL) {
                        return -1;
                }

                const char *contenu = "salut a tous";

                if (fwrite(contenu,
                           1,
                           strlen(contenu),
                           f) != strlen(contenu)) {

                        fclose(f);
                        return -1;
                }

                fclose(f);
        }

        total_taille_logique = 0;
        total_blocs_linux = 0;
        total_fichiers = 0;

        if (nftw(dossier, compter_cb, 64, FTW_PHYS) != 0) {
                return -1;
        }

        unsigned long long espace_theorique =
                total_taille_logique +
                total_fichiers * INODE_SIZE;

        unsigned long long espace_reel_dossier =
                total_taille_logique;

        unsigned long long espace_reel_disque =
                total_blocs_linux * 512ULL;

        printf("\n");
        printf("Nombre fichiers             %llu\n",
               total_fichiers);

        printf("Donnees reelles             %llu octets\n",
               total_taille_logique);

        printf("Espace theorique (+inodes)  %llu octets\n",
               espace_theorique);

        printf("Espace reel dossier         %llu octets\n",
               espace_reel_dossier);

        printf("Espace reel disque          %llu octets\n",
               espace_reel_disque);

        return 0;
}

static int remove_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
        (void)sb;
        (void)typeflag;
        (void)ftwbuf;
        int rv = remove(fpath);
        if (rv != 0) {
                perror(fpath);
        }
        return rv;
}

int reset_demo(void) {
        struct stat st;
        if (stat(DEMO_DIR, &st) == 0) {
                if (nftw(DEMO_DIR, remove_cb, 64, FTW_DEPTH | FTW_PHYS) != 0) {
                        return -1;
                }
        } else {
                printf("rien a supp\n");
        }
        return 0;
}

void afficher_menu(void) {
        printf("1 - gros fichier\n");
        printf("2 - plein petit fichier\n");
        printf("3 - reset\n");
        printf("4 - quitter\n");
}

int lire_choix(char *choix, size_t taille) {
        fflush(stdout);
        if (fgets(choix, (int)taille, stdin) == NULL) {
                return -1;
        }
      
        choix[strcspn(choix, "\n")] = '\0';
        return 0;
}

int main(void) {
        char choix[64];

        for (;;) {
                afficher_menu();

                if (lire_choix(choix, sizeof(choix)) != 0) {
                        return 1;
                }

                if (strcmp(choix, "1") == 0) {
                        if (gros_f() != 0) {
                                return 1;
                        }
                } else if (strcmp(choix, "2") == 0) {
                        if (inode() != 0) {
                                return 1;
                        }
                } else if (strcmp(choix, "3") == 0) {
                        if (reset_demo() != 0) {
                                return 1;
                        }
                } else if (strcmp(choix, "4") == 0) {
                        break;
                } else {
                        printf("err\n");
                }
        }

        return 0;
}