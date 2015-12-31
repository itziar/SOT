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


#define TST ".tst"
#define OUT ".out"
#define OK ".ok"

#define PWD getenv("PWD")

#define MAXFILES 20

#define MAXLINEA 256

const char DELIMITERS[] = " \t"; //delimitadores para strtok

//estructura de la shell comando argumentos
typedef struct Comando{
	char* command;
	int nargs;
	char* arg[];
}Comando;


/* COMPARAR FICHERO CON UNA TERMNACION */
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

/* ENCONTRAR FICHEROS CON UNA DETERMINADA TERMINACION*/
int
FindFiles(char* directorio, char* arry[], char* term){
	DIR* dir;
	int closer, contador;
	struct dirent* ent;
	struct stat s;
	char path[1024];
	
	contador=0;
	dir = opendir(directorio);
	if (dir==NULL){
		fprintf(stderr, "No se puede abrir el directorio %s\n", directorio);
		return -1;
	}
	while((ent=readdir(dir))!=NULL){
		if((strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, "..")!=0)){
			sprintf(path, "%s/%s", directorio, ent->d_name);
			stat(path, &s);
			if(!S_ISDIR(s.st_mode)){
				if(IsTerm(path, term)){
					//añado el nombre del fichero al array
					arry[contador]=strdup(ent->d_name);
					contador++;
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

/* ARCHIVO CON LA EXTENSION TERM*/
char*
ExtensionFile(char* fichero, char* term){
	char *nameFile, *saveptr, *token, *archivo;
	
	archivo=strdup(fichero);
	token=strtok_r(archivo, ".", &saveptr);
	nameFile = malloc (strlen(token)+strlen(term)+1);
	sprintf(nameFile,"%s%s",token, term);
	return nameFile;
}

/* CREAR ARCHIVO DADO */
void
CreatFile(char* nameFile){
	int fw;
	fw=creat(nameFile, 0660);
	if(fw<0){
		fprintf(stderr, "Error al crear %s\n", nameFile);
	}
}

/* EXITE O NO UN FICHERO DADO*/
int
ExistsFile(char* fichero){
	if(access(fichero, F_OK)<0){
		return 0; //no existe
	}else{
		return 1; //existe
	}
}

/* SEPARAR POR ESPACIOS */
Comando*
separar_tokens(char* linea, Comando* cadena){
	char* saveptr;
	char* token;
	int i;
	//char* pch;
	token=strtok_r(linea, DELIMITERS, &saveptr);
	
	/*if((pch=convert_dollar_var(token))){
		token=pch;
	};*/
	cadena->command=token;

	i=0;
	cadena->arg[i]=token;

	while(token){
		token=strtok_r(saveptr, DELIMITERS, &saveptr);
		/*if(token){
			if((pch=convert_dollar_var(token))){
				token=pch;
			};
		};*/
		i++;
		cadena->arg[i]=token;
	};
	return cadena;
};

/* EJECUTA */
void
EjecutarLinea(char* linea){
	//tokenizar la linea para que sea como quiero
	char* path;
	Comando* cadena;
	cadena=(Comando*)malloc(512*sizeof(Comando));
	cadena=separar_tokens(linea, cadena);
	int i=0;
	printf("%s\n", cadena->command);
	while(cadena->arg[i]!=NULL){
		printf("%s\n", cadena->arg[i]);
		i++;
	}
	/*if(strcmp(cadena->command, "cd")==0){
		cambio_directorio(linea);
		return -2;
	}else{
		path=create_path(linea->command);
		forky(path, linea);
		return fp;
	};
	ejecutar con forky*/
}

/* LEER LINEAS DEL FICHERO*/
void
ReadLines(char* archivo, char* file_out){
	FILE* fd_in;
	char milinea[MAXLINEA];
	char* linea=NULL;

	//abrir el archivo
	fd_in=fopen(archivo, "r");
	if (fd_in==NULL){
		fprintf(stderr, "No se puede abrir el archivo %s\n", archivo);
		//retornar error
	}
	//leer linea a linea
	while(feof(fd_in)!=1){
		linea=fgets(milinea, sizeof(milinea), fd_in);
		if(linea!=NULL){
			printf("linea\n" );
			EjecutarLinea(linea);
			printf("----\n");

		}	
	}
	//cerrar el archivo
	if(fclose(fd_in)<0){
		fprintf(stderr, "No se puede cerrar el archivo %s\n", archivo);
	}	
}


/* LEE Y ESCRIBE EN DESCRIPTORES DE FICHEROS*/
void
ReadWrite(int fd_reader, int fd_writer){
	int nr;
	char  buffer[1024];
	for(;;){
		nr = read(fd_reader, buffer, sizeof(buffer));
    	if(nr == 0)
			break;
		if(nr < 0)
			err(1, "bad reading");
		if(nr > 0)
			printf("%s\n", buffer);
			write(fd_writer, buffer, nr);
	}
}

/* CREAR EL FICHERO .OUT Y SI ESO .OK*/
void
ProcesarFichero(char* fichero){
	char *file_out, *file_ok;
	int fd_out, fd_ok;
	//creamos el archivo .out
	file_out=ExtensionFile(fichero, OUT);
	//CreatFile(file_out);
	//procesar fichero (abrir leer ejecutar y cerrar)
	ReadLines(fichero, file_out);
	file_ok=ExtensionFile(fichero, OK);
	if(ExistsFile(file_ok)){
		printf("existe\n");
		// if test correcto
//		file_ok=CreatFile(fichero);
		printf("%s: test correcto\n", fichero);
	//else 
		printf("%s: test incorrecto\n", fichero);
	}else{
		printf("no existe\n");
		//creamos el archivo .ok
		CreatFile(file_ok);
		//abrimos los ficheros para obtener los fd
		fd_out=open(file_out, O_RDONLY);
		fd_ok=open(file_ok, O_WRONLY);
		//metemos el contenido de .out en .ok
		if(fd_out>0 && fd_ok>0){
			ReadWrite(fd_out, fd_ok);
			//cerramos los fd
			if (close(fd_out)<0 || close(fd_ok)<0)
				printf("error al cerrar los descriptores\n");
		}else{
			printf("No se puede ni leer o escribir\n");
		}
	}
	
}


//MAIN DEL PROGRAMA PRINCIPAL--------------------------------------------------------------------

int 
main(int argc, char *argv[])
{
	argc--;
	argv++;
	char* arry[MAXFILES];
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
	// para cada fichero concurrentemente thread o fork? mejor thread
	ProcesarFichero(arry[0]);

	exit(EXIT_SUCCESS);
};