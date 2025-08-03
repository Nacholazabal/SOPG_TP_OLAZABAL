/**
 * @file servidor.c
 * @brief Servidor TCP básico que interpreta comandos SET, GET y DEL.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define PORT 5000       // Puerto donde escuchará el servidor
#define BUFSIZE 512     // Tamaño máximo del buffer de entrada

/**
 * @brief Envía un mensaje completo al cliente usando send().
 *
 * @param sockfd Socket del cliente.
 * @param msg Cadena a enviar (debe terminar en '\0').
 * @return Número de bytes enviados, o valor negativo en caso de error.
 */
ssize_t sendall(int sockfd, const char *msg) {
    size_t len = strlen(msg);
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(sockfd, msg + total, len - total, 0);
        if (n <= 0) return n; // Error o desconexión
        total += n;
    }
    return total;
}

/**
 * @brief Crea y configura el socket del servidor (IPv4, TCP).
 *
 * @return Descriptor del socket configurado y en escucha.
 */
int setup_server_socket(void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); exit(EXIT_FAILURE); }

    // Estructura con la dirección del servidor
    struct sockaddr_in serveraddr = {0};
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(PORT);

    // Asociar el socket al puerto definido
    if (bind(s, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    // Poner el socket en modo escucha
    if (listen(s, 1) < 0) { perror("listen"); exit(EXIT_FAILURE); }

    return s;
}

/**
 * @brief Lee el comando enviado por el cliente hasta '\\n' o fin de buffer.
 *
 * @param clientfd Socket del cliente.
 * @param buffer Buffer donde guardar el comando recibido.
 * @return 0 en éxito, -1 si hubo error o desconexión.
 */
int read_command(int clientfd, char *buffer) {
    ssize_t total = 0, n = 0;
    while (total < BUFSIZE) {
        n = recv(clientfd, buffer + total, 1, 0);
        if (n <= 0) return -1;      // Error o cliente desconectado
        if (buffer[total] == '\n') { // Fin de línea
            total++;
            break;
        }
        total++;
    }
    buffer[total] = '\0'; // Terminar la cadena con '\0'
    return 0;
}

/**
 * @brief Procesa el comando recibido del cliente (SET, GET o DEL).
 *
 * @param clientfd Socket del cliente.
 * @param cmd Comando recibido.
 * @param key Clave asociada al comando.
 * @param value Valor (solo para SET).
 */
void process_command(int clientfd, const char *cmd, const char *key, const char *value) {
    // Comando SET: guardar el valor en un archivo con nombre 'key'
    if (strcmp(cmd, "SET") == 0 && value != NULL) {
        FILE *f = fopen(key, "w");
        if (!f) {
            sendall(clientfd, "ERROR\n");
            return;
        }
        fprintf(f, "%s", value);
        fclose(f);
        sendall(clientfd, "OK\n");

    // Comando GET: leer contenido del archivo 'key'
    } else if (strcmp(cmd, "GET") == 0) {
        FILE *f = fopen(key, "r");
        if (!f) {
            sendall(clientfd, "NOTFOUND\n");  // Archivo no encontrado
        } else {
            char val[256];
            size_t r = fread(val, 1, sizeof(val) - 1, f);
            val[r] = '\0'; // Terminar la cadena
            fclose(f);
            sendall(clientfd, "OK\n");
            sendall(clientfd, val);
            sendall(clientfd, "\n");
        }

    // Comando DEL: eliminar el archivo con nombre 'key'
    } else if (strcmp(cmd, "DEL") == 0) {
        remove(key);  // Ignora si el archivo no existe
        sendall(clientfd, "OK\n");

    // Comando inválido
    } else {
        sendall(clientfd, "ERROR\n");
    }
}

/**
 * @brief Atiende la conexión con un cliente: lee el comando y lo procesa.
 *
 * @param clientfd Socket del cliente.
 */
void handle_client(int clientfd) {
    char buffer[BUFSIZE + 1];
    if (read_command(clientfd, buffer) < 0) {
        close(clientfd);
        return;
    }

    // Variables para parsear el comando: cmd, key y (opcional) value
    char cmd[8] = {0}, key[256] = {0}, value[256] = {0};
    int parsed = sscanf(buffer, "%7s %255s %255[^\n]", cmd, key, value);

    if (parsed < 2) {
        sendall(clientfd, "ERROR\n");
    } else {
        process_command(clientfd, cmd, key, (parsed == 3 ? value : NULL));
    }

    // Cerrar conexión con el cliente
    close(clientfd);
}

int main(void) {
    // Evitar que SIGPIPE mate al servidor si el cliente se desconecta abruptamente
    signal(SIGPIPE, SIG_IGN);

    // Configurar socket del servidor
    int server_socket = setup_server_socket();
    printf("Servidor escuchando en puerto %d...\n", PORT);

    // Bucle principal: aceptar clientes y atenderlos
    while (1) {
        struct sockaddr_in clientaddr;
        socklen_t addrlen = sizeof(clientaddr);

        // Esperar conexión de un cliente
        int clientfd = accept(server_socket, (struct sockaddr *)&clientaddr, &addrlen);
        if (clientfd < 0) {
            perror("accept");
            continue;
        }

        // Manejar la conexión del cliente
        handle_client(clientfd);
    }

    // (No se llega nunca aquí)
    close(server_socket);
    return 0;
}