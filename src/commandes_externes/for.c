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
#include "../../utils/gestionStruct.h"
#include "../../utils/freeStruct.h"
#include "../../utils/exit.h"
#include "../../utils/signaux.h"

int max = 0;

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

int nouveau_var(char *ancienne, char *nouveau, commandeStruct *cmd)
{
    if (cmd->type == CMD_EXTERNE || cmd->type == CMD_INTERNE)
    {
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
                    free(ancienne_cmd);
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
                strcat(cmd->cmdSimple->args[k], "\0");
                k++;
            }
            if (ancienne_cmd != NULL)
                free(ancienne_cmd);
        }
    }
    else if (cmd->type == FOR)
    {
        if (strcmp(cmd->cmdFor->rep, ancienne) == 0)
        {
            cmd->cmdFor->rep = realloc(cmd->cmdFor->rep, strlen(nouveau) + 1);
            sprintf(cmd->cmdFor->rep, "%s", nouveau);
        }
        nouveau_var(ancienne, nouveau, cmd->cmdFor->cmd);
    }
    else if (cmd->type == PIPE)
    {
        int l = 0;
        commandeStruct *inter_type;
        while (cmd->pipe->commandes[l] != NULL)
        {
            inter_type = remplissage_cmdStruct(cmd->pipe->commandes[l]->type, cmd->pipe->commandes[l], NULL, NULL, NULL, NULL, 0, NULL);
            nouveau_var(ancienne, nouveau, inter_type);
            l++;
        }
        if (inter_type != NULL)
        {
            freeCmdStruct(inter_type);
        }
    }
    else if (cmd->type == CMD_STRUCT)
    {
        for (int i = 0; i < cmd->nbCommandes; i++)
        {
            nouveau_var(ancienne, nouveau, cmd->cmdsStruc[i]);
        }
    }
    else if (cmd->type == IF)
    {

        nouveau_var(ancienne, nouveau, cmd->cmdIf->test);
        nouveau_var(ancienne, nouveau, cmd->cmdIf->commandeIf);
        if (cmd->cmdIf->commandeElse != NULL)
        {
            nouveau_var(ancienne, nouveau, cmd->cmdIf->commandeElse);
        }
    }
    else if (cmd->type == REDIRECTION)
    {
        if(cmd->cmdSimple->red->cmd->args != NULL){
            perror("pas null");
        }else{
            perror("null");
        }
        commandeStruct *inter = remplissage_cmdStruct(cmd->cmdSimple->red->cmd->type, cmd->cmdSimple->red->cmd, NULL, NULL, NULL, NULL, 0, NULL);
        if (inter == NULL)
        {
            perror("remplissage_cmdStruct");
            return 1;
        }

        // if (nouveau_var(ancienne, nouveau, inter) != 0)
        // {
        //     perror("nouveau_var");
        //     freeCmdStruct(inter);
        //     return 1;
        // }

        size_t nb_args = 0;
        perror("iciiiii");
        if (inter->cmdSimple == NULL || inter->cmdSimple->red == NULL || inter->cmdSimple->red->cmd == NULL || inter->cmdSimple->red->cmd->args == NULL)
        {
            perror("Invalid cmd structure");
            freeCmdStruct(inter);
            return 1;
        }

        while (inter->cmdSimple->red->cmd->args[nb_args] != NULL)
        {
            nb_args++;
        }

        cmd->cmdSimple->red->cmd->args = malloc((nb_args + 1) * sizeof(char *));
        if (cmd->cmdSimple->red->cmd->args == NULL)
        {
            perror("malloc");
            freeCmdStruct(inter);
            return 1;
        }

        for (size_t i = 0; i < nb_args; i++)
        {
            cmd->cmdSimple->red->cmd->args[i] = strdup(inter->cmdSimple->red->cmd->args[i]);
            if (cmd->cmdSimple->red->cmd->args[i] == NULL)
            {
                perror("strdup");
                for (size_t j = 0; j < i; j++)
                {
                    free(cmd->cmdSimple->red->cmd->args[j]);
                }
                free(cmd->cmdSimple->red->cmd->args);
                freeCmdStruct(inter);
                return 1;
            }
        }
        cmd->cmdSimple->red->cmd->args[nb_args] = NULL;

        if (cmd->cmdSimple->red->fichier != NULL && strstr(cmd->cmdSimple->red->fichier, "$") != NULL)
        {
            sprintf(cmd->cmdSimple->red->fichier, "%s", nouveau);
        }

        if (inter != NULL)
        {
            freeCmdStruct(inter);
        }
        perror("apres");
    }
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
        char *p = strstr(entry->d_name, ".");
        if (p != NULL)
        {
            if (entry->d_name[0] != p[0])
            {
                char *nom_sans_ext = malloc(strlen(entry->d_name) - strlen(p) + 1);
                memset(nom_sans_ext, 0, strlen(entry->d_name) - strlen(p) + 1);
                strncpy(nom_sans_ext, entry->d_name, strlen(entry->d_name) - strlen(p));
                sprintf(entry->d_name, "%s", nom_sans_ext);
                if (nom_sans_ext != NULL)
                    free(nom_sans_ext);
            }
        }
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
        return type == for_type; //! retourne 0 si c'est faux
    }
    else
    {
        return -1;
    }
}

void print_arg(commandeStruct *cmd)
{
    int k = 0;
    while (cmd->cmdSimple->args[k] != NULL)
    {
        printf("cmd->cmdSimple->args[%d] = %s\n", k, cmd->cmdSimple->args[k]);
        k++;
    }
}

int boucle_for(cmdFor *cmdFor);

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
        if (ret > max)
        {
            max = ret;
        }
        free(cmdCopie.rep);
        if (ret == 1)
        {
            perror("fontion for dans -r");
            return 1;
        }
        return 0;
    }
    return 1;
}

// TODO ERREUR DE SYNTAXE CODE ERREUR = 2
//  TODO Si ca ce passe mal ft faire un truc
// TODO JE FERME PAS LE REP ?
int boucle_for(cmdFor *cmdFor)
{

    int ret = -255; // TODO A CHANGER;
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
            if (rechercheDansArgs("-r", cmdFor->op) && entry->d_type == DT_DIR)
            {
                int r = option_r(entry, cmdFor);
                if (r == 1)
                    break;
                // continue ;
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
                    return 2;
                }
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
                strcat(path, entry->d_name);
                strcat(path, "\0");
                // printf("path = %s\n",path);
                int n = nouveau_var(inter, path, cmdFor->cmd->cmdsStruc[nbr_cmd]);
                if (n != 0)
                {
                    perror("problème dans nouveau");
                    free_for(cmdFor);
                    return 1;
                }

                perror("fsh for");
                ret = fsh("", &dernier_exit, cmdFor->cmd->cmdsStruc[nbr_cmd]);
                if (ret == -255)
                {
                    max = -255;
                }
                else if (ret > max)
                {
                    max = ret;
                }
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
                strcat(path, "\0");
                // printf("ancienne = %s\n",ancienne);
                n = nouveau_var(ancienne, dollar, cmdFor->cmd->cmdsStruc[nbr_cmd]);
                if (n != 0)
                {
                    perror("problème dans le 2ème appel de nv");
                    free_for(cmdFor);
                    return 1;
                }

                nbr_cmd = nbr_cmd + 1;

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
        // printf(" la valeur de retour du while est %d\n",ret);
    }
    closedir(dir);

    return ret;
}
