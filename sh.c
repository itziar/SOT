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
	gcc -c -Wall -Wshadow sh.c && gcc -o sh sh.o && ./sh
*/

const char delimiters[] = " \t"; //delimitadores para strtok
const int lon_cadena=512;
const char* PROMPT=">>";


//estructura de la shell comando argumentos
typedef struct Comando{
	char* command;
	int nargs;
	char* arg[];
}Comando;

//type error
typedef struct exit {
	int error;
	char* msg;
}Exit; 


int //devuelve el numero de espacios o tab
is_blank(char* cadena){
	int len_cadena;
	int blancos;
	len_cadena=strlen(cadena);
	blancos=0;
	char c=cadena[blancos];
	while((isblank(c)) && blancos<len_cadena){
		blancos++;
		c=cadena[blancos];
	};
	return blancos;
};

char*
remove_blank_before(char* cadena){
	int blancos;
	int lencadena=strlen(cadena);
	blancos=0;
	char c=cadena[blancos];
	while(!isblank(c)&& blancos<lencadena){
		blancos++;
		c=cadena[blancos];
	};
	cadena[blancos]='\0';
	return cadena;
};

//es var entorno
int
dollar(char c){
	return (c=='$');
};

char*
convert_dollar_var(char* cadena){
	char* valor;
	char* variable;

	valor=strchr(cadena,'$');
	if(valor!=NULL){
		valor[0]='\0';
		valor++;
		variable=getenv(valor);
		if(!variable){
			fprintf(stderr, "error: var %s does not exist\n", cadena );
			return NULL;
		}else{
			return variable;
		};
	};
	return NULL;
};


Comando*
separar_tokens(char* cadena, Comando* linea){
	char* saveptr;
	char* token;
	int i;
	char* pch;
	token=strtok_r(cadena, delimiters, &saveptr);
	
	if((pch=convert_dollar_var(token))){
		token=pch;
	};
	linea->command=token;

	i=0;
	linea->arg[i]=token;

	while(token){
		token=strtok_r(saveptr, delimiters, &saveptr);
		if(token){
			if((pch=convert_dollar_var(token))){
				token=pch;
			};
		};
		i++;
		linea->arg[i]=token;
	};
	return linea;
};



//variable de entrada =
int
var_ent(char* cadena){
	int loncadena=strlen(cadena);
	int i;
	//char c;
	for(i=0; i<loncadena; i++){
		if (cadena[i]=='='){
			return 1;
		};
	};
	return 0;
};

int
add_var(char* cadena){
	char* valor;
	char* temporal;
	int correct;
	int blank=0;

	valor=strchr(cadena,'=');
	if(valor!=NULL){
		valor[0]='\0';
		valor++;
		//quitar blancos
		blank=is_blank(cadena);
		cadena=cadena+blank;//aumentar directorio en blancos posiciones
		cadena=remove_blank_before(cadena);

		blank=is_blank(valor);
		valor=valor+blank;//aumentar directorio en blancos posiciones
		valor=remove_blank_before(valor);
		//
		//compruebo si hay $
		if(dollar(cadena[0])){
			temporal=convert_dollar_var(cadena);
			if(temporal==NULL){
				fprintf(stderr,"error: var %s does not exist\n",cadena);
				return -1;
			};
			cadena=strdup(temporal);
		};
		if(dollar(valor[0])){
			temporal=convert_dollar_var(valor);
			if(temporal==NULL){
				fprintf(stderr,"error: var %s does not exist\n",valor);
				return -1;
			};
			valor=strdup(temporal);
		};
		//añado la variable
		correct=setenv(cadena, valor,1);
		if(correct<0){
			fprintf(stderr, "error: cant save %s a %s\n", valor, cadena);
			return -1;
		};
		return 1;
	};
	return -1;
};
//


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
};

//CAMBIAR DIRECTORIO
int
change_directory(char* directory){
	return chdir(directory);
};

