#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include "../../utils/commande.h"
#include "../../utils/gestion.h"
#include "../../utils/for.h"
#define ARG_MAX 512
size_t tailleArgs(char **args);

commandeStruct *remplissage_cmdStruct(Type type, cmd_simple *cmdSimple, cmd_pipe *pipestruct, cmdIf *cmdIfStruct, cmdFor *cmdForStruct , cmd_redirection* cmdredirection, int nbcommandes, commandeStruct *cmd)
{

    if (cmd == NULL)
    {
        cmd = malloc(sizeof(commandeStruct));
    }
    cmd->type = type;
    cmd->cmdSimple = cmdSimple;
    cmd->pipe = pipestruct;
    cmd->cmdIf = cmdIfStruct;
    cmd->cmdFor = cmdForStruct;
    cmd->cmdRed = cmdredirection;
    cmd->nbCommandes = nbcommandes;

    return cmd;
}
void freeCmdSimple(cmd_simple *cmd)
{
    if (cmd == NULL)
        return;

    if (cmd->args != NULL)
    {
        for (char **arg = cmd->args; *arg; ++arg)
        {
            if (*arg != NULL)
            {
                free(*arg);
            }
        }
        free(cmd->args);
    }
    free(cmd);
}

void freePipe(cmd_pipe *pipeStruct)
{
    if (!pipeStruct)
        return;
    for (int i = 0; i < pipeStruct->nbCommandes; i++)
    {
        free(pipeStruct->commandes[i]);
    }
    free(pipeStruct->commandes);
    free(pipeStruct);
}

void freeCmdStruct(commandeStruct *cmd)
{
    if (cmd != NULL)
    {
        if (cmd->cmdSimple != NULL)
        {
            freeCmdSimple(cmd->cmdSimple);
        }

        if (cmd->pipe != NULL)
        {
            freePipe(cmd->pipe);
        }
        free(cmd);
    }
}

cmd_simple *remplissage_cmdSimple(char **args)
{
    cmd_simple *cmd = malloc(sizeof(cmd_simple));
    // printf("dans remplissage cmd simple \n");

    if (cmd == NULL)
    {
        perror("malloc CommandSimple");
        return NULL;
    }
    int nbargs = 0;
    while (args[nbargs])
    {
        // printf("args[%d] = %s\n",nbargs,args[nbargs]);
        nbargs++;
    }
    cmd->args = malloc((nbargs + 1) * sizeof(char *));
    if (cmd->args == NULL)
    {
        perror("malloc args");
        free(cmd);
        return NULL;
    }
    for (int i = 0; i < nbargs; i++)
    {
        cmd->args[i] = strdup(args[i]);
        // printf("args[%d] = %s\n",i,cmd->args[i]);
        if (!cmd->args[i])
        {
            perror("strdup");
            for (int j = 0; j < i; j++)
            {
                free(cmd->args[j]);
            }
            free(cmd->args);
            free(cmd);
            return NULL;
        }
    }
    cmd->args[nbargs] = NULL;

    if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "pwd") == 0 || strcmp(args[0], "ftype") == 0 || strcmp(args[0], "exit") == 0)
    {
        // printf("dans la condition pr type cmd interne\n");
        cmd->type = CMD_INTERNE;
    }
    else
    {
        cmd->type = CMD_EXTERNE;
    }
    return cmd;
}


size_t tailleArgs(char **args)
{
    size_t taille = 0;
    while (args[taille] != NULL)
    {
        taille++;
    }
    return taille;
}


int arg_cmdsimple_redirection(char **args, char **commande)
{
    size_t size = tailleArgs(args) - 2;
    for (int i = 0; i < size; i++)
    {
        commande[i] = strdup(args[i]);
        if (commande[i] == NULL)
        {
            perror("strdup arg_cmdsimple_redirection ");
            return 1;
        }
    }
    return 0;
}

void free_redirection(cmd_redirection *cmd)
{
    if (!cmd)
    {
        return;
    }
    else
    {
        if (cmd->cmd != NULL)
        {
            freeCmdSimple(cmd->cmd);
        }
        free(cmd);
    }
}

