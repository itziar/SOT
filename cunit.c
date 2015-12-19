#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <err.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h> //para isblank espacio o tab
#include <stddef.h>

/*
	gcc -c -Wall -Wshadow cunit.c && gcc -o cunit cunit.o && ./cunit
*/


#define TST (".tst")
#define OUT (".out")
#define OK (".ok")

#define PWD getenv("PWD")


int
IsTerm(char* file, char* term){
	int lenfich, lenterm;

	lenfich=strlen(file);
	lenterm=strlen(term);
	if(lenfich<lenterm){
		return 0;
	}else{
		int dif=lenfich-lenterm;
		char* cadena=file+dif;
		if(strcmp(cadena, term)==0){
			return 1;
		}else{
			return 0;
		}
	}
}


int
FindFiles(char* directorio, char* arry[], char* term){
	DIR* dir;
	int closer;
	struct dirent* ent;
	struct stat s;
	char path[1024];
	int contador;
	contador=0;
	dir = opendir(directorio);
	if (dir==NULL){
		fprintf(stderr, "No se puede abrir el directorio %s\n", directorio);
		return -1;
	}
	if(dir!=NULL){
		while((ent=readdir(dir))!=NULL){
			if((strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, "..")!=0)){
				sprintf(path, "%s/%s", directorio, ent->d_name);
				stat(path, &s);
				if(!S_ISDIR(s.st_mode)){
					if(IsTerm(path, term)){
						//añado el path al array
						printf("%s\n", path);
						arry[contador]=strdup(path);
						contador++;
					}
				}
			}
		}
	}
	closer=closedir(dir);
	if(closer<0 && dir!=NULL){
		fprintf(stderr, "No se ha podido cerrar el directorio\n");
		return -1;
	}
	return contador;
}




//MAIN DEL PROGRAMA PRINCIPAL--------------------------------------------------------------------

int 
main(int argc, char *argv[])
{
	argc--;
	argv++;
	char* arry[20];
	int nfiles;
	if (argc==1){
		if (strcmp(argv[0],"-c")==0){
			nfiles=FindFiles(PWD, arry, OUT);
			printf("Borrar %d ficheros .out\n", nfiles);
			nfiles=FindFiles(PWD, arry, OK);
			printf("Borrar %d ficheros .ok\n", nfiles);
			exit(EXIT_SUCCESS);
		}
	};
	nfiles=FindFiles(PWD, arry, TST);
	if(nfiles<0){
		printf("No hay ningun fichero .tst\n");
		exit(EXIT_SUCCESS);
	}
	exit(EXIT_SUCCESS);
};