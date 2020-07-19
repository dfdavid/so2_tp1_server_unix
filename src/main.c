#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <sys/un.h>
#include <stdbool.h>
#include <zconf.h>
#include <signal.h>
#include <unistd.h>
#include <asm/errno.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>

#define CRED_LENGTH 20
#define BUFFER_SIZE 1024
#define SIM_CONECT 2
#define RETRY 3
#define NUM_USERS 2
#define DEFAULT_PATH "/tmp/socc"
#define FIRMWARE_FILE "../data/tp1_client_u"
#define FILE_BUFFER_SIZE 1500

int autenticar(char *user, char *password);
int getTelemetria(char *ipaddr);
int getScan(int sockfd);
int sendUpdate(int sockfd);
void get_dir();


struct Auth{
    char usr[CRED_LENGTH];
    char psw[CRED_LENGTH];
};



int main(int argc, char *argv[]) {

    struct Auth current_user;
    int sockfd, newsockfd;
    //int pid;
    socklen_t cli_length;
    char send_buffer[BUFFER_SIZE];
    memset(send_buffer, 0, sizeof(send_buffer));
    struct sockaddr_un st_serv, st_cli;
    signal(SIGPIPE, SIG_IGN); //


    //creacion del socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    //aca maneja el error si eventualmente el socket no se abre
    if (sockfd < 0) {
        perror("error al invocar 'socket' ");
        exit(1);
    }

    /* Limpieza de la estructura */
    memset(&st_serv, 0, sizeof(st_serv));
    /* Carga de la familia de direccioens */
    st_serv.sun_family = AF_UNIX;
    /* Carga del path del archivo especial del socket. No se permite asignar, por eso se hace con strncpy */
    if(argv[1] != NULL){
        strncpy(st_serv.sun_path,argv[1], sizeof(st_serv.sun_path));
    }
    else{
        char *path=DEFAULT_PATH;
        strncpy(st_serv.sun_path, path, sizeof(st_serv.sun_path));
    }


    //Antes del bind, se configuran las opciones del socket
    unlink(DEFAULT_PATH); //libero el archivo especial si es que existe previamente

    int socksize;
    unsigned int m = sizeof(socksize);
    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (void *) &socksize, &m); // obtengo el tamaño del buffer del socket por default y lo guardo en socksize
    printf("CONTROL: Tamaño del socket UN %i\n", socksize);


    //hacer el bind, (se une el socket con la estructura. Deben ser consistentes o fallara)
    //Upon successful completion, bind() returns 0. Otherwise, -1 is returned and errno is set to indicate the error.
    int bind_result;
    bind_result = bind(sockfd, (struct sockaddr *) &st_serv, sizeof(st_serv));
    if (bind_result < 0) {
        perror("binding error");
        exit(2);
    }


    //invocar a listen(int sockfd, int maxConections) Esta funcion Convierte al Socket en un socket servidor
    //Upon successful completions, listen() returns 0. Otherwise, -1 is returned and errno is set to indicate the error.

    int listening = listen(sockfd, SIM_CONECT);
    if (listening < 0) {
        perror("listening error");
    }

    //prompt 1
    //autenticacion
