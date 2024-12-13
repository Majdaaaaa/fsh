#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include "../../utils/commande.h"
#include "../../utils/gestionStruct.h"
#define PATH_MAX 1024


int redirection(cmd_redirection *cmdredirect){
    int fd = -1  , copie_stdout = -1 , copie_stdin = -1;
    
    if (cmdredirect ==  NULL ) {
        perror("Structure redirection vide");
        return 1;
    }
    if(cmdredirect->cmd == NULL || cmdredirect->fichier == NULL || cmdredirect->separateur == NULL)
    {
       perror("Arguments pour la redirection manquants");
       return 1;
    }

    char *fichier = cmdredirect->fichier;
    char *separateur = cmdredirect->separateur;


    if(strcmp(separateur , ">") == 0)
    {
        fd = open(fichier , O_WRONLY | O_CREAT | O_TRUNC , 0644);
    }
    else if(strcmp(separateur , ">>") == 0)
    {
        fd = open(fichier , O_WRONLY | O_CREAT | O_APPEND , 0644);
    }
    else if(strcmp(separateur , "<") == 0)
    {
        fd = open(fichier , O_RDONLY);
    }


    if(fd<0){
        perror("Erreur lors de l'ouverture du fichier");
        return 1;
    }

    copie_stdout = dup(STDOUT_FILENO);
    copie_stdin = dup(STDIN_FILENO);

    if (copie_stdout < 0 || copie_stdin < 0) {
        perror("Erreur lors de la sauvegarde des descripteurs");
        close(fd);
        return 1;
    }


    if(strcmp(separateur , ">>") == 0 || strcmp(separateur , ">") == 0)
    {
       if(dup2(fd , STDOUT_FILENO) < 0){
        perror("Erreur lors de la duplication du fd");
        close(fd);
        close(copie_stdin);
        close(copie_stdout);
        return 1;
       } 
    }
    else if(strcmp(separateur , "<") == 0)
    {
        if(dup2(fd,STDIN_FILENO)<0){
            perror("Erreur lors de la duplication du fd");
            close(fd);
            close(copie_stdin);
            close(copie_stdout);
            return 1;
        }
    }

    close(fd);


    int dernier_exit = 0;

    char* chemin = malloc(PATH_MAX);
    if(chemin == NULL){
        perror("Erreur lors de l'allocation de mémoire");
        close(copie_stdin);
        close(copie_stdout);
        return 1;
    }
    if(getcwd(chemin,PATH_MAX)==NULL){
        perror("getcwd");
        free(chemin);
        close(fd);
        close(copie_stdin);
        close(copie_stdout);
        return 1;
    }
    fsh(chemin , &dernier_exit, remplissage_cmdStruct(cmdredirect->type,NULL,NULL,NULL,NULL,cmdredirect,1,NULL));

    //retablir les fds

     if (dup2(copie_stdout, STDOUT_FILENO) < 0) {
        perror("Erreur lors de la restauration de la sortie standard");
        close(copie_stdout);
        close(copie_stdin);
        return 1;
    }
    if (dup2(copie_stdin, STDIN_FILENO) < 0) {
        perror("Erreur lors de la restauration de l'entrée standard");
        close(copie_stdout);
        close(copie_stdin);
        return 1;
    }


    close(copie_stdin);
    close(copie_stdout);
    free(chemin);

    return 0;
    
}
/*int redirection(char* input){
    char * args[1000]; // le tableau d'argument qu'on va utiliser dans excvp 
    char * filename = NULL ; 
    int fd ; 
    int i = 0 ; 
    const char* separateur = " " ;
    int append = 0 ;
    // ls -l cmd > f1
    char *token = strtok(input, separateur);
    // token -> ls
    int inverse=0;

    /*on va commencer par separer la chaine input selon les espaces pour extraire le fichier vers lequel redirigier + la commande à executer

    while(token != NULL){
        if(strcmp(token,">")==0){  // token -> f1
            filename = strtok(NULL, separateur); //filname = f1
        }
        else if(strcmp(token,">>")==0){
            filename = strtok(NULL, separateur); //filname = f1
            append = 1 ; 
        }
        else if(strcmp(token,"<")==0){
            filename=strtok(NULL,separateur);
            inverse=1;

        }
        else{
            args[i] = token; /* [ls , -l] ici si c'est pas > ou >> on prend cmd et ses option 
            i++; 
        }

        // lire ce qu'il y a aprés 
        token = strtok(NULL,separateur);

    }
    args[i] = NULL ;

    if(filename != NULL){
        if (inverse){
            fd = open(filename, O_WRONLY);
        }else{
            if(append){
                fd = open(filename , O_WRONLY|O_CREAT|O_APPEND, 0644);
            }else{
                fd = open(filename , O_WRONLY|O_CREAT|O_EXCL, 0644);
            }
        }
        if(fd<0){
            perror("erreur lors de l'ouverture du fichier");
            return 1;
        }
    }else{
        perror("erreur fichier non specifié");
        return 1;
    }

    int avant;
    if (inverse){
         avant=dup(STDIN_FILENO);
        dup2(fd,STDIN_FILENO);
        close(fd);
    }else{
        // garder le df de la sortie standard pour la rétablir aprés 
        avant = dup(STDOUT_FILENO);  // comme si sortie_avant = STDOUT_FILENO 
        if(avant < 0){
            perror("dup");
            return 1; 
        }
        // faire en sorte que STDOUT_FILENO pointe vers le fichier fd
        dup2(fd,STDOUT_FILENO); // comme si STDOUT_FILENO = fd 
        close(fd);
        }
    
    // creer un fils qui va executer la commande 
    if(args[0] != NULL){

    switch (fork()){
        case -1:
            perror("erreur lors de la creation du fils");
            return 1 ; 
        case 0 :
            if(execvp(args[0] , args) < 0){
                perror("execvp"); 
                exit(1);
            }
        default :
            wait(NULL); // attends son fils
            //retablir la sortie
            if(filename != NULL){
                if(inverse){
                    if(dup2(avant,STDIN_FILENO)<0){
                        perror("aaaaammmsss dup");
                        close(avant);
                        return 1;
                    }
                }else{
                    if(dup2(avant, STDOUT_FILENO)<0){
                    perror("dup2");
                    close(avant);
                    return 1;
                }
                close(avant);
                }
            }
    }
}else{
    perror("erreur lors de l'execution de la commande ");
}

return 0;

}*/





    // char *buf=malloc(BUFFERSIZE);
        //     if (buf==NULL){
        //         perror("buffer nulle");
        //         return 1;
        // }
        // int r=read(fd,buf,BUFFERSIZE);
        // if (r<0){
        //     perror("erreur lecture");
        //     return 1;
        // }