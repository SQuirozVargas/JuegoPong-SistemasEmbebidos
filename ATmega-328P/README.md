# ATmega-328P
# Adjunto el codigo del juego

// Deber 2: VideoJuego PONG

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

#define TAM_BIENVENIDA 96
#define TAM_GANADOR 32

// Matriz de bienvenida
unsigned char BIENVENIDA[]{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Espacio
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
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Espacio
};

// Matrices de dificultad
unsigned char MODO_FACIL[]   = {0x00, 0x7E, 0x7E, 0x16, 0x16, 0x16, 0x06, 0x00}; // F
unsigned char MODO_NORMAL[]  = {0x00, 0x7E, 0x7E, 0x0C, 0x18, 0x7E, 0x7E, 0x00}; // N
unsigned char MODO_DIFICIL[] = {0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x3C, 0x00}; // D

// Matrices de jugador ganador
unsigned char GANADOR_J1[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Espacio
  0x00, 0x66, 0x66, 0x7E, 0x7E, 0x06, 0x06, 0x00, // J
  0x00, 0x40, 0x44, 0x7E, 0x7E, 0x40, 0x40, 0x00, // 1
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Espacio
};

unsigned char GANADOR_J2[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Espacio
  0x00, 0x66, 0x66, 0x7E, 0x7E, 0x06, 0x06, 0x00, // J
  0x00, 0x66, 0x76, 0x7E, 0x5E, 0x4C, 0x00, 0x00, // 2
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Espacio
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
                    _delay_ms(0.08);
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
            _delay_ms(0.08);
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
    // Velocidad segun nivel
    int velocidad;
    if(nivel == 0)      velocidad = 150;
    else if(nivel == 1) velocidad = 100;
    else                velocidad = 60;

    // Posicion inicial de las paletas
    int columna_paleta_jugador1 = 3;
    int columna_paleta_jugador2 = 3;

    // Posicion inicial de la bola
    int fila_bola    = 3;
    int columna_bola = 3;

    // Direccion de la bola
    int direccion_fila    =  1;
    int direccion_columna =  1;

    // Puntaje
    int puntos_jugador1 = 0;
    int puntos_jugador2 = 0;

    // Temporizador para mover la bola
    int contador_velocidad = 0;

    // Temporizador para mover las paletas
    int contador_paleta = 0;

    while(1){

        // ---- DIBUJAR MATRIZ ----
        unsigned char frame[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        // Dibujar paleta jugador 2 en fila 0
        frame[0] |= (1 << columna_paleta_jugador2);
        frame[0] |= (1 << (columna_paleta_jugador2 + 1));

        // Dibujar paleta jugador 1 en fila 7
        frame[7] |= (1 << columna_paleta_jugador1);
        frame[7] |= (1 << (columna_paleta_jugador1 + 1));

        // Dibujar bola
        frame[fila_bola] |= (1 << columna_bola);

        // Multiplexado
        for(int j = 0; j < 8; j++){
            PORTD = posiciones[j];
            PORTB = ~frame[j];
            _delay_ms(0.08);
        }

        // ---- LEER BOTONES DE PALETAS (led a led) ----
        contador_paleta++;
        if(contador_paleta >= 20){
            contador_paleta = 0;

            // Jugador 1 izquierda (PC0)
            if(!(PINC & (1 << PC0))){
                if(columna_paleta_jugador1 > 0) columna_paleta_jugador1--;
            }
            // Jugador 1 derecha (PC1)
            if(!(PINC & (1 << PC1))){
                if(columna_paleta_jugador1 < 6) columna_paleta_jugador1++;
            }
            // Jugador 2 izquierda (PC2)
            if(!(PINC & (1 << PC2))){
                if(columna_paleta_jugador2 > 0) columna_paleta_jugador2--;
            }
            // Jugador 2 derecha (PC3)
            if(!(PINC & (1 << PC3))){
                if(columna_paleta_jugador2 < 6) columna_paleta_jugador2++;
            }
        }

        // ---- MOVER BOLA SEGUN VELOCIDAD ----
        contador_velocidad++;
        if(contador_velocidad >= velocidad){
            contador_velocidad = 0;

            // Calcular siguiente posicion
            int siguiente_fila    = fila_bola    + direccion_fila;
            int siguiente_columna = columna_bola + direccion_columna;

            // ---- COLISION CON PALETA JUGADOR 1 (bola en fila 7) ----
            if(fila_bola == 7){
                if(columna_bola == columna_paleta_jugador1 ||
                   columna_bola == columna_paleta_jugador1 + 1){
                    direccion_fila = -direccion_fila;
                    siguiente_fila = fila_bola + direccion_fila;
                    // Chequeo adicional de pared lateral en esquina
                    if(siguiente_columna <= 0 || siguiente_columna >= 7){
                        direccion_columna = -direccion_columna;
                        siguiente_columna = columna_bola + direccion_columna;
                    }
                } else {
                    // Punto para jugador 2
                    puntos_jugador2++;
                    fila_bola         = 3;
                    columna_bola      = 3;
                    direccion_fila    = 1;
                    direccion_columna = 1;
                    if(puntos_jugador2 >= 3) return 2;
                    continue;
                }
            }
            // ---- COLISION CON PALETA JUGADOR 2 (bola en fila 0) ----
            else if(fila_bola == 0){
                if(columna_bola == columna_paleta_jugador2 ||
                   columna_bola == columna_paleta_jugador2 + 1){
                    direccion_fila = -direccion_fila;
                    siguiente_fila = fila_bola + direccion_fila;
                    // Chequeo adicional de pared lateral en esquina
                    if(siguiente_columna <= 0 || siguiente_columna >= 7){
                        direccion_columna = -direccion_columna;
                        siguiente_columna = columna_bola + direccion_columna;
                    }
                } else {
                    // Punto para jugador 1
                    puntos_jugador1++;
                    fila_bola         = 3;
                    columna_bola      = 3;
                    direccion_fila    = -1;
                    direccion_columna = 1;
                    if(puntos_jugador1 >= 3) return 1;
                    continue;
                }
            }
            // ---- COLISION CON PAREDES LATERALES ----
            else if(siguiente_columna <= 0 || siguiente_columna >= 7){
                direccion_columna = -direccion_columna;
                siguiente_columna = columna_bola + direccion_columna;
            }

            // Actualizar posicion de la bola
            fila_bola    = siguiente_fila;
            columna_bola = siguiente_columna;
        }
    }
}

// ---- ESTADO GANADOR ----
void estado_ganador(int ganador){
    for(int i = 0; i <= TAM_GANADOR - 8; i++){
        for(int k = 0; k < 50; k++){
            for(int j = 0; j < 8; j++){
                PORTD = posiciones[j];
                if(ganador == 1) PORTB = ~GANADOR_J1[i + j];
                else             PORTB = ~GANADOR_J2[i + j];
                _delay_ms(0.08);
            }
        }
    }
    PORTD = 0x00;
    PORTB = 0xFF;
}

int main(){
    DDRD  = 0xFF; // PORTD salida — filas
    DDRB  = 0xFF; // PORTB salida — columnas
    DDRC  = 0x00; // PORTC entrada — botones
    PORTC = 0xFF; // Pull-ups internos activados

    while(1){
        estado_bienvenido();
        _delay_ms(200);
        int nivel   = estado_seleccion_nivel();
        int ganador = estado_juego(nivel);
        estado_ganador(ganador);
    }
}