/*
    bool auth = false;
    int intentos=RETRY;
    memset(&current_user, 0, sizeof(current_user) );
    while(auth == false) {
        printf("Autenticacion necesaria \n");
        printf("Usuario: ");
        fgets(current_user.usr, sizeof(current_user.usr)-1, stdin);
        current_user.usr[strlen(current_user.usr)-1]='\0';

        printf("Password: ");
        fgets(current_user.psw, sizeof(current_user.psw)-1, stdin);
        current_user.psw[strlen(current_user.psw)-1]='\0';

        //si al llamar a autenticar obtengo 0 informo el error y descuento un intento
        if (autenticar(current_user.usr, current_user.psw) == 0) {
            intentos--;
            if(intentos==0){
                printf("Limite de intentos fallidos alncanzado \n");
                exit(3);
            }
            printf("Usuario o Password incorecto, queda/n %i intento/s \n", intentos);
        }

            // si el llamado a autenticar devuelve 1, el login es exitoso y escapo del bucle
        else {
            auth = true;
        }
    }//fin de la autenticacion*/
    printf("Login successfull...\n \n");

    cli_length = sizeof(st_cli);

    //prompt 2 (already logged)
    while (1) {
        printf("%s\\> Esperando una conexion... \n", current_user.usr);

        //Upon successful completion, accept() returns the nonnegative file descriptor of the accepted socket. Otherwise, -1 is returned and errno is set to indicate the error.
        //Acccept devuelve otro Fd, la dupla de func listen y accept permiten que el server pueda admitir a varios clientes. Del lado del cliente, se usa la funcion connect()
        // la func accept es bloqueante, se queda el hilo aca hasta que se reciba una coneccion
        newsockfd = accept(sockfd, (struct sockaddr *) &st_cli, &cli_length);

        if (newsockfd < 0) {
            perror("accept(): no se acepto la conexion");
            exit(1);
        }

        printf("%s\\> Conexion entrante... \n\n\n", current_user.usr);
        sleep(1);

        /*pid = fork();
        if (pid < 0) {
            perror("error al hacer fork");
        }
        else if (pid == 0) { //hijo
        */
        //close(sockfd); // una vez que se hizo el accept, solo se trabaja sobre el newsockfd
        bool terminar = false;
        while (terminar == false) {

            bool opt_valid = false;
            int opt = 0;
            while (opt_valid != true) { //menu de operaciones
                printf("\n\n");
                printf("    Seleccione una opcion: \n");
                printf("1 - Update Satellite Firmware \n");
                printf("2 - Start Scanning \n");
                printf("3 - Get Telemetry \n\n");
                printf("Opcion  ");
                scanf("%d", &opt);  //scanf ( tipo_de_dato_a_leer, puntero al dato)
                //fgets(opt, strlen(opt), stdin);
                if ( (opt == 1) || (opt == 2) || (opt == 3) ) { //los parentesis estan sugeridos por CPPCHECK
                    opt_valid = true;
                }
                else{
                    opt = 0;
                }
            }
            //printf("Ha elegido %d \n", opt);

            //switch por opciones
            switch (opt) {
                case 1:
                    memset(send_buffer, 0, sizeof(send_buffer));
                    printf("llamando a la funcion de envio...  \n");
                    strncpy(send_buffer, "1", 1);
                    if (send(newsockfd, send_buffer, sizeof(opt), 0) < 0) { //esto es un: che, preparate para recibir
                        perror("error al enviar solicitud de firmaware update");
                    }
                    memset(send_buffer, 0, sizeof(send_buffer));
                    sendUpdate(newsockfd);
                    //close(newsockfd);
                    //kill(getpid(), SIGINT); //el proceso hijo pierde contacto con su cliente y se auto detiene.

                    break;

                case 2:
                    memset(send_buffer, 0, sizeof(send_buffer));
                    strcpy(send_buffer, "2");
                    printf("sending scannig request... \n");
                    if (send(newsockfd, send_buffer, sizeof(send_buffer), 0) < 0) {
                        if (errno == EPIPE){
                            close(newsockfd);
                            printf("comunicacion con el cliente terminada. \n");
                            terminar=true;
                        }

                        perror("error al enviar solicitud de Start scaning");
                    }
                    //memset(send_buffer, 0, sizeof(send_buffer));
                    //getScan(newsockfd);
                    break;

                case 3:
                    //get telemetria
                    printf("Solicitando telemetria \n");
                    strncpy(send_buffer, "3", 1);
                    if (send(newsockfd, send_buffer, sizeof(send_buffer), 0) < 0) {
                        perror("error al enviar solicitud de get telemetry");
                    }
                    memset(send_buffer, 0, sizeof(send_buffer));
                    //getTelemetria( /*socket cliente*/ );
                    //printf("%s \n", inet_ntoa( st_cli.sin_addr )); // se le pasa ip en formato ascii la ip del cliente
                    break;

                default:
                    break;
            }

            /*if (strcmp(send_buffer, "fin\n") == 0) {
                terminar = true;
            }*/

        }//end while
        //}//end if "proceso hijo"

        //else {//proceso padre
        // https://linux.die.net/man/2/close
        close(newsockfd);
        //printf("El cliente cerro la conexion con el comando 'fin'  \n");
        //continue;
        //}
    }//end while "big server loop"
    return 0; //el programa del servidor no termina
}// end main


/**
 * @brief Indica si las credenciales de login insertadas son validas o no.
 * @param user Nombre de usuario registrado (credencial de usuario)
 * @param password Palabra clave asociada a un usuario registrado (credencial de usuario)
 * @return Si el par de credenciales pasadas matchean con un suario registrado devuelve 1, en otro caso devuleve 0.
 */
int autenticar(char *user, char *password){

    struct Auth users[NUM_USERS]= {  {"admin", "admin"} , {"david","dandrea747"}  };
    struct Auth check;

    strcpy(check.usr, user);
    strcpy(check.psw, password);
    for (int i=0; i< NUM_USERS; i++ ) {
        if (strcmp(check.usr, users[i].usr )==0 && strcmp(check.psw, users[i].psw)==0) return 1;
    }//end for
    return 0;

}//end autenticar

