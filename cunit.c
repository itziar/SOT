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

char DELIMITERS[] = " \t"; //delimitadores para strtok

//estructura de la shell comando argumentos
typedef struct Comando{
	char* command;
	int nargs;
	char* arg[MAXFILES];
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
int
CreatFile(char* nameFile){
	int fd;
	fd=open(nameFile, O_RDWR|O_CREAT|O_TRUNC, 0660);
	if(fd<0){
		err(1, "Error al crear %s\n", nameFile);
	}
	return fd;

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

//es var entorno
int
dollar(char c){
	return (c=='$');
}

/* REEMPLAZA $ POR SU VALOR */
char*
ConvertDollarVar(char* palabra){
	char* valor;
	char* variable;

	valor=strchr(palabra,'$');
	if(valor!=NULL){
		valor[0]='\0';
		valor++;
		variable=getenv(valor);
		if(!variable){
			fprintf(stderr, "error: var %s does not exist\n", valor);
			return NULL;
		}else{
			return variable;
		}
	}
	return NULL;
}


/* COMANDO CD */
//IMPLEMENTACION CD------------------------------------------------------------------------------

//para cambiar a /HOME
char*
getEnv(char* argv){
	char* env;
	env=getenv(argv);
	if(env!=NULL){
		return env;
	}else{
		return(NULL);
	}
}

//CAMBIAR DIRECTORIO
int
change_directory(char* directory){
	return chdir(directory);
}

//PROGRAMA PRINCIPAL CD
void
ChangeDirectory(Comando* comando){
	char* path;
	int error;
	char buf[1024];
	char dir[1024];
	if(comando->arg[1]!=NULL){
		if(dollar(comando->arg[1][0])){
			path=ConvertDollarVar(comando->arg[1]);
			if(path!=NULL){
				fprintf(stderr, "Error: cant change directory %s\n", path);
			}
		}
		if(comando->arg[1][0]!='/'){
			getcwd(buf, sizeof(buf));
			sprintf(dir, "%s/%s", buf, comando->arg[1]);
			error=change_directory(dir);
			if(error){
				fprintf(stderr, "Error: cant change directory %s\n", dir);
			}
		}else{			
			error=change_directory(comando->arg[1]);
			if(error){
				fprintf(stderr, "Error: cant change directory %s\n", dir);

			}
		}
	}else{
		path=getEnv("HOME");
		error=change_directory(path);
		if(error){
			fprintf(stderr, "Error: cant change directory %s\n", path);
		}
	}
}

//FIN DE CD--------------------------------------------------------------------------------------


/* SEPARAR POR ESPACIOS */
Comando*
SepararTokens(char* linea, Comando* cadena){
	char* saveptr;
	char* token;
	int i;
	char* pch;
	char *aux=(char*) malloc(1024*sizeof(char));
	strcpy(aux,linea);
	
	token=strtok_r(aux, DELIMITERS, &saveptr);
	if((pch=ConvertDollarVar(token))){
		token=pch;
	}
	cadena->command=token;
	i=0;
	cadena->arg[i]=token;
	while(token!=NULL){
		token=strtok_r(saveptr, DELIMITERS, &saveptr);
		if(token){
			if((pch=ConvertDollarVar(token))){
				token=pch;
			}
		}
		i++;
		cadena->arg[i]=token;
	}
	return cadena;
}


int 
is_separadordospuntos(char character){
	return (character==':');
}

int
mytokenizedospuntos(char* str, char** args){
	int long_str, nargs, i, blanco;
	long_str=strlen(str);
	nargs=0;
	i=0;
	blanco=1;
	while(i<long_str){
		if(is_separadordospuntos(str[i])){
			if(!blanco){
				str[i]='\0';
				blanco=1;
			}
		}else{ //es caracter
			if(blanco){
				args[nargs]=&str[i];
				nargs++;
				blanco=0;
			}
		}
		i++;
	}
	return nargs;
}

char*
CreatePath(char* argv){
	char commando[1024];
	char* str=getenv("PATH");
	char* copia_path=(char*) malloc(1024*sizeof(char));	
	char* listcommand[1024];
	strcpy(copia_path, str);
	int nargs=mytokenizedospuntos(copia_path, listcommand);
	char buf[1024];
	int ok=1;
	int i=0;
	//sacar el directorio actual
	getcwd(buf, sizeof(buf));
	sprintf(commando,"%s/%s", buf, argv);
	ok=access(commando,X_OK);
	if(!ok){
		return strdup(commando);
	}
	while(i<nargs && ok){
		sprintf(commando, "%s/%s",listcommand[i], argv);
		ok=access(commando, X_OK);
		if(!ok){			
			return strdup(commando);
		}
		i++;
	}
	return NULL;
}

/*FORK PARA EJECUTAR EL COMANDO*/
int
forky(Comando* command, int fd_ant, int fd_out){
	int pid;
	int fp[2];
	char* path;
	dup2(fd_out, 2); //salida de error a fd_out
	if(pipe(fp)<0){
		err(1, "cant create more pipes \n" );
	}
	pid=fork();
	switch(pid){
	case -1:
		err(1, "cannt create more forks");		
	case 0:
		//es el primer comando entrada a dev/null
		if(fd_ant==0){
			int null_in=open("/dev/null", O_RDONLY);
			if(null_in<0){
				err(1, "cant open /dev/null");
			}
			dup2(null_in, 0);
			close(null_in);
		}else{
			dup2(fd_ant, 0);
			close(fd_ant);
		}
			
		close(fp[0]);
		dup2(fp[1],1);
		close(fp[1]);
		path=CreatePath(command->command);
		execv (path, command->arg);
		err(1,"execv failed");
	default:
		close(fp[1]);	
		return fp[0];
	}	
}

/* EJECUTA */
int
EjecutarLineas(char* cadena, Comando* linea, int fd_ant, int fd_out, int is_primero){	
	int fp;
	linea=SepararTokens(cadena, linea);
	if(strcmp(linea->command, "cd")==0){
		if(is_primero==0){
			ChangeDirectory(linea);
		//}else{
		//	fprintf(stderr, "El comando cd debe estar al principio de fichero\n" );
		}
		return -2;
	}else{
		fp=forky(linea, fd_ant, fd_out);
		return fp;
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
			err(1, "cannt read");
		if(nr > 0){
			write(fd_writer, buffer, nr);
		}		
	}
}

/* LEER LINEAS DEL FICHERO*/
void
ReadLines(char* archivo, int fd_out){
	FILE* fd_in;
	char milinea[MAXLINEA];
	char* linea=NULL;
	int lon;
	Comando* command;
	int fp=0;
	int is_primero=1;
	//abrir el archivo
	fd_in=fopen(archivo, "r");
	if (fd_in==NULL){
		err(1, "No se puede abrir el archivo %s\n", archivo);
		//retornar error
	}
	//leer linea a linea
	while(feof(fd_in)!=1){
		linea=fgets(milinea, sizeof(milinea), fd_in);
		if(linea!=NULL && strcmp(linea, "\n")!=0){ //lineas en blanco
			lon=strlen(linea);
			//quitar \n
			if(linea[lon-1]=='\n'){
				linea[lon-1]='\0';
			}
			//		
			command=(Comando*)malloc(512*sizeof(Comando));
			fp=EjecutarLineas(linea, command, fp, fd_out, is_primero);
			free(command);	
			is_primero=0;
		}	
	}
	ReadWrite(fp, fd_out);
	close(fp);
	close(fd_out);
	//cerrar el archivo
	if(fclose(fd_in)<0){
		err(1, "No se puede cerrar el archivo %s\n", archivo);
	}	
}

int
Comparate(char* file_out, char* file_ok){
	//para comparar comparo caracter a caracter
	FILE* fd_out;
	FILE* fd_ok;
	int equal;
	char ch_out, ch_ok;
	fd_out=fopen(file_out, "r");
	fd_ok=fopen(file_ok, "r");
	
	if(fd_out == NULL) {
		printf("Cannot open %s for reading ", file_out);
	}
	
	if(fd_ok == NULL) {
		printf("Cannot open %s for reading ", file_ok);
	}
	
	ch_out = getc(fd_out);
	ch_ok = getc(fd_ok);
 	
 	while((ch_out != EOF) && (ch_ok != EOF) && (ch_out == ch_ok)) {
		ch_out = getc(fd_out);
		ch_ok = getc(fd_ok);
	}
 
	if(ch_out == ch_ok){
		equal=1;
	}
	if (ch_out != ch_ok){
		equal=0;
	}
	
	fclose(fd_out);
	fclose(fd_ok);
	return equal;
}

/* CREAR EL FICHERO .OUT Y SI ESO .OK*/
int
check_ok(char* file_out , char* file_ok, int fd_out){
	int success, fd_ok;
	if(ExistsFile(file_ok)){
		success=Comparate(file_out, file_ok);
		if(success){
			return 1;
		}else{
			return 0;
		}
	}else{
		//creamos el archivo .ok
		fd_out=open(file_out, O_RDONLY);
		fd_ok=CreatFile(file_ok);
		//metemos el contenido de .out en .ok
		if(fd_out>0 && fd_ok>0){
			ReadWrite(fd_out, fd_ok);			
		}else{
			return 0;
		}
	}
	close(fd_ok);
	return 1;
}

void
forky_fichs(char* fichero_tst){
	char *file_out, *file_ok;
	int fd_out, test_correct;
	int pid;
	file_out=ExtensionFile(fichero_tst, OUT);
	file_ok=ExtensionFile(fichero_tst, OK);
	fd_out=CreatFile(file_out);


	pid=fork();
	switch(pid){
	case -1:
		err(1, "cant create more forks");
	case 0:
		ReadLines(fichero_tst, fd_out);
		exit(EXIT_SUCCESS);
	default:
		wait(NULL);
		//lo de los ficheros
		test_correct=check_ok(file_out, file_ok, fd_out);
		if(test_correct){
			printf("%s: Test correcto\n", fichero_tst );
		}else{
			printf("%s: Test incorrecto\n", fichero_tst);
		}
	}
	close(fd_out);
}

//MAIN DEL PROGRAMA PRINCIPAL--------------------------------------------------------------------

int 
main(int argc, char *argv[])
{
	
	argc--;
	argv++;
	char* arry[MAXFILES];
	int nfiles;
	int i;
	if (argc==1){
		if (strcmp(argv[0],"-c")==0){
			nfiles=FindFiles(PWD, arry, OUT);
			//printf("Borrar %d ficheros .out\n", nfiles);
			for(i=0; i<nfiles; i++){
				unlink(arry[i]);
			}
			nfiles=FindFiles(PWD, arry, OK);
			//printf("Borrar %d ficheros .ok\n", nfiles);
			for(i=0; i<nfiles; i++){
				unlink(arry[i]);
			}
			exit(EXIT_SUCCESS);
		}
	}
	nfiles=FindFiles(PWD, arry, TST);
	//char* str=getenv("PATH");
	//int nargs=mytokenizedospuntos(str, listcommand);
	if(nfiles<0){
		printf("No hay ningun fichero .tst\n");
		exit(EXIT_SUCCESS);
	}
	for(i=0; i<nfiles; i++){
		forky_fichs(arry[i]);
	}
	exit(EXIT_SUCCESS);
}