/*
Program that simulates a console
Team:
Sabrina Santana A01025397
Rubén Sánchez A01021759

Consola:  Algunas partes de la actividad la realizamos en conjunto con el equipo de
  Octavio Garduza y Christian Dalma:
 - Manejo de señales de SIGTSTP y SIGINT
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

//Puertos (escritura y lectura)
#define TCP_PORTE 8000
#define TCP_PORTL 9000
//Número de semáforos
#define S 4

//Variables globales
char buffer[1000];
int clientelector, clienteescritor;
pid_t pid;
int todosEnRojo=0;
int todosEnAmarillo=0;

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

//Gestor para control c
void gestor_ctrlc(int sid)
{
    printf("Mensaje de emergencia. Todos a intermitente (amarillo).\n");
    //Si es proceso hijo
    if(pid==0){
        //Si los semáforos no estaban en amarillo
        if(todosEnAmarillo==0){
            itoa(1, buffer);
            todosEnAmarillo=1;
        }
        //Si los semáforos están en amarillo
        else{
            itoa(2, buffer);
            todosEnAmarillo=0;
        }
        //Enviar el color al que cambiarán los semáforos
        write(clienteescritor, &buffer, sizeof(buffer));
    }
}

//Gestor para control z
void gestor_ctrlz(int sid)
{
    printf("Mensaje de emergencia. Todos a rojo.\n");
    //Si es proceso hijo
    if(pid==0){
        //Si los semáforos no estaban en rojo
        if(todosEnRojo==0){
            itoa(0, buffer);
            todosEnRojo=1;
        }
        //Si los semáforos están en rojo
        else{
            itoa(2, buffer);
            todosEnRojo=0;
        }
        //Enviar el color al que cambiarán los semáforos
        write(clienteescritor, &buffer, sizeof(buffer));
    }
}

//Main
int main(int argc, const char * argv[])
{
    //Variables
    struct sockaddr_in direccionL;
    struct sockaddr_in direccionE;
    int lector, escritor;

    ssize_t leidos;
    socklen_t escritoslector, escritosescritor;
    int continuar = 1;

    int contador = 0;
    int resp;

    //Validar argumentos
    if (argc != 2) {
        printf("Use: %s IP_lector \n", argv[0]);
        exit(-1);
    }
    printf("Consola de semáforos\n");

    //Establecer gestores
    signal(SIGINT, gestor_ctrlc);
    signal(SIGTSTP, gestor_ctrlz);

    //Crear sockets
    lector = socket(PF_INET, SOCK_STREAM, 0);
    escritor = socket(PF_INET, SOCK_STREAM, 0);

    //Enlace con el socket de lectura
    inet_aton(argv[1], &direccionL.sin_addr);
    direccionL.sin_port = htons(TCP_PORTL);
    direccionL.sin_family = AF_INET;

    //Enlace con el socket de escritura
    inet_aton(argv[1], &direccionE.sin_addr);
    direccionE.sin_port = htons(TCP_PORTE);
    direccionE.sin_family = AF_INET;

    bind(lector, (struct sockaddr *) &direccionL, sizeof(direccionL));
    bind(escritor, (struct sockaddr *) &direccionE, sizeof(direccionE));

    // Escuhar
    listen(lector, 10);
    listen(escritor, 10);

    escritoslector = sizeof(direccionL);
    escritosescritor = sizeof(direccionE);

    // Aceptar conexiones
    while (continuar)
    {
        clientelector = accept(lector, (struct sockaddr *) &direccionL, &escritoslector);
        clienteescritor = accept(escritor, (struct sockaddr *) &direccionE, &escritosescritor);

        printf("Aceptando conexiones en %s:%d \n",
               inet_ntoa(direccionL.sin_addr),
               ntohs(direccionL.sin_port));
        printf("Aceptando conexiones en %s:%d \n",
               inet_ntoa(direccionE.sin_addr),
               ntohs(direccionE.sin_port));

        pid = fork();

        if (pid == 0) continuar = 0;

    }

    if (pid == 0) {
        //Establecer gestores
        signal(SIGINT, gestor_ctrlc);
        signal(SIGTSTP, gestor_ctrlz);

        close(lector);
        close(escritor);

        if (clientelector >= 0 && clienteescritor >=0) {
            //Leer pid de semáforo
            read(clientelector, &buffer, sizeof(buffer));
            printf("Pid recibido: %s \n ",buffer);
            int spid = atoi(buffer);

            //Esperar mensaje de semáforo
            while (leidos = read(clientelector, &buffer, sizeof(buffer))) {
                    //Pasar mensaje a int
                  resp=atoi(buffer);

                switch(resp){
                    //Si mensaje es "rojo"
                    case 0:
                        printf("El semáforo %d cambió a rojo.\n", spid);
                        break;
                    //Si mensaje es "amarillo"
                    case 1:
                        printf("El semáforo %d cambió a amarillo.\n", spid);
                        break;
                    //Si mensaje es "verde"
                    case 2:
                        printf("El semáforo %d cambió a verde.\n", spid);
                        printf("El semáforo %d está en verde.\n", spid);
                        printf("Los otros 3 semaforos en rojo\n");
                        break;
                    default:
                        printf("Color no válido.\n");
                }
            }

        }
        //Cerrar
        close(clientelector);
        close(clienteescritor);
    }

    else if (pid > 0)
    {
        while (wait(NULL) != -1);

        // Cerrar sockets
        close(lector);
        close(escritor);

    }
    return 0;
}