cmd_redirection *remplissageCmdRedirection(char **args)
{
    size_t taille = tailleArgs(args);
    cmd_redirection *cmd = malloc(sizeof(cmd_redirection));
    if (cmd == NULL)
    {
        perror("malloc");
        return NULL;
    }

    cmd->type = REDIRECTION;
    // TODO tableau dynmaique
    char *commande[10];
    memset(commande, 0, sizeof(commande));
    if (strstr(args[1], "<") != NULL)
    {
        cmd->fichier = args[0];
        cmd->separateur = "<";
        size_t size = taille - 2;
        for (int i = 0; i < size; i++)
        {
            commande[i] = strdup(args[taille - size + i]);
            if (commande[i] == NULL)
            {
                free_redirection(cmd);
                return NULL;
            }
        }
        cmd->cmd = remplissage_cmdSimple(commande);
    }
    else
    {
        if (arg_cmdsimple_redirection(args, commande) == 1)
        {
            perror("strdup cmd_redirection");
            free_redirection(cmd);
            return NULL;
        }
        size_t pos_sep = tailleArgs(commande);
        cmd->cmd = remplissage_cmdSimple(commande);
        if (strstr(args[pos_sep], ">") != NULL)
        {
            cmd->fichier = args[taille - 1]; // l'avant dernier élement vu que le dernier est NULL
            cmd->separateur = ">";
        }
        else if (strstr(args[pos_sep], " >> ") != NULL)
        {
            cmd->fichier = args[taille - 1];
            cmd->separateur = ">>";
        }
        else if (strstr(args[pos_sep], " 2> ") != NULL)
        {
            cmd->fichier = args[taille - 1];
            cmd->separateur = "2>";
        }
        else if (strstr(args[pos_sep], " >| ") != NULL)
        {
            cmd->fichier = args[taille - 1];
            cmd->separateur = " >| ";
        }
        else if (strstr(args[pos_sep], "2>|") != NULL)
        {
            cmd->fichier = args[taille - 1];
            cmd->separateur = "2>|";
        }
        else if (strstr(args[pos_sep], " 2>> ") != NULL)
        {
            cmd->fichier = args[taille - 1];
            cmd->separateur = "2>>";
        }
    }
    return cmd;
}

int arg_cmdsimple_pipe(char **args, char **commande, int i, int j)
{
    for (int h = 0; h < (i - j); h++)
    {
        commande[h] = strdup(args[j + h]);
        if (commande[h] == NULL)
        {
            return 1;
        }
    }
    commande[i-j] = NULL; // pour le dernier élementp
    return 0;
}

void free_pipe(cmd_pipe *cmd)
{
    for (int i = 0; i < cmd->nbCommandes; i++)
    {
        if (cmd->commandes[i] != NULL)
        {
            freeCmdSimple(cmd->commandes[i]);
        }
    }
    free(cmd);
}

cmd_pipe *remplissageCmdPipe(char **args)
{
    cmd_pipe *cmd = malloc(sizeof(cmd_pipe));
    cmd->commandes = malloc(40 * sizeof(cmd_simple));
    int nb = 0;
    int j = 0;

    // TODO tableau dynamique
    char *commande[10];
    memset(commande, 0, sizeof(commande));
    for (size_t i = 0; i <= tailleArgs(args); i++)
    {
        memset(commande, 0, sizeof(commande));
        if (args[i] == NULL)
        {
            if (arg_cmdsimple_pipe(args, commande, i, j) == 1)
            {
                free_pipe(cmd);
                return NULL;
            }
            cmd->commandes[nb] = remplissage_cmdSimple(commande);
            if (cmd->commandes[nb] == NULL)
            {
                perror("remplissage cmd simple dans remplissage pipe");
                free_pipe(cmd);
                return NULL;
            }
            nb += 1;
            j = i + 1;
        }
        else if (strcmp(args[i], "|") == 0)
        {
            if (arg_cmdsimple_pipe(args, commande, i, j) == 1)
            {
                free_pipe(cmd);
                return NULL;
            }
            cmd->commandes[nb] = remplissage_cmdSimple(commande);
            if (cmd->commandes[nb] == NULL)
            {
                perror("remplissage cmd simple dans remplissage pipe");
                free_pipe(cmd);
                return NULL;
            }
            nb += 1;
            j = i + 1;
        }
    }
    cmd->type = PIPE;
    cmd->nbCommandes = nb;
    cmd->commandes = (cmd_simple **)realloc(cmd->commandes, cmd->nbCommandes * sizeof(cmd_simple *));
    return cmd;
}
// si vous voulez teste les pipes 
// cat fichier.txt | sort | head -n 5 | ftype fichier.txt
//  cat fichier.txt | sort | head -n 5



