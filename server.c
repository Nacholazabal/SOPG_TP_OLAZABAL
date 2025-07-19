#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define PORT 5000           // Puerto donde escucha el servidor
#define BUFSIZE 512         // Tamaño máximo del buffer de entrada

// Función auxiliar para enviar todo el buffer al cliente
// send() puede enviar menos bytes de los pedidos, por eso repetimos hasta enviar todo
ssize_t sendall(int sockfd, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = buf;
    while (total < len) {
        ssize_t n = send(sockfd, p + total, len - total, 0);
        if (n <= 0) return n; // Error o desconexión
        total += n;
    }
    return total;
}

int main(void) {
    // Ignorar SIGPIPE para evitar que el server muera si el cliente se desconecta de golpe
    signal(SIGPIPE, SIG_IGN);

    // Crear el socket TCP (IPv4, orientado a conexión)
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); exit(EXIT_FAILURE); }

    // Configurar la dirección del servidor
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;            // IPv4
    serveraddr.sin_addr.s_addr = INADDR_ANY;    // Escuchar en todas las interfaces locales
    serveraddr.sin_port = htons(PORT);          // Puerto en orden de red

    // Asociar el socket al puerto y dirección
    if (bind(s, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    // Poner el socket en modo escucha (esperar conexiones)
    if (listen(s, 1) < 0) { perror("listen"); exit(EXIT_FAILURE); }

    printf("Servidor escuchando en puerto %d...\n", PORT);

    // Bucle principal: aceptar y atender clientes uno por uno
    while (1) {
        struct sockaddr_in clientaddr;
        socklen_t addrlen = sizeof(clientaddr);
        // Esperar una conexión entrante
        int clientfd = accept(s, (struct sockaddr *)&clientaddr, &addrlen);
        if (clientfd < 0) { perror("accept"); continue; }

        // Leer el comando del cliente, byte a byte hasta '\n' o buffer lleno
        char buffer[BUFSIZE+1];
        ssize_t n = 0, total = 0;
        while (total < BUFSIZE) {
            n = recv(clientfd, buffer + total, 1, 0);
            if (n <= 0) break; // Error o desconexión
            if (buffer[total] == '\n') { total++; break; } // Fin de línea
            total++;
        }
        if (n <= 0) { close(clientfd); continue; } // Si hubo error, cerrar y esperar otro cliente
        buffer[total] = '\0'; // Terminar el string

        // Parsear el comando recibido: cmd, key y value (si corresponde)
        char cmd[8], key[256], value[256];
        int parsed = sscanf(buffer, "%7s %255s %255[^\n]", cmd, key, value);
        if (parsed < 2) {
            // Comando inválido
            sendall(clientfd, "ERROR\n", 6);
            close(clientfd);
            continue;
        }

        // Procesar comando SET
        if (strcmp(cmd, "SET") == 0 && parsed == 3) {
            // Abrir (o crear) archivo con nombre 'key' para escritura
            FILE *f = fopen(key, "w");
            if (!f) {
                sendall(clientfd, "ERROR\n", 6);
                close(clientfd);
                continue;
            }
            // Escribir el valor en el archivo
            fprintf(f, "%s", value);
            fclose(f);
            sendall(clientfd, "OK\n", 3); // Responder OK
        // Procesar comando GET
        } else if (strcmp(cmd, "GET") == 0 && parsed == 2) {
            // Intentar abrir el archivo para lectura
            FILE *f = fopen(key, "r");
            if (!f) {
                sendall(clientfd, "NOTFOUND\n", 9); // No existe
            } else {
                char val[256];
                size_t r = fread(val, 1, sizeof(val)-1, f); // Leer contenido
                val[r] = '\0';
                fclose(f);
                sendall(clientfd, "OK\n", 3); // Responder OK
                sendall(clientfd, val, strlen(val)); // Enviar valor
                sendall(clientfd, "\n", 1); // Nueva línea
            }
        // Procesar comando DEL
        } else if (strcmp(cmd, "DEL") == 0 && parsed == 2) {
            remove(key); // Borrar archivo (no importa si existe o no)
            sendall(clientfd, "OK\n", 3); // Responder OK
        // Comando desconocido
        } else {
            sendall(clientfd, "ERROR\n", 6);
        }
        // Cerrar la conexión con el cliente
        close(clientfd);
    }
    // Cerrar el socket principal (nunca se llega aquí)
    close(s);
    return 0;
}