//PROGRAMA PRINCIPAL CD
void
cambio_directorio(Comando* comando){
	char* path;
	int error;
	char buf[1024];
	char dir[1024];

	
	if(comando->arg[1]!=NULL){
		if(dollar(comando->arg[1][0])){
			path=convert_dollar_var(comando->arg[1]);
			if(path!=NULL){
				fprintf(stderr, "Error: cant change directory %s\n", path);
			};
		};
		if(comando->arg[1][0]!='/'){
			getcwd(buf, sizeof(buf));
			sprintf(dir, "%s/%s", buf, comando->arg[1]);
			error=change_directory(dir);
			if(error){
				fprintf(stderr, "Error: cant change directory %s\n", dir);
			}
		}else{
			path=getEnv("HOME");
			sprintf(dir, "%s%s", path, comando->arg[1]);
			error=change_directory(dir);
			if(error){
				fprintf(stderr, "Error: cant change directory %s\n", dir);

			}
		};
	}else{
		path=getEnv("HOME");
		error=change_directory(path);
		if(error){
			fprintf(stderr, "Error: cant change directory %s\n", path);
		};	

		
	};
};

//FIN DE CD--------------------------------------------------------------------------------------

//background &
int
background(char* cadena){
	int loncadena=strlen(cadena);
	int i;
	//char c;
	for(i=0; i<loncadena; i++){
		if (cadena[i]=='&'){
			return 1;
		};
	};
	return 0;
};

int
redireccion_entrada(char* cadena){
	int loncadena=strlen(cadena);
	int i;
	for(i=0; i<loncadena; i++){
		if (cadena[i]=='<'){
			return 1;
		};
	};
	return 0;
};

int//comprobamos si hay que redirigir la entrada estandar devuelve 0 si no
comprobarRedirecEntrada(char *linea){
	int fd;
	char *p;
	char* pch;
	fd = 0;
	if( (p = strchr(linea,'<')) ){
		p[0] = '\0';
		p++;
		while( (p[0]==' ') || (p[0]=='\t')){
			p++;
		}
		if((pch=convert_dollar_var(p))){
			p=pch;
		};
		if((access(p,F_OK)) == 0){
			fd = open(p, O_RDONLY);
		}else{
			fprintf(stderr,"El fichero de entrada %s no existe.\n", p);
			return -1;
		}
	}

	return fd;
}

int
redireccion_salida(char* cadena){
	int loncadena=strlen(cadena);
	int i;
	//char c;
	for(i=0; i<loncadena; i++){
		if (cadena[i]=='>'){
			return 1;
		};
	};
	return 0;
};
void //quita &
background_del(char *linea){
	char *p;
	if( (p = strchr(linea,'&')) ){
		p[0] = '\0';
		p--;
		while( (p[0]==' ') || (p[0]=='\t') ){
			p[0] = '\0';
			p--;
		}
	};
}

int//comprobamos si hay que redirigir la salida estandar devuelve 1 si no
comprobarRedirecSalida(char *linea)
{   int fd;
	char *p, *paux;
	char* pch;
	fd = 1;
	if( (p = strchr(linea,'>')) ){
		p[0] = '\0';
		paux = p;
		paux--;
		while( (paux[0]==' ') || (paux[0]=='\t') ){
			paux[0] = '\0';
			paux--;
		}
		p++;
		while( (p[0]==' ') || (p[0]=='\t') ){
			p++;
		}
		if((pch=convert_dollar_var(p))){
			p=pch;
		};
		if( (access(p,F_OK)) == 0){
			if(unlink(p)<0){
				fprintf(stderr,"error al borrar el fichero");
				return 0;
			}
		}

		fd = open(p, O_CREAT|O_WRONLY, 0660);
		if(fd<0){
			fprintf(stderr,"error con open.\n");
			return 0;      
		} 
	}
	return fd;
}

char*
create_path(char* argv){
	char* listcommand[]={"/bin", "/usr/bin", "/usr/local/bin"};
	int no_ok=1;
	int i=0;
	int lon_listcom;
	int lon_com;
	int longtotal;
	char* path;
	char* commando;
	lon_com=strlen(argv);
	while(i<3 && no_ok){
		lon_listcom=strlen(listcommand[i]);
		longtotal=lon_listcom+lon_com+1;
		commando=malloc(longtotal);
		sprintf(commando, "%s/%s",listcommand[i], argv);
		no_ok=access(commando, X_OK);
		if(no_ok){ 
			free(commando);		
			i++;
		}else{
			path=strdup(commando);
			free(commando);
		};
	};
	return path;
};


