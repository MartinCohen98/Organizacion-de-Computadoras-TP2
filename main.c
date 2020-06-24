#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#define MEMSIZE 65536 //2 a la 16
#define CACHESIZE 32 //32 bloques en el cache
#define BLOCKSIZE 128
#define SETSNUM 4
#define SETSIZE CACHESIZE / SETSNUM
#define BUFLEN 25

void init();
unsigned int get_offset(unsigned int address);
unsigned int find_set(unsigned int address);
unsigned int select_oldest(unsigned int setnum);
int compare_tag (unsigned int tag, unsigned int set);
void read_tocache(unsigned int blocknum, unsigned int way, unsigned int set);
void write_tocache(unsigned int address, unsigned char value);
unsigned char read_byte(unsigned int address);
void write_byte(unsigned int address, unsigned char value);
float get_miss_rate();

unsigned int get_tag(unsigned int address);
void sumar_tiempo();
void interpretar_archivo(FILE* archivo);


struct BloqueCache {
	bool valido;
	unsigned char bloque[BLOCKSIZE];
	unsigned int tiempoDesdeLectura;
	unsigned int tag;
};

struct Conjunto {
	struct BloqueCache via[SETSIZE];
};

unsigned char memoria[MEMSIZE];
struct Conjunto cache[SETSNUM];
unsigned int misses;
unsigned int cuenta;

int main(int argc, char** argv) {
	bool argsOk = false;
	int retorno = 0;

    if(argc == 2) {
    	argsOk = true;
    }

    if(!argsOk) {
    	fprintf(stderr, "Error en entrada de parametros, ingresar un archivo.\n");
    	retorno = -1;
    }

	FILE* archivo;
	archivo = fopen(argv[1], "r");
	if (archivo == NULL) {
		fprintf(stderr, "Error al arbir archivo, checkear nombre de archivo\n");
		return -2;
	}

	interpretar_archivo(archivo);

	fclose(archivo);
    printf("\n");
    return retorno;
}


void interpretar_archivo(FILE* archivo) {
	init();

	int resultadoLectura;
	char buffer[BUFLEN];
	resultadoLectura = fscanf(archivo, "%s", buffer);
	while (resultadoLectura != EOF) {
		if (strcmp(buffer, "FLUSH") == 0)
			init();
		if (strcmp(buffer, "MR") == 0)
			printf("%f", get_miss_rate());
		if (strcmp(buffer, "R") == 0) {
			unsigned int address;
			resultadoLectura = fscanf(archivo, "%u", &address);
			if (address < MEMSIZE)
				printf("%u\n", read_byte(address));
		}
		if (strcmp(buffer, "W") == 0) {
			unsigned int address;
			unsigned char value;
			resultadoLectura = fscanf(archivo, "%u, %hhu", &address, &value);
			if (address < MEMSIZE)
				write_byte(address, value);
		}
		resultadoLectura = fscanf(archivo, "%s", buffer);
	}
}


void init() {
	memset(memoria, 0, sizeof(memoria));
	for (int i = 0; i < SETSNUM; i++) {
		for (int j = 0; j < (SETSIZE); j++) {
			cache[i].via[j].valido = false;
		}
	}
	misses = 0;
}


unsigned int get_offset(unsigned int address) {
	unsigned int offset = address % BLOCKSIZE;
	return offset;
}


unsigned int find_set(unsigned int address) {
	unsigned int bloque = address / BLOCKSIZE;
	unsigned int set = bloque % SETSNUM;
	return set;
}


unsigned int select_oldest(unsigned int setnum) {
	unsigned int oldest = 0;
	for (unsigned int i = 0; i < SETSIZE; i++) {
		if (!cache[setnum].via[i].valido) {
			oldest = i;
		} else {
			if (cache[setnum].via[oldest].valido &&
				(cache[setnum].via[i].tiempoDesdeLectura >
				cache[setnum].via[oldest].tiempoDesdeLectura)) {
				oldest = i;
			}
		}
	}
	return oldest;
}


int compare_tag(unsigned int tag, unsigned int set) {
	int match = -1;
	for (unsigned int i = 0; i < SETSIZE; i++) {
		if ((cache[set].via[i].tag == tag) && (cache[set].via[i].valido))
			match = i;
	}
	return match;
}


void read_tocache(unsigned int blocknum, unsigned int way, unsigned int set) {
	unsigned int direccion_bloque = blocknum * BLOCKSIZE;
	memcpy(cache[set].via[way].bloque , &memoria[direccion_bloque], BLOCKSIZE);
	cache[set].via[way].tag = blocknum / SETSNUM;
	cache[set].via[way].tiempoDesdeLectura = 0;
	cache[set].via[way].valido = true;
}


void write_tocache(unsigned int address, unsigned char value) {
	unsigned int set = find_set(address);
	unsigned int tag = get_tag(address);
	for (int i = 0; i < SETSIZE; i++) {
		if (cache[set].via[i].valido && (cache[set].via[i].tag == tag)) {
			unsigned int offset = get_offset(address);
			cache[set].via[i].bloque[offset] = value;
		}
	}
}


unsigned char read_byte(unsigned int address) {
	unsigned int offset = get_offset(address);
	unsigned int set = find_set(address);
	unsigned int tag = get_tag(address);
	int way = compare_tag(tag, set);
	if (way < 0) {
		way = select_oldest(set);
		read_tocache(address / BLOCKSIZE, way, set);
		misses++;
	}
	sumar_tiempo();
	return cache[set].via[way].bloque[offset];
}


void write_byte(unsigned int address, unsigned char value) {
	write_tocache(address, value);
	memoria[address] = value;
}


float get_miss_rate() {
	return ((float) misses / (float) cuenta);
}


void sumar_tiempo() {
	for (int i = 0; i < SETSNUM; i++) {
		for (int j = 0; j < SETSIZE; j++) {
			cache[i].via[j].tiempoDesdeLectura++;
		}
	}
	cuenta++;
}


unsigned int get_tag(unsigned int address) {
	return (address / BLOCKSIZE) / SETSNUM;
}


