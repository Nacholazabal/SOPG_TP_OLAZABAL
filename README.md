# Trabajo Práctico SOPG - Servidor Clave-Valor

## Descripción

Esta es la entrega del TP para SOPG. Se trata de un servidor TCP que implementa un sistema de almacenamiento clave-valor en formato ASCII.

El servidor:
- Escucha en el puerto 5000
- Acepta conexiones TCP de clientes
- Procesa comandos de texto (SET, GET, DEL)
- Almacena la información en archivos del sistema
- Responde con mensajes de texto según el protocolo especificado

## Comandos soportados

- `SET <clave> <valor>`: Crea un archivo con el nombre de la clave y el contenido especificado
- `GET <clave>`: Lee el contenido del archivo correspondiente a la clave
- `DEL <clave>`: Elimina el archivo correspondiente a la clave

## Ejemplo de funcionamiento

```
nacho@Atlas:~/sopg/tp$ nc localhost 5000
SET auto dezire
OK

nacho@Atlas:~/sopg/tp$ nc localhost 5000
GET auto
OK
dezire

nacho@Atlas:~/sopg/tp$ nc localhost 5000
DEL auto
OK

nacho@Atlas:~/sopg/tp$ nc localhost 5000
GET auto
NOTFOUND

nacho@Atlas:~/sopg/tp$
```

## Compilación y ejecución

```bash
gcc -o server server.c
./server
```

El servidor estará listo para recibir conexiones en el puerto 5000.