int
forky(char* path, Comando* command, int fd_in, int is_pipe, int back){
	int  sts;
	int pid;
	int fp[2];
	int null_out=0;
	if(is_pipe){	
		if(pipe(fp)<0){
			fprintf(stderr, "cannt create more pipes\n" );
			return -1;
		};
	};

	pid=fork();
	switch(pid){
	case -1:
		err(1, "cannt create more forks");
		break;
	case 0:
	  
		if(is_pipe){
			dup2(fd_in,0);
			close(fd_in);
			close(fp[0]);
			dup2(fp[1],1);
			close(fp[1]);

		};
		if(!is_pipe && back){
			null_out=open("dev/null", O_RDONLY);
			if(null_out<0){
				fprintf(stderr, "cant open null\n" );
			};
			dup2(null_out, 0);
			close(null_out);
		};
		execv (path, command->arg);
		err(1,"execv failed");
		break;
	default:
		if(!back){
			while(wait(&sts)!=pid){
				;
			};
		};
		if(is_pipe){
			close(fp[1]);	
		};
		return fp[0];
	};
};

int
writer(int fd_out, int fp){
	int w, nr;
	char  buf[1024];
	nr = read(fp, buf, sizeof(buf));
	while(nr!=0){
		if(nr<0){
			fprintf(stderr,"error read\n");
			return -1;
		};
		w = write(fd_out, buf, nr);
		if(w!=nr){
			fprintf(stderr,"no ha escrito bien\n");
			return -1;
		};
		nr = read(fp, buf, sizeof(buf));
	};
	return 1;
};


int
exet(char* cadena, Comando* linea, int fd_in, int is_pipe, int back){
	char* path;
	int fp;
	linea=separar_tokens(cadena, linea);
	if(strcmp(linea->command, "cd")==0){
		cambio_directorio(linea);
		return -2;
	}else{
		path=create_path(linea->command);
		fp=forky(path, linea, fd_in, is_pipe, back);
		return fp;
	};
	
};

//
void
procesar_in(char* cadena){
	Comando* linea;
	int back, red_in, red_out, is_pipe;
	int fd_in, fd_out, fp;
	is_pipe=0;
	fp=0;
	char* p;
	if(var_ent(cadena)){
		add_var(cadena);
	}else{
		back=background(cadena);
		if(back){
			//printf("tiene &\n");
			background_del(cadena);
		};
		
		red_out=redireccion_salida(cadena);
		if(red_out){ 
		//	printf("tiene >\n");
			fd_out=comprobarRedirecSalida(cadena);
			if(fd_out!=1){
				is_pipe=1;

			};
		}else{
			fd_out=1;
		};
		

		red_in=redireccion_entrada(cadena);
		if(red_in){
	//		printf("tiene <\n");
			fd_in=comprobarRedirecEntrada(cadena);
	
			if(fd_in){
				is_pipe=1;
			}
		}else{
			fd_in=0;
		};
		if(fd_in>=0 && fd_out>=0){
			while((p = strchr(cadena,'|'))){	
				is_pipe=1;
				p[0] = '\0';
				linea=(Comando*)malloc(512*sizeof(Comando));
				fd_in=exet(cadena, linea, fd_in, is_pipe, back);
				free(linea);
				p++;
				cadena=p;
			};
			linea=(Comando*)malloc(512*sizeof(Comando));
			fp=exet(cadena, linea, fd_in, is_pipe, back);
			if(is_pipe){
				writer(fd_out, fp);
			};
			if(fd_in){
				close(fd_in);
			};
			if(fd_out!=1){
				close(fd_out);
			};
			if(is_pipe){
				close(fp);
			};
			free(linea);
		};
	};
};

//MAIN DEL PROGRAMA PRINCIPAL--------------------------------------------------------------------

int 
main(int argc, char *argv[])
{
	int lon;
	char cadena[lon_cadena]; /* Un array lo suficientemente grande como para guardar la línea más larga del fichero */
	printf("%s ", PROMPT);
	while (fgets(cadena, lon_cadena, stdin) != NULL){
		lon=strlen(cadena);
		if(cadena==NULL && strcmp(cadena,"\n")==0) { // linea blanco
			continue;
		}else{
			//quitar \n
			if(cadena[lon-1]=='\n'){
				cadena[lon-1]='\0';
			};
			//
			printf("%s ", PROMPT);
			procesar_in(cadena);
		};
	};
	exit(EXIT_SUCCESS);
};