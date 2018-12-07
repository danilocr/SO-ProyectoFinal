#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define RESET   "\x1b[0m"

union semun {
	int              val;   
	struct semid_ds *buf;   
	unsigned short  *array; 
	struct seminfo  *__buf;  
};

//Variables globales
int sem0,sem1,sem2,sem3,sem4,sem5;

//Estructura de la memoria compartida
typedef struct {
	int est;
	int cont;
	int tam;
	int idSem;
}servicioH;

//Métodos para el manejo de semáforos
void BloquearSemaforo(int id, int i);
void DesbloquearSemaforo(int id, int i);
int CrearSemaforos(int n, short* vals);

//Método 
void entraHombre(servicioH* aux);
void saleHombre(servicioH* aux);
void entraMujer(servicioH* aux);
void saleMujer(servicioH* aux);
void estado(int n);

//Método principal
void main(int argc, char* argv[])
{
	//Variables de procesos pesados
	key_t Clave;
	int Id_Memoria;
	int idSem;
	//Variable que seŕa la memoria compartida
	servicioH* aux;
	//Código para crear memoria compartida
	
	//Obtenemos la clave de memoria compartida
	Clave = ftok ("/bin/ls", 33);
	if (Clave == -1){
		printf("No consigo clave para memoria compartida");
		exit(0);
	}

	//Obtenemos la id de la meoria compartida
	Id_Memoria = shmget (Clave, sizeof(servicioH), 0777 | IPC_CREAT);
	if (Id_Memoria == -1){
		printf("No consigo Id para memoria compartida");
		exit (0);
	}

	//Creamos la memoria compartida
	aux = (servicioH *)shmat (Id_Memoria, (char *)0, 0);
	if (aux == NULL){
		printf("No consigo memoria compartida");
		exit (0);
	}

	//Si existe un argumento este será el tamaño del array
	if(argc == 2){
		printf("Esperamos a que ingresen personas\n");	
		short vals[6];
		aux->est = 0;
		aux->cont = 0;
		vals[0] = 0;
		vals[1] = 0;
		vals[2] = 0;
		vals[3] = 0;
		vals[4] = 0;
		vals[5] = 0;
		//Asignamos semáforos
		sem0=0;
		sem1=1;
		sem2=2;
		sem3=3;
		sem4=4;
		sem5=5;
		//Creamos semáforos 
		idSem=CrearSemaforos(4,vals);	
		aux->idSem = idSem;
		//Asignamos el argumento al tamaño del baño
		int c = atoi(argv[1]) ;
		aux->tam = c;
	}
	//Se usa para el otro proceso y desbloquear al primer proceso
	else{
		sem0=1;
		sem1=0;
		sem2=3;
		sem3=2;
		sem4=5;
		sem5=4;
		DesbloquearSemaforo(aux->idSem, sem1);
	}
	// Se ejecuta en el segundo proceso - la entrada de personas
	if(sem0 == 1){
		printf("COMENZARON A ENTRAR PERSONAS \n");
		entraHombre(aux);sleep(1);
		entraHombre(aux);sleep(1);
		entraHombre(aux);sleep(1);
		//hay un cambio
		entraMujer(aux);sleep(1);
		entraMujer(aux);sleep(1);
		//cambiamos
		entraHombre(aux);sleep(1);
		entraHombre(aux);sleep(1);
		entraMujer(aux);sleep(1);
		entraHombre(aux);sleep(1);
		entraHombre(aux);sleep(1);
		entraHombre(aux);sleep(1);
		entraHombre(aux);sleep(1);
		entraHombre(aux);sleep(1);
		sleep(9);//Suponemos otra entrada
		entraHombre(aux);
	}
	// Ejecuta el primer proceso y hace que se desocupe el baño
	if(sem0 == 0){
		while(1){	
			//Verifica que no este vacio, si lo esta espera que exista un entrada
			if(aux->est == 0){
				printf(MAGENTA"esta bloqueado porque esta vacio\n" RESET);
				BloquearSemaforo(aux->idSem, sem1);
			}
			//Desocupa el baño
			sleep(3);
			if(aux->est == 1)
				saleHombre(aux);
			if(aux->est == 2)
				saleMujer(aux);
			estado(aux->est);
		}
	}
	//Cerramos la memoria compartida
	shmdt ((char *)aux);
	shmctl (Id_Memoria, IPC_RMID, (struct shmid_ds *)NULL);
}