// // type
// //   char* rep;
// //   char variable ; 
//  //int nbCommandes;
// commandeStruct** cmd;
// //  op

cmdFor* make_for(char ** args){
    // cmdFor *cmdFor = malloc(sizeof(*cmdFor));

    cmdFor *cmdFor = malloc(sizeof(*cmdFor));
    if (cmdFor == NULL){
        perror("problème d'allocation de mémoire pour for");
        return NULL;
    }
    //TODO j'ai commenté
    if (tailleArgs(args) < 8){
        perror("problème de syntaxe");
        printf("la taille de l'argument = %ld\n",tailleArgs(args));
        for (size_t i = 0; i<tailleArgs(args);i++){
            printf("%s\n",args[i]);
        }
        // free_for(cmdFor);
        return NULL;
    }
    
    cmdFor->rep = NULL;
    cmdFor->op = NULL;
    cmdFor->variable = NULL;
    cmdFor->cmd = NULL;

    //* --------- option----------
    // cmdFor->op = NULL;
    // cmdFor->nbCommandes = malloc(sizeof(int));
    // if (cmdFor->nbCommandes == NULL){
    //     free_for(cmdFor);
    // }
    // cmdFor->nbCommandes = 0;
    // ? -------- Type ---------
    cmdFor->type = FOR;

    // * ------------------ variable ---------------
    if (strlen(args[1]) != 1){
        perror("Erreur de syntaxe, la variabme doit contenir un seul caractère");
        // free_for(cmdFor);
        return NULL;
    }
    
    size_t taille = tailleArgs(args);
    // for (size_t i = 0 ; i <taille; i++){
    //     printf("args[%ld] = %s \n",i,args[i]);
    //     printf("taille %d \n",sizeof(args[i]));

    // }

    cmdFor->variable = strdup(args[1]);
    if (cmdFor->variable == NULL) {
        perror("erreur de duuuup");
        // free_for(cmdFor);
        return NULL;
    }
    
    cmdFor->rep = strdup(args[3]);
    // if (cmdFor->rep == NULL) {
    //     perror("erreur de duuuuuuuup");
    //     // free_for(cmdFor);
    //     return NULL;
    // }
    cmdFor->op = malloc(ARG_MAX*sizeof(char));
    if (cmdFor->op == NULL) {
        perror("aie aie aie");
        // free_for(cmdFor);
        return NULL;
    }
    // ? ----------------- option-----------
    int i = 4;
    int j = 0;
    while (strcmp(args[i],"{") != 0 ){
        printf("chui dans le while \n");
        if (strcmp(args[i],"-A") == 0 || strcmp(args[i],"-r") == 0 ){
            cmdFor->op[j]=args[i];
            i++;
            j++;
        }
        else if(strcmp(args[i],"-e") == 0 || strcmp(args[i],"-t") == 0 || strcmp(args[i],"-p") == 0 ){
            if (args[i+1][0]!='-'){
                cmdFor->op[j]=args[i]; //TODO a changer
                i=i+2;
                j++;
            }
            else{
                perror("il manque un argument");
                // free_for(cmdFor);
                return NULL;
            }
        }
        //TODO j'ai commenté
        // else{
        //     perror("ce n'est pas un argument valide");
        //     return NULL;
        // }
    }
    if (strcmp(args[i],"{") == 0){
        // printf("args[%d] = %s\n",i,args[i]);
        // printf("chui laaaa\n");
         i++; // pour sauter l'{
    }
    
    // printf("args[%d] = %s\n",i,args[i]);
    cmdFor->cmd = malloc(sizeof(commandeStruct));
    if (cmdFor->cmd == NULL){
        perror("pb d'alloc de sous cmd de for");
        // free_for(cmdFor);
        //TODO APPELER LES FREE
        return NULL;
    }

    char * tab[ARG_MAX];
    unsigned int k=0;
    while (args[i]!= NULL && i<taille && strcmp(args[i],"}") != 0){ //TODO ATTENTION PR LES CMD PLUS COMPLEXE LE STRCMP } PAS OUF
        tab[k]=args[i];
        // printf("args[%d] = %s et i = %d \n",i,args[i],i);
        // printf("tab[%d] = %s et i = %d \n",k,tab[k],i);
        k=k+1;
        i=i+1;
        // printf("tab[%d] = %s et i = %d \n",k,tab[k],i);
        // printf("args[%d] = %s et i = %d \n",i,args[i],i);
    }
    char *inter=NULL;
    inter=strdup(tab[0]);//TODO A CHANGER le probleme c que gestion prends un string au lieu d'un tableau du coup chui obligé de recoller tout le monde 
    char *c = " ";
    size_t len = strlen(inter) + strlen(c) + strlen(tab[1]) + 1;
    char *temp = realloc(inter,len);
    inter = temp;
    strcat(inter,c);
    strcat(inter,tab[1]);
    if (tab[2] != NULL){
        len = strlen(inter) + strlen(c) + strlen(tab[2]) + 1;
        temp = realloc(inter,len);
        inter = temp;
        strcat(inter,c);
        strcat(inter,tab[2]);
    }
    // printf("inter = %s",inter);
    // // for (int i = 0; i <k;i++){
    // //     printf(tab[i]);
    // //     printf("\n");
    // // }
    // // printf("inter = %s\n",inter);
    // // printf("dans make for var = %s\n",cmdFor->variable);
    cmdFor->cmd[0] = malloc(sizeof(commandeStruct));
    cmdFor->cmd[1] = NULL; // TODO A CHANGER si j'ai plusieurs commande ça ne marche pas hein
    gestion_cmd(inter,cmdFor->cmd[0]);
    // if (cmdFor->cmd==NULL){
    //     printf("chui BIEN NULLLE\n");
    //     return NULL;
    // }else{
    //     printf("chui pas null le bebe cdm du for\n");
    //     printf("le type dans make for du bébé cmd de for = %d\n",cmdFor->cmd[0]->type);
    //     // fflush(NULL);
    // }


    // boucle_for(cmdFor);
    return cmdFor;
}


