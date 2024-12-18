#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include "../../utils/redirection.h"
#include "../../utils/gestion.h"
#include "../../utils/extern.h"
#include "../../utils/ftype.h"
#include <linux/limits.h>
#include "../../utils/commande.h"
#include "../../utils/freeStruct.h"
#include "../../utils/exit.h"
#define ARG_MAX 512

int compte_occ(char *chaine, char *sous_chaine)
{
    int res = 0;
    char *c = strstr(chaine, sous_chaine);
    while (c != NULL)
    {
        c += strlen(sous_chaine);
        res++;
        c = strstr(c, sous_chaine);
    }
    return res;
}

void eleverSlash(char *path)
{
    size_t len = strlen(path);
    if (len > 0 && path[len - 1] == '/')
    {
        path[len - 1] = '\0'; // Supprime le dernier '/'
    }
}

//TODO A CHANGER PR QUE SOIT PR TT LES TYPES
int nouveau(char *ancienne, char *nouveau, commandeStruct *cmd)
{
    // if (cmd->type == CMD_EXTERNE || cmd->type == CMD_INTERNE)
    // {
        // printf("chui dans nouveau\n");
        int k = 0;
        while (cmd->cmdSimple->args[k] != NULL)
        {
            char *ancienne_cmd = strdup(cmd->cmdSimple->args[k]);
            char *a_changer = strstr(ancienne_cmd, ancienne);
            if (a_changer == NULL)
            {
                k++;
            }
            else
            {
                int occ_ancienne = compte_occ(cmd->cmdSimple->args[k], ancienne);
                int taille = strlen(cmd->cmdSimple->args[k]) - occ_ancienne * strlen(ancienne) + occ_ancienne * strlen(nouveau) + 1;
                char *realloue = realloc(cmd->cmdSimple->args[k], taille); 
                if (realloue == NULL)
                {
                    perror("Reallocation");
                    return 1;
                }
                cmd->cmdSimple->args[k] = realloue;
                char *prefixe = ancienne_cmd;
                cmd->cmdSimple->args[k][0] = '\0'; // pour pas qu'il soit à null
                while (a_changer != NULL)
                {
                    int taille_prefixe = strlen(prefixe) - strlen(a_changer);
                    if (taille_prefixe > 0)
                    {
                        strncat(cmd->cmdSimple->args[k], prefixe, taille_prefixe);
                    }
                    strcat(cmd->cmdSimple->args[k], nouveau);
                    prefixe = a_changer + strlen(ancienne);
                    a_changer = strstr(prefixe, ancienne);
                }
                strcat(cmd->cmdSimple->args[k], prefixe);
                // printf("dans nouveau\n cmd->cmdSimple->args[%d] = %s\n",k,cmd->cmdSimple->args[k]);
                k++;
            }
            if (ancienne_cmd != NULL)
                free(ancienne_cmd);
        }
    // }
    return 0;
}

int optionA(struct dirent *entry, cmdFor *cmdFor)
{
    return (rechercheDansArgs("-A", cmdFor->op) && entry->d_name[0] == '.' && entry->d_name[1] != '.' && entry->d_name[1] != '\0');
}

int arg_options(char **op, char *for_opt)
{
    for (int i = 0; op[i] != NULL; i++)
    {
        if (strcmp(op[i], for_opt) == 0)
        {
            return i + 1;
        }
    }
    return 0;
}

int option_e(struct dirent *entry, cmdFor *cmdFor)
{
    char *ext = cmdFor->op[arg_options(cmdFor->op, "-e")];
    char *basename = entry->d_name;
    char *dot = strrchr(basename, '.');

    if (dot != NULL && strcmp(dot + 1, ext) == 0 && basename[0] != '.') // pas un . a la 1ere pos
    {
        // printf("Nom du fichier: [%s]\n", basename);
        // printf("Extension: [%s]\n", ext);
        // printf("extension du fichier dot+1 : [%s]\n",dot+1);
        return 1;
    }
    return 0;
}

int option_t(struct dirent *entry, cmdFor *cmd)
{
    int type = entry->d_type;
    int indice_op = arg_options(cmd->op, "-t");
    int for_type = -1;
    if (indice_op != 0)
    {
        if (strcmp(cmd->op[indice_op], "f") == 0)
        {
            for_type = 8;
        }
        else if (strcmp(cmd->op[indice_op], "d") == 0)
        {
            for_type = 4;
        }
        else if (strcmp(cmd->op[indice_op], "l") == 0)
        {
            for_type = 10;
        }
        else if (strcmp(cmd->op[indice_op], "p") == 0)
        {
            for_type = 1;
        }
        else
        {
            return -1;
        }
        return type == for_type;
    }
    else
    {
        return -1;
    }
}

void print_arg(commandeStruct *cmd){
    int k = 0;
    while (cmd->cmdSimple->args[k] != NULL)
    {
        printf("cmd->cmdSimple->args[%d] = %s\n",k,cmd->cmdSimple->args[k]);
        k++;
    }
}