//funcion 3
/**
 * @brief Abre un socket UDP. Solicita al cliente conectado informacion de telemetria, la la recibe mediante la conexion UDP y la imprime en la consola.
 * @param ipaddr La direccion IP del programa cliente remoto almacenada en la estructura de la conexoin TCP, necesaria para iniciar la comunicacion por socket UDP.
 * @return devuelve 1 si se ha si se ha recibido el string 'udp_complete' desde el cliente
 */
int getTelemetria(char *ipaddr){
    printf("DEBUG: ud invoco la funcion get telemetria \n");

    int sockudp;
    /////////////////////struct sockaddr_in dest_addr;
    ////////socklen_t dest_size= sizeof(dest_addr);
    ////////////socklen_t *dest_addr_size_p=&dest_size;
    char bufferudp[BUFFER_SIZE];
    //char bufferudp2[BUFFER_SIZE];
    //char *word = null;


    //creacion del socket UDP
    sockudp= socket(AF_INET, SOCK_DGRAM, 0);
    if (sockudp < 0){
        perror("error creando socket UDP");
        exit(-2);
    }

    //limpieza y llendo de la estructura
    /* Limpieza de la estructura */
    //////////////////memset(&dest_addr, 0, sizeof(dest_addr));
    /* Carga de la familia de direccioens */
    ////////////dest_addr.sin_family=AF_INET;
    /* Carga del número de puerto. Se usa 'htons para darle el orden correcto a los bytes' */
    /////////////dest_addr.sin_port=htons(UDP_PORT);
    //Carga de dirección IPv4 del socket
    /*
     composicion de la estructura sockadd_in:
<<<<<<< HEAD
=======

>>>>>>> b85e7fad67b6061cecc0908a25ece3457b56234b
     struct sockaddr_in {
            short int sin_family;
            unsigned short int sin_port;
            struct in_addr sin_addr;
    };
<<<<<<< HEAD
=======

>>>>>>> b85e7fad67b6061cecc0908a25ece3457b56234b
    //para mas detalles sobre inet_aton consultar:
    //https://linux.die.net/man/3/inet_aton
    //int inet_aton(const char *cp, struct in_addr *inp);
    */
    ///////inet_aton(ipaddr, &dest_addr.sin_addr);
    /////////////printf("Direccion IP del host remoto: %s\n\n", inet_ntoa(dest_addr.sin_addr));
    printf("Formato de la telemetria: ID Sat | Uptime Sat | Firmware Sat | Free-RAM Sat\n");
    sleep(0.5);

    //se configuran las opciones del socket con setsocketopt(). Solo se hace bind del lado del srevidor. El servidor UDP es el satelite
    int socksize;
    unsigned int m= sizeof(socksize);
    //int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
    setsockopt(sockudp, SOL_SOCKET, SO_REUSEADDR, &m, sizeof(m)); // defino que la direccion del socket puede ser reutilizada por el SO

    memset(bufferudp, 0, sizeof(bufferudp));
    strcpy(bufferudp, "get_tel");
    sleep(2);
    ////////////sendto(sockudp, bufferudp, sizeof(bufferudp), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    //ssize_t sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);

    while( 1 ){
        if(strcmp(bufferudp, "udp_complete") == 0){
            break;
        }
        else if (strcmp(bufferudp, "udp_complete") != 0){
            memset(bufferudp, 0, sizeof(bufferudp));
            ///////////recvfrom(sockudp, bufferudp, sizeof(bufferudp), 0, (struct sockaddr *)&dest_addr, dest_addr_size_p );
            //ssize_t recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len);
            char telemetria[BUFFER_SIZE]="";

            strcpy(telemetria, bufferudp);
            printf("Telemetria: %s\n", telemetria);
            sleep(0.5);
        }
    }
    shutdown(sockudp, SHUT_WR);  //originalmente torce le puso 2 y no la macro
    close(sockudp);
    return 1;
}

//funcion 1
/**
 * @brief Envia al cliente una version del firmaware ya compilado y listo para ejecutar.
 * @param sockfd_arg El socket TCP de la comunicacion ya establecida entre cliente y servidor.
 * @return
 */