void free_for(cmdFor *cmdFor){
    if (cmdFor->rep != NULL){
        free(cmdFor->rep);
    }
    if (cmdFor->variable !=NULL){
        free(cmdFor->variable);
    }
    // if (cmdFor->nbCommandes!=NULL){
    //     free(cmdFor->nbCommandes);
    // }
    if (cmdFor->op!= NULL){
        int i = 0;
        while(cmdFor->op[i]!=NULL){
            free(cmdFor->op[i]);
            i++;
        }
        free(cmdFor->op);
    }
    if (cmdFor->cmd!=NULL){
        int i = 0;
        while (cmdFor->cmd[i]!=NULL){
            free(cmdFor->op[i]);
            i++;
        }
        free(cmdFor->cmd);
    }
}



    // char *debut_variable = input + 4;
    // char *fin_variable = strstr(debut_variable, " in");
    // int ret = 0;

    // if (fin_variable != NULL)
    // {
    //     int len = fin_variable - debut_variable;
    //     if (len != 1)
    //     {
    //         perror("Erreur de syntaxe, la variable doit contenir un seul caractère");
    //         return 1;
    //     }
    //     cmdFor->variable = *debut_variable;
    // }
    // else
    // {
    //     perror("in attendu");
    //     ret=1;
    //     // return ret;
    // }

    // // ? --------------- répertoire --------------
    // char *debut_rep_opt = strstr(input, "in ");
    // char *fin_rep_opt = strstr(input, " {");
    // char *fin_cmd = strstr(input, " }");

    // if (fin_rep_opt == NULL || fin_cmd == NULL || debut_rep_opt == NULL)
    // {
    //     perror("Erreur de syntaxe");
    //     ret=1;
    //     // return ret;
    // }

    // debut_rep_opt += 3;
    // int len_rep = fin_rep_opt - debut_rep_opt;
    // char rep[len_rep + 1];
    // strncpy(rep, debut_rep_opt, len_rep);
    // rep[len_rep] = '\0';
    
    // // * ------------ extraction des commandes ------------
    // char *debut_cmd = fin_rep_opt + 2;
    // int len_cmd = fin_cmd - debut_cmd;
    // char commandes[len_cmd + 1];
    // strncpy(commandes, debut_cmd, len_cmd);
    // commandes[len_cmd] = '\0';

    // // TODO APPPELER LA FCT FSH SUR COMMANDES
    // // gestion_cmd(commandes,cmdFor->cmd);
    // gestion_cmd(commandes, *(cmdFor->cmd));

    // return ret;
// }