int option_r(struct dirent *entry, cmdFor *cmd)
{
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
    {
        char path[PATH_MAX];
        if (cmd->rep[strlen(cmd->rep) - 1] != '/')
        {
            if (snprintf(path, sizeof(path), "%s/%s/", cmd->rep, entry->d_name) >= PATH_MAX)
            {
                perror("chemin trop long");
                return 1;
            }
        }
        else
        {
            if (snprintf(path, sizeof(path), "%s%s/", cmd->rep, entry->d_name) >= PATH_MAX)
            {
                perror("chemin trop long");
                return 1;
            }
        }
        // faire une copie pour pas modifier les champs de cmd
        cmdFor cmdCopie = *cmd;
        // copier le chemin
        cmdCopie.rep = strdup(path);
        if (cmdCopie.rep == NULL)
        {
            perror("copie du chemin");
            return 1;
        }
        int ret = boucle_for(&cmdCopie);
        free(cmdCopie.rep);
        if (ret == 1)
        {
            perror("fontion for dans -r");
            return 1;
        }
        return 0;
    }
    return 0;
}




// TODO ERREUR DE SYNTAXE CODE ERREUR = 2
//  TODO Si ca ce passe mal ft faire un truc
// TODO JE FERME PAS LE REP ?
int boucle_for(cmdFor *cmdFor)
{
    int ret = 0; // TODO A CHANGER;
    DIR *dir = opendir(cmdFor->rep);
    if (dir == NULL)
    {
        fprintf(stderr, "command_for_run: %s\n", cmdFor->rep);
        ret = 1;
        return ret;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if ((entry->d_name[0] != '.' || optionA(entry, cmdFor)))
        {

            if (rechercheDansArgs("-e", cmdFor->op))
            {
                if (!option_e(entry, cmdFor))
                {
                    continue;
                }
            }

            if (rechercheDansArgs("-t", cmdFor->op))
            {
                int res = option_t(entry, cmdFor);
                if (res == 0)
                {
                    continue;
                }
                if (res == -1)
                {
                    dernier_exit = 1;
                    return 1;
                }
            }

            if (rechercheDansArgs("-r", cmdFor->op) && entry->d_type == DT_DIR)
            {
                ret = option_r(entry, cmdFor);
                if (ret == 1)
                    break;
                continue;
            }

            int nbr_cmd = 0;
            while (cmdFor->cmd->cmdsStruc[nbr_cmd] != NULL)
            {
                char *inter = malloc(strlen(cmdFor->variable) + 2); // ? CA C PR AVOIR LE BON NOM DE VARIABLE +2 pr $ et le char 0
                strcpy(inter, "$");
                strcat(inter, cmdFor->variable);

                char *path = malloc(strlen(entry->d_name) + strlen(cmdFor->rep) + 2); // +2 pr / et '\0'
                if (path == NULL)
                {
                    return 1;
                }
                strcpy(path, cmdFor->rep);
                if (cmdFor->rep[strlen(cmdFor->rep) - 1] != '/')
                {
                    strcat(path, "/");
                }

                if (cmdFor->cmd->cmdsStruc[nbr_cmd]->type == CMD_EXTERNE)
                {
                    if (entry->d_name != NULL)
                    {
                        char *c = strstr(entry->d_name, ".");
                        if (c != NULL && c != entry->d_name)
                        {
                            char *nom_sans_ext = malloc(strlen(entry->d_name) - strlen(c) + 1);
                            memset(nom_sans_ext, 0, strlen(entry->d_name) - strlen(c) + 1);
                            strncpy(nom_sans_ext, entry->d_name, strlen(entry->d_name) - strlen(c));
                            sprintf(entry->d_name, "%s", nom_sans_ext);
                            if (nom_sans_ext != NULL)
                                free(nom_sans_ext);
                        }
                    }
                }
                strcat(path, entry->d_name);
                // printf("path = %s\n",path);
                int n = nouveau(inter, path, cmdFor->cmd->cmdsStruc[nbr_cmd]);
                if (n!= 0){
                    perror("problème dans nouveau");
                    free_for(cmdFor);
                    return 1;
                }
                // print_arg(cmdFor->cmd->cmdsStruc[nbr_cmd]);
                ret = fsh("", &dernier_exit, cmdFor->cmd->cmdsStruc[nbr_cmd]);
                if (cmdFor->cmd->cmdsStruc[nbr_cmd] == NULL)
                {
                    perror("pb ds le changement de var");
                    free_for(cmdFor);
                    return 1;
                }
                char *ancienne = malloc(strlen(entry->d_name) + strlen(cmdFor->rep) + 2);
                strcpy(ancienne, cmdFor->rep);
                if (cmdFor->rep[strlen(cmdFor->rep) - 1] != '/')
                {
                    strcat(ancienne, "/");
                }
                strcat(ancienne, entry->d_name);
                char *dollar = malloc(strlen(cmdFor->variable) + 2); // ? CA C PR AVOIR LE BON NOM DE VARIABLE +2 pr $ et le char 0
                strcpy(dollar, "$");
                strcat(dollar, cmdFor->variable);
                // printf("ancienne = %s\n",ancienne);
                n = nouveau(ancienne, dollar, cmdFor->cmd->cmdsStruc[nbr_cmd]);
                if (n != 0){
                    perror("problème dans le 2ème appel de nv");
                    free_for(cmdFor);
                    return 1;
                }
                // if (cmdFor->cmd->cmdsStruc[nbr_cmd] == NULL)
                // {
                //     perror("pb ds le changement de var");
                //     free_for(cmdFor);
                //     // closedir(dir);
                //     return 1;
                // }
                // printf("-----------\n");
                nbr_cmd = nbr_cmd + 1;
                // printf("nbr_cmd = %d\n",nbr_cmd);
                // printf("-----------\n");
                if (dollar != NULL)
                    free(dollar);
                if (ancienne != NULL)
                    free(ancienne);
                if (path != NULL)
                    free(path);
                if (inter != NULL)
                    free(inter);
            }
        }
    }
    closedir(dir);

    return ret;
}