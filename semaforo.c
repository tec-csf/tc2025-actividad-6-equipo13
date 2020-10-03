/*
Program that simulates a street light
Team:
Sabrina Santana A01025397
Rubén Sánchez A01021759

Algunas partes de la actividad la realizamos en conjunto con el equipo de
  Octavio Garduza y Christian Dalma:
 - Escribir los pids a un archivo, leer el archivo y guardarlos en un arreglo,
   buscar el pid del proceso siguiente al actual.
 - Implementación de un socket de escritura y uno de lectura.
*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

//Puertos (lectura y escritura)
#define TCP_PORTL 8000
#define TCP_PORTE 9000

//Variables globales
int estado;
int pidSigiente;
char buffer[1000];
int lector;
int escritor;
int siguiente;
int senialConsola;
int soyElVerde=0;

//Función para invertir string
void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

//Función para transformar int en string
void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

//Gestor para la alarma
void gestorAlarm(int sid)
{
    printf("Soy el semaforo con PID: %d. Los 30 segundos han terminado, mandaré una señal al proceso de la derecha\n", getpid());
    estado = 0; //Cambiar a rojo
    soyElVerde=0;//Este semáforo ya no es el que está en verde
    itoa(estado, buffer);
    printf("Mandando estado de rojo a consola ... \n");
    //Mandar estado a consola
    write(escritor, &buffer, sizeof(buffer));
    //KILL a siguiente semáforo
    kill(siguiente, SIGUSR1);
}

//Gestor para el gestor de SIGUSR1
void gestor(int sid)
{
    printf("Soy el semaforo con PID: %d. Recibí la señal SIGUSR1, voy a ponerme en verde\n", getpid());
    estado = 2; //Cambiar a verde
    soyElVerde=1; //El semáforo es el que está en verde, aún en caso de emergencia
    itoa(estado, buffer);
    printf("Mandando estado de verde a consola ... \n");
    //Mandar estado a la consola
    write(escritor, &buffer, sizeof(buffer));

    //Establecer gestor y activar alarma
    signal(SIGALRM, gestorAlarm);
    alarm(30);

}

//Main
int main(int argc, const char * argv[])
{
    //Variables
    FILE *fp;
    int i;

    char caracteres[10];
    int cont = 0;
    int arr[4];

    estado = 0; //Empezar en rojo
    //Establecer gestor
    signal(SIGUSR1, gestor);

    printf("PID del semaforo: %d\n", getpid());
    struct sockaddr_in direccionL;
    struct sockaddr_in direccionE;

    ssize_t leidos, escritoslector, escritosescritor;

    //Validar argumentos
    if (argc != 2) {
        printf("Use: %s IP_Servidor \n", argv[0]);
        exit(-1);
    }

    // Crear el socket
    lector = socket(PF_INET, SOCK_STREAM, 0);
    escritor = socket(PF_INET, SOCK_STREAM, 0);

    // Establecer conexión lectura y escritura
    inet_aton(argv[1], &direccionL.sin_addr);
    direccionL.sin_port = htons(TCP_PORTL);
    direccionL.sin_family = AF_INET;

    inet_aton(argv[1], &direccionE.sin_addr);
    direccionE.sin_port = htons(TCP_PORTE);
    direccionE.sin_family = AF_INET;

    escritoslector = connect(lector, (struct sockaddr *) &direccionL, sizeof(direccionL));
    sleep(2);
    escritosescritor = connect(escritor, (struct sockaddr *) &direccionE, sizeof(direccionE));

    if (escritoslector >= 0 && escritosescritor >= 0) {
        printf("Socket de lectura conectado a %s:%d \n",
               inet_ntoa(direccionL.sin_addr),
               ntohs(direccionL.sin_port));
        printf("Socket de escritura conectado a %s:%d \n",
               inet_ntoa(direccionE.sin_addr),
               ntohs(direccionE.sin_port));

        int pid = getpid();
        itoa(pid, buffer);

        //Escribir pid en archivo de texto
        fp = fopen ( "pids.txt", "a" );
        fprintf(fp,"%s",buffer);
        fprintf(fp,"%s","\n");
        fclose (fp);

        //Mandar pid a consola (servidor)
        write(escritor, &buffer, sizeof(buffer));

        //Esperar permiso para continuar
        printf("Cuando todos los semaforos estén creados, ponga un 0 para empezar:\n");
        printf("Importante: Poner 0 en el primer semaforo al ultimo\n");
        scanf("%d", &i);

        //Leer archivo de texto
        fp = fopen ( "pids.txt", "r" );

        //Obtener pid del siguiente semáforo
        while (feof(fp) == 0 && cont < 4)
        {
            fgets(caracteres,10,fp);
            arr[cont] = atoi(caracteres);
            cont++;
        }
        for(int indice=0;indice<4;indice++){
            if(arr[indice] == pid && indice != 3){
                siguiente = arr[indice+1];
            } else if(arr[indice] == pid){
                siguiente = arr[0];
            }
        }

        printf("Soy el semáforo %d\n", pid);
        printf("El que sigue es: %d\n", siguiente);

        //Si semáforo es el primer semáforo
        if(pid == arr[0]){
            raise(SIGUSR1);
        }

        fclose (fp);

        //Leer mensaje de la consola
        while ((leidos = read(lector, &buffer, sizeof(buffer)))) {
            //Color al cual cambiarse
            senialConsola=atoi(buffer);

            switch(senialConsola){
                  case 0:
                    printf("Recibí mensaje de ponerme en rojo.\n");
                    estado = 0; //Cambiar a rojo
                    itoa(estado, buffer);
                    write(escritor, &buffer, sizeof(buffer));
                    //Apagar alarma
                    alarm(0);
                    break;
                  case 1:
                    printf("Recibí mensaje de ponerme en intermitente.\n");
                    estado = 1; //Cambiar a amarillo
                    itoa(estado, buffer);
                    write(escritor, &buffer, sizeof(buffer));
                    //Apagar alarma
                    alarm(0);
                    break;
                  case 2:
                    //Si semáforo era el que estaba en verde
                    if(soyElVerde){
                        raise(SIGUSR1);
                    }
                    break;
                  default:
                    printf("Color no válido\n");
              }
        }
    }

    // Cerrar sockets
    close(lector);
    close(escritor);

    return 0;
}