int sendUpdate(int sockfd_arg){
    //replicate example function
    int firmware_fd;
    struct stat wtf;
    char *filename=FIRMWARE_FILE;

    //intento abrir el archivo de firmaware en este caso:
    get_dir();
    if ( ( firmware_fd = open(filename, O_RDONLY) ) < 0 ){ //abro el binario
        perror("error al abrir el archivo de firmware");
        return -1; //
    }

    int count;
    char buffer_envio[BUFFER_SIZE];

    fstat(firmware_fd, &wtf); //wtf es un estructura "stat" que permite manipular datos dobre el archivo que voy a trabajar. En este caso es el binario del firmware
    off_t filesize = wtf.st_size; //off_t This is a signed integer type used to represent file sizes. In the GNU C Library, this type is no narrower than int.
    printf("DEBUG: tamaño del archivo a tranmitir: %ld\n", filesize);
    char bytes[sizeof(filesize)] = "";
    // snprinf (destino, size, formato, dato_entrada)
    snprintf(bytes, sizeof(filesize), "%ld", filesize);
    printf("DEBUG: n° de bytes a enviar: %s\n", bytes);

    //envio al cliente a traves del socket el tamaño del archivo a recibir
    memset(buffer_envio,0, sizeof(buffer_envio));
    strncpy(buffer_envio, bytes, sizeof(bytes));
    if ( send(sockfd_arg, buffer_envio, sizeof(buffer_envio), 0) < 0 ){
        perror("error al enviar al archivo");
    }

    //voy leyendo el archivo de a partes del tamano del buffer de envio y voy enviando
    while ( ( count=(int) read(firmware_fd, buffer_envio, BUFFER_SIZE) ) > 0 ){
         //ssize_t send(int socket, const void *buffer, size_t length, int flags);
         //voy enviando al socket el buffer de envio que es lo que lei recien
        if (send( sockfd_arg, buffer_envio, count, 0 ) < 0 ){
            perror("error enviando el archivo, (dentro del while)");
        }
        memset(buffer_envio, 0, BUFFER_SIZE);
    }

    close(firmware_fd); //cierro el archivo
    return 1; //return int 1: envio finalizado, no se garantiza la recepcion
}

//funcion2


/**
 * @brief Crea un archivo en el file system del SO del servidor con los datos enviados por el cliente.
 * @param sockfd_arg2 El sockfd de la comunicacion establecida entre servidor y cliente.
 * @return Devuelve 0 en caso de producirse algun error durante la creacion del archivo. Devuelve 1 en caso de que la recepcion del archivo sea correcta.
 *//*

int getScan(int sockfd_arg2){
    struct timeval inicio, fin;
    char *nombre_archivo=IMAGE_FILE;

    int imagen_fd;
    // int open(const char *path, int oflag/s, file_permissions );
    if ( ( imagen_fd=open(nombre_archivo, O_WRONLY|O_CREAT|O_TRUNC, 0666 ) ) < 0  ){
        perror("error al crear el archivo de la imagen a recibir\n");
        return 0;
    }

    char buffer_recepcion2[FILE_BUFFER_SIZE];
    long byte_leido=0;
    gettimeofday(&inicio, NULL);
    uint32_t bytes_recibidos;
    if (( byte_leido= recv(sockfd_arg2, &bytes_recibidos, 4, 0) ) != 0 ){
        if ( byte_leido < 0  ){
            perror("error al leer del sock_arg2");
        }
    }

    bytes_recibidos=ntohl(bytes_recibidos);
    printf("N° de bytes a recibir: %d\n", bytes_recibidos);
    while (bytes_recibidos) {
        memset(buffer_recepcion2, 0, FILE_BUFFER_SIZE);
        if( (byte_leido=recv(sockfd_arg2, buffer_recepcion2, FILE_BUFFER_SIZE, 0)) != 0 ){
            if(byte_leido<=0){
                perror("error el la lectura del socket_arg2");
                continue;
            }
        }
        //ahora escribo el archivo con lo que tengo en el buffer_recepcion

        if( ( write(imagen_fd, buffer_recepcion2, (size_t)byte_leido ) ) < 0){
            perror("error al escribir el archivo de imagen");
            exit(EXIT_FAILURE);
        }
        bytes_recibidos -=byte_leido;
    }
    gettimeofday(&fin, NULL);
    close(imagen_fd);
    printf("DEBUG: imagen escaneada recibida en %lf segundos", (float)(((fin.tv_sec - inicio.tv_sec)*1000000 +fin.tv_usec) - inicio.tv_usec)/1000000);
    return 1; //esto es 1 dado que se comleto la recepcion


}
*/



/**
 *@brief Funcion simple que imprime en consola el path absoluto en el cual se encuentra el ejecutable del programa que la invoca.
 */
void get_dir() {
    char cwd[BUFFER_SIZE];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }
}