//Método para crear semáforos
int CrearSemaforos(int n, short* vals){
  union semun arg; 
  int id;

  assert(n > 0); 
  assert(vals != NULL);

  id = semget(IPC_PRIVATE, n, SHM_R | SHM_W);

  arg.array = vals;
  semctl(id, 0, SETALL, arg);

  return id;
}
//Método para bloquear semaforo i
void BloquearSemaforo(int id, int i){
  struct sembuf sb;
  sb.sem_num = i;

  sb.sem_op = -1; 
  sb.sem_flg = SEM_UNDO; 
  semop(id, &sb, 1);
}
//Método para desbloquear semaforo i
void DesbloquearSemaforo(int id, int i){
  struct sembuf sb;
  sb.sem_num = i; 
  sb.sem_op = 1; 
  sb.sem_flg = SEM_UNDO; 
  semop(id, &sb, 1);
}
void entraHombre(servicioH * aux){
	// Si esta el cartel de mujeres activo entonces se detiene
	if(aux->est == 2){
		printf( YELLOW "Esperamos a que se libere los SH. Para que ingresen los hombres\n" RESET);	
		BloquearSemaforo(aux->idSem,sem3);
		aux->est = 0;
	}
	//Si el esta vacio cambia el cartel a hombres
	if(aux-> est == 0){
		printf("Cambiamos el cartel a HOMBRES\n");
		aux-> est = 1; // Lo cambiamos a hombres
		DesbloquearSemaforo(aux->idSem, sem0);
	}
	//Si esta lleno se activan espera a que exista un casillero vacio
	if(aux->cont == aux->tam){
		printf(BLUE "Los servicios higienicos estan llenos, espere\n"RESET);
		aux->cont--;
		BloquearSemaforo(aux->idSem,sem5);
		printf(BLUE "Existe un sitio libre\n"RESET);
	}
	//Imprimimos que el ingreso de hombre ha sido correcto y el número de persona ingresado
	aux->cont++;
	printf("Un hombre ha ingresado %d\n",aux->cont);
}

void saleHombre(servicioH* aux){
	//Si esta lleno desbloqua un sitio
	if(aux->cont == aux->tam){
		printf("Desbloqueamos porque estaba lleno, hombre\n");
		DesbloquearSemaforo(aux->idSem, sem4);
	}
	//Si se vacía se desbloquea el semaforo para que ingrese una mujer
	if(aux->cont == 0){
		printf(BLUE "Se tiene que cambiar el cartel a VACIO\n" RESET);	
		aux->est = 0;
		DesbloquearSemaforo(aux->idSem, sem2);
	}
	//De lo contrario sale un hombre del baño
	else{
		aux->cont--;
		printf("Un hombre ha salido\n");	
	}
}

void entraMujer(servicioH * aux){
	// Si esta el cartel de hombres activo entonces se detiene
	if(aux->est == 1){
		printf(YELLOW "Esperamos a que se libere los SH. Para que entren las mujeres\n" RESET);	
		BloquearSemaforo(aux->idSem,sem3);
		aux->est = 0;
	}
	//Si el esta vacio cambia el cartel a mujeres
	if(aux->est == 0){
		printf("Cambiamos el cartel a MUJERES\n");
		aux->est = 2; // Lo cambiamos a mujer
		DesbloquearSemaforo(aux->idSem, sem0);	
	}
	//Si esta lleno se activan espera a que exista un casillero vacio
	if(aux->cont == aux->tam){
		printf(BLUE "Los servicios higienicos estan llenos, espere\n"RESET);
		//se bloquea el acceso a la memoria compartida
		aux->cont--;
		BloquearSemaforo(aux->idSem,sem5);
		printf(BLUE"Existe un sitio libre\n"RESET);
	}
	//Imprimimos que el ingreso de mujer ha sido correcto y el número de persona ingresado
	aux->cont++;
	printf("Una mujer ha ingresdo %d \n",aux->cont);
}

void saleMujer(servicioH* aux){
	//Si esta lleno desbloqua un sitio
	if(aux->cont == aux->tam){
		printf("Desbloqueamos porque estaba lleno, mujer\n");
		DesbloquearSemaforo(aux->idSem, sem4);
	}
	//Si se vacía se desbloquea el semaforo para que ingrese un hombre
	if(aux->cont== 0){
		printf(BLUE "Se tiene que cambiar el cartel a VACIO\n" RESET);	
		DesbloquearSemaforo(aux->idSem, sem2);
		aux->est = 0;
	}
	//De lo contrario sale una mujer del baño
	else{
		aux->cont--;
		printf("Una mujer ha salido\n");	
	}

}
//Método para ver quien está ocupando el baño
void estado(int n){
	switch(n){
		case 0 : printf(RED "Los servicios higienicos esta vacio\n" RESET);break;
		case 1 : printf(YELLOW "Esta siendo usado por hombres\n"RESET);break;
		case 2 : printf(GREEN "Esta siendo usado por mujeres\n" RESET);break;
	}
}
