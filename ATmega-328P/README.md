// Deber 2: VideoJuego PONG 

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

#define TAM_BIENVENIDA 96
#define TAM_GANADOR    80

// Matriz de bienvenida
unsigned char BIENVENIDA[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x7E, 0x7E, 0x5A, 0x5A, 0x7E, 0x3C, 0x00, // B
  0x00, 0x66, 0x66, 0x7E, 0x7E, 0x66, 0x66, 0x00, // I
  0x00, 0x7E, 0x7E, 0x5A, 0x5A, 0x5A, 0x5A, 0x00, // E
  0x00, 0x7E, 0x7E, 0x0C, 0x18, 0x7E, 0x7E, 0x00, // N
  0x00, 0x1E, 0x3E, 0x60, 0x60, 0x3E, 0x1E, 0x00, // V
  0x00, 0x7E, 0x7E, 0x5A, 0x5A, 0x5A, 0x5A, 0x00, // E
  0x00, 0x7E, 0x7E, 0x0C, 0x18, 0x7E, 0x7E, 0x00, // N
  0x00, 0x66, 0x66, 0x7E, 0x7E, 0x66, 0x66, 0x00, // I
  0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x3C, 0x00, // D
  0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x7E, 0x00, // O
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Matriz de GANADOR
unsigned char GANADOR[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Espacio
    0x00, 0x7E, 0x7E, 0x46, 0x56, 0x76, 0x76, 0x00, // G
    0x00, 0x7E, 0x7E, 0x1A, 0x1A, 0x7E, 0x7E, 0x00, // A
    0x00, 0x7E, 0x7E, 0x0C, 0x18, 0x7E, 0x7E, 0x00, // N
    0x00, 0x7E, 0x7E, 0x1A, 0x1A, 0x7E, 0x7E, 0x00, // A
    0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x3C, 0x00, // D
    0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x7E, 0x00, // O
    0x00, 0x7E, 0x7E, 0x12, 0x12, 0x3E, 0x6C, 0x00, // R
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Espacio
};

// Matrices de dificultad
unsigned char MODO_FACIL[]   = {0x00, 0x7E, 0x7E, 0x16, 0x16, 0x16, 0x06, 0x00}; // F
unsigned char MODO_NORMAL[]  = {0x00, 0x7E, 0x7E, 0x0C, 0x18, 0x7E, 0x7E, 0x00}; // N
unsigned char MODO_DIFICIL[] = {0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x3C, 0x00}; // D

// Digitos 0-9
unsigned char DIGITO_0[] = {0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x7E, 0x00};
unsigned char DIGITO_1[] = {0x00, 0x08, 0x04, 0x7E, 0x7e, 0x00, 0x00, 0x00};
unsigned char DIGITO_2[] = {0x00, 0x66, 0x76, 0x7E, 0x5E, 0x4C, 0x40, 0x00};
unsigned char DIGITO_3[] = {0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5E, 0x7E, 0x00};
unsigned char DIGITO_4[] = {0x00, 0x0E, 0x0E, 0x08, 0x08, 0x7E, 0x7E, 0x00};
unsigned char DIGITO_5[] = {0x00, 0x5E, 0x5E, 0x5E, 0x76, 0x76, 0x76, 0x00};
unsigned char DIGITO_6[] = {0x00, 0x7E, 0x7E, 0x4A, 0x4A, 0x7A, 0x7A, 0x00};
unsigned char DIGITO_7[] = {0x00, 0x46, 0x66, 0x36, 0x1E, 0x0E, 0x06, 0x00};
unsigned char DIGITO_8[] = {0x00, 0x7E, 0x7E, 0x56, 0x56, 0x7E, 0x7E, 0x00};
unsigned char DIGITO_9[] = {0x00, 0x1E, 0x1E, 0x16, 0x76, 0x76, 0x7E, 0x00};

// Palabra PUNTOS para scroll de puntuacion
unsigned char PUNTOS[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x7E, 0x7E, 0x12, 0x12, 0x1E, 0x0C, 0x00, // P
    0x00, 0x7E, 0x7E, 0x60, 0x60, 0x7E, 0x7E, 0x00, // U
    0x00, 0x7E, 0x7E, 0x0C, 0x18, 0x7E, 0x7E, 0x00, // N
    0x00, 0x06, 0x06, 0x7E, 0x7E, 0x06, 0x06, 0x00, // T
    0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x7E, 0x00, // O
    0x00, 0x5E, 0x5E, 0x56, 0x76, 0x76, 0x76, 0x00, // S
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Multiplexacion
unsigned char posiciones[8] = {1, 2, 4, 8, 16, 32, 64, 128};

// ---- ESTADO BIENVENIDO ----
void estado_bienvenido(){
    while(1){
        for(int i = 0; i <= TAM_BIENVENIDA - 8; i++){
            for(int k = 0; k < 50; k++){
                for(int j = 0; j < 8; j++){
                    PORTD = posiciones[j];
                    PORTB = ~BIENVENIDA[i + j];
                    _delay_ms(0.05);
                }
                if(!(PINC & (1 << PC4))){
                    PORTD = 0x00;
                    PORTB = 0xFF;
                    return;
                }
            }
        }
    }
}

// ---- ESTADO SELECCION NIVEL ----
int estado_seleccion_nivel(){
    int nivel = 0;

    while(1){
        for(int j = 0; j < 8; j++){
            PORTD = posiciones[j];
            if(nivel == 0)      PORTB = ~MODO_FACIL[j];
            else if(nivel == 1) PORTB = ~MODO_NORMAL[j];
            else                PORTB = ~MODO_DIFICIL[j];
            _delay_ms(0.05);
        }
        if(!(PINC & (1 << PC4))){
            nivel = (nivel + 1) % 3;
            _delay_ms(200);
        }
        if(!(PINC & (1 << PC5))){
            PORTD = 0x00;
            PORTB = 0xFF;
            return nivel;
        }
    }
}

// ---- ESTADO JUEGO ----
int estado_juego(int nivel){
    int velocidad;
    if(nivel == 0)      velocidad = 150;
    else if(nivel == 1) velocidad = 100;
    else                velocidad = 60;

    int columna_paleta    = 3;
    int fila_bola         = 4;
    int columna_bola      = 3;
    int direccion_fila    = 1;
    int direccion_columna = 1;
    int puntos            = 0;
    int contador_velocidad = 0;
    int contador_paleta    = 0;

    while(1){
        unsigned char frame[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        frame[7] |= (1 << columna_paleta);
        frame[7] |= (1 << (columna_paleta + 1));
        frame[fila_bola] |= (1 << columna_bola);

        for(int j = 0; j < 8; j++){
            PORTD = posiciones[j];
            PORTB = ~frame[j];
            _delay_ms(0.05);
        }

        contador_paleta++;
        if(contador_paleta >= 20){
            contador_paleta = 0;
            if(!(PINC & (1 << PC2))){
                if(columna_paleta > 0) columna_paleta--;
            }
            if(!(PINC & (1 << PC3))){
                if(columna_paleta < 6) columna_paleta++;
            }
        }

        contador_velocidad++;
        if(contador_velocidad >= velocidad){
            contador_velocidad = 0;

            int siguiente_fila    = fila_bola    + direccion_fila;
            int siguiente_columna = columna_bola + direccion_columna;

            if(fila_bola == 7){
                if(columna_bola == columna_paleta ||
                   columna_bola == columna_paleta + 1){
                    puntos++;
                    if(puntos >= 10) return -1; // GANADOR
                    direccion_fila = -direccion_fila;
                    siguiente_fila = fila_bola + direccion_fila;
                    if(siguiente_columna <= 0 || siguiente_columna >= 7){
                        direccion_columna = -direccion_columna;
                        siguiente_columna = columna_bola + direccion_columna;
                    }
                } else {
                    return puntos;
                }
            }
            else if(siguiente_fila <= 0){
                direccion_fila = -direccion_fila;
                siguiente_fila = fila_bola + direccion_fila;
                if(siguiente_columna <= 0 || siguiente_columna >= 7){
                    direccion_columna = -direccion_columna;
                    siguiente_columna = columna_bola + direccion_columna;
                }
            }
            else if(siguiente_columna <= 0 || siguiente_columna >= 7){
                direccion_columna = -direccion_columna;
                siguiente_columna = columna_bola + direccion_columna;
            }

            fila_bola    = siguiente_fila;
            columna_bola = siguiente_columna;
        }
    }
}

// ---- ESTADO GANADOR ----
void estado_ganador(){
    for(int rep = 0; rep < 3; rep++){
        for(int i = 0; i <= TAM_GANADOR - 8; i++){
            for(int k = 0; k < 50; k++){
                for(int j = 0; j < 8; j++){
                    PORTD = posiciones[j];
                    PORTB = ~GANADOR[i + j];
                    _delay_ms(0.05);
                }
            }
        }
    }
    PORTD = 0x00;
    PORTB = 0xFF;
}

// ---- ESTADO PUNTUACION ----
void estado_puntuacion(int puntos){
    int decenas  = puntos / 10;
    int unidades = puntos % 10;

    unsigned char* digitos[10] = {
        DIGITO_0, DIGITO_1, DIGITO_2, DIGITO_3, DIGITO_4,
        DIGITO_5, DIGITO_6, DIGITO_7, DIGITO_8, DIGITO_9
    };

    unsigned char buffer[80];
    int tam = 0;

    for(int i = 0; i < 8; i++)  buffer[tam++] = 0x00;

    if(puntos >= 10){
        for(int i = 0; i < 8; i++) buffer[tam++] = digitos[decenas][i];
    }
    for(int i = 0; i < 8; i++)  buffer[tam++] = digitos[unidades][i];
    for(int i = 0; i < 48; i++) buffer[tam++] = PUNTOS[i];
    for(int i = 0; i < 8; i++)  buffer[tam++] = 0x00;

    for(int i = 0; i <= tam - 8; i++){
        for(int k = 0; k < 50; k++){
            for(int j = 0; j < 8; j++){
                PORTD = posiciones[j];
                PORTB = ~buffer[i + j];
                _delay_ms(0.05);
            }
        }
    }

    PORTD = 0x00;
    PORTB = 0xFF;
}

int main(){
    DDRD  = 0xFF;
    DDRB  = 0xFF;
    DDRC  = 0x00;
    PORTC = 0xFF;

    while(1){
        estado_bienvenido();
        _delay_ms(10);
        int nivel  = estado_seleccion_nivel();
        int puntos = estado_juego(nivel);
        if(puntos == -1) estado_ganador();
        else             estado_puntuacion(puntos);
    }
}
