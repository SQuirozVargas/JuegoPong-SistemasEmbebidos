// ============================================================
// DEBER 2: VIDEOJUEGO PONG
// MICROCONTROLADOR: ATmega328P
// FRECUENCIA DE TRABAJO: 8 MHz
//
// FUNCIONES DEL MICROCONTROLADOR:
// - Controlar la matriz LED 8x8 mediante multiplexación.
// - Mostrar mensajes desplazables.
// - Leer los pulsadores del usuario.
// - Ejecutar la lógica del videojuego Pong.
// - Enviar comandos de sonido al PIC16F887.
//
// CONEXIONES PRINCIPALES:
//
// MATRIZ LED 8x8
// PORTD -> Filas F0-F7
// PORTB -> Columnas C0-C7
//
// PULSADORES
// ADC6 -> SUBIR
// PC3  -> BAJAR
// PC4  -> SELECCIONAR NIVEL
// PC5  -> INICIAR PARTIDA
//
// COMUNICACIÓN PARALELA HACIA EL PIC16F887
// PC0 -> RA0
// PC1 -> RA1
// PC2 -> RA2
// ============================================================

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>

// ============================================================
// TAMAÑO DE LOS MENSAJES DESPLAZABLES
// ============================================================

#define TAM_BIENVENIDA 96
#define TAM_GANADOR    72

// ============================================================
// COMANDOS ENVIADOS AL PIC16F887
//
// Los pines PC0, PC1 y PC2 forman un bus paralelo de 3 bits.
// Cada combinación representa un evento del videojuego.
//
// PC2 PC1 PC0
//  0   0   0  -> Silencio
//  0   0   1  -> Inicio de partida
//  0   1   0  -> Rebote contra pared
//  0   1   1  -> Rebote contra paleta
//  1   0   0  -> Cambio de nivel
//  1   0   1  -> Derrota
//  1   1   0  -> Victoria
//  1   1   1  -> Música de bienvenida
// ============================================================

#define CMD_SILENCIO    0
#define CMD_INICIO      1
#define CMD_PARED       2
#define CMD_PALETA      3
#define CMD_NIVEL       4
#define CMD_DERROTA     5
#define CMD_VICTORIA    6
#define CMD_BIENVENIDA  7

// ============================================================
// MATRIZ DEL MENSAJE "BIENVENIDO"
//
// Cada grupo de 8 valores representa una letra.
// El mensaje se desplaza horizontalmente en la matriz LED.
// ============================================================

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

// ============================================================
// MATRIZ DEL MENSAJE "GANADOR"
//
// Este mensaje se muestra cuando el jugador alcanza 10 puntos.
// ============================================================

unsigned char GANADOR[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x7E, 0x7E, 0x46, 0x56, 0x76, 0x76, 0x00, // G
    0x00, 0x7E, 0x7E, 0x1A, 0x1A, 0x7E, 0x7E, 0x00, // A
    0x00, 0x7E, 0x7E, 0x0C, 0x18, 0x7E, 0x7E, 0x00, // N
    0x00, 0x7E, 0x7E, 0x1A, 0x1A, 0x7E, 0x7E, 0x00, // A
    0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x3C, 0x00, // D
    0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x7E, 0x00, // O
    0x00, 0x7E, 0x7E, 0x12, 0x12, 0x3E, 0x6C, 0x00, // R

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// ============================================================
// ICONOS PARA SELECCIONAR LA DIFICULTAD
//
// F -> Fácil
// N -> Normal
// D -> Difícil
// ============================================================

unsigned char MODO_FACIL[] = {
    0x00, 0x7E, 0x7E, 0x16, 0x16, 0x16, 0x06, 0x00
};

unsigned char MODO_NORMAL[] = {
    0x00, 0x7E, 0x7E, 0x0C, 0x18, 0x7E, 0x7E, 0x00
};

unsigned char MODO_DIFICIL[] = {
    0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x3C, 0x00
};

// ============================================================
// MATRICES DE LOS DÍGITOS DEL 0 AL 9
//
// Se utilizan para mostrar el puntaje obtenido al finalizar.
// ============================================================

unsigned char DIGITO_0[] = {
    0x00, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x7E, 0x00
};

unsigned char DIGITO_1[] = {
    0x00, 0x08, 0x04, 0x7E, 0x7E, 0x00, 0x00, 0x00
};

unsigned char DIGITO_2[] = {
    0x00, 0x66, 0x76, 0x7E, 0x5E, 0x4C, 0x40, 0x00
};

unsigned char DIGITO_3[] = {
    0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5E, 0x7E, 0x00
};

unsigned char DIGITO_4[] = {
    0x00, 0x0E, 0x0E, 0x08, 0x08, 0x7E, 0x7E, 0x00
};

unsigned char DIGITO_5[] = {
    0x00, 0x5E, 0x5E, 0x5E, 0x76, 0x76, 0x76, 0x00
};

unsigned char DIGITO_6[] = {
    0x00, 0x7E, 0x7E, 0x4A, 0x4A, 0x7A, 0x7A, 0x00
};

unsigned char DIGITO_7[] = {
    0x00, 0x46, 0x66, 0x36, 0x1E, 0x0E, 0x06, 0x00
};

unsigned char DIGITO_8[] = {
    0x00, 0x7E, 0x7E, 0x56, 0x56, 0x7E, 0x7E, 0x00
};

unsigned char DIGITO_9[] = {
    0x00, 0x1E, 0x1E, 0x16, 0x76, 0x76, 0x7E, 0x00
};

// ============================================================
// MATRIZ DEL MENSAJE "PUNTOS"
//
// Se muestra después del número obtenido por el jugador.
// ============================================================

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

// ============================================================
// POSICIONES DE LAS FILAS DE LA MATRIZ
//
// Cada valor activa una fila durante la multiplexación.
// ============================================================

unsigned char posiciones[8] = {
    1, 2, 4, 8, 16, 32, 64, 128
};

// ============================================================
// FUNCIÓN: enviar_comando_pic
//
// Envía un evento al PIC16F887 mediante PC0-PC2.
// El PIC interpreta el valor recibido y reproduce un sonido.
//
// El comando se mantiene durante 20 ms para permitir su lectura.
// Después, el bus regresa a 000 para habilitar un nuevo evento.
// ============================================================

void enviar_comando_pic(unsigned char comando) {
    // Conservar el estado de PC3-PC5 y modificar PC0-PC2.
    PORTC =
        (PORTC & 0xF8) |
        (comando & 0x07);

    _delay_ms(20);

    // Regresar a silencio.
    PORTC &= 0xF8;
}

// ============================================================
// FUNCIONES: iniciar_musica_bienvenida y detener_musica_bienvenida
//
// La música de bienvenida funciona de forma continua.
// Por esta razón, el código 111 permanece activo mientras
// el texto "BIENVENIDO" se desplaza en la matriz.
// ============================================================

void iniciar_musica_bienvenida() {
    PORTC =
        (PORTC & 0xF8) |
        CMD_BIENVENIDA;
}

void detener_musica_bienvenida() {
    PORTC &= 0xF8;
}

// ============================================================
// FUNCIÓN: inicializar_adc
//
// Configura el convertidor analógico-digital para leer ADC6.
// El pulsador SUBIR fue conectado a ADC6 para liberar PC2,
// debido a que PC2 se utiliza para comunicarse con el PIC.
// ============================================================

void inicializar_adc() {
    // Usar AVCC como referencia y seleccionar ADC6.
    ADMUX =
        (1 << REFS0) |
        6;

    // Activar ADC con prescaler de 64.
    // Frecuencia del ADC: 8 MHz / 64 = 125 kHz.
    ADCSRA =
        (1 << ADEN) |
        (1 << ADPS2) |
        (1 << ADPS1);
}

// ============================================================
// FUNCIÓN: leer_adc6
//
// Ejecuta una conversión analógica y devuelve el valor leído.
// El resultado puede variar entre 0 y 1023.
// ============================================================

unsigned int leer_adc6() {
    // Seleccionar ADC6 conservando la referencia AVCC.
    ADMUX =
        (ADMUX & 0xF0) |
        6;

    // Iniciar conversión.
    ADCSRA |=
        (1 << ADSC);

    // Esperar hasta finalizar la conversión.
    while (ADCSRA & (1 << ADSC)) {
    }

    return ADC;
}

// ============================================================
// FUNCIÓN: boton_subir_presionado
//
// Determina si el botón conectado a ADC6 fue presionado.
// Al presionar el botón, ADC6 queda conectado a GND.
// ============================================================

unsigned char boton_subir_presionado() {
    if (leer_adc6() < 200) {
        return 1;
    }

    return 0;
}

// ============================================================
// FUNCIÓN: estado_bienvenido
//
// Desplaza el mensaje "BIENVENIDO" en la matriz LED.
// Durante este estado, el PIC reproduce música de bienvenida.
// El botón conectado a PC4 permite avanzar al siguiente estado.
// ============================================================

void estado_bienvenido() {
    // Iniciar la música antes de mostrar el mensaje.
    iniciar_musica_bienvenida();

    while (1) {
        for (
            int i = 0;
            i <= TAM_BIENVENIDA - 8;
            i++
        ) {
            for (int k = 0; k < 50; k++) {
                for (int j = 0; j < 8; j++) {
                    // Activar una fila y mostrar sus columnas.
                    PORTD =
                        posiciones[j];

                    PORTB =
                        ~BIENVENIDA[i + j];

                    _delay_ms(0.05);
                }

                // PC4 permite salir del mensaje de bienvenida.
                if (!(PINC & (1 << PC4))) {
                    PORTD = 0x00;
                    PORTB = 0xFF;

                    // Detener música antes de cambiar de estado.
                    detener_musica_bienvenida();

                    _delay_ms(200);

                    return;
                }
            }
        }
    }
}

// ============================================================
// FUNCIÓN: estado_seleccion_nivel
//
// Permite elegir entre tres dificultades:
// 0 -> Fácil
// 1 -> Normal
// 2 -> Difícil
//
// PC4 cambia el nivel.
// PC5 confirma la selección.
// ============================================================

int estado_seleccion_nivel() {
    int nivel = 0;

    while (1) {
        for (int j = 0; j < 8; j++) {
            PORTD =
                posiciones[j];

            // Mostrar el símbolo correspondiente al nivel actual.
            if (nivel == 0) {
                PORTB =
                    ~MODO_FACIL[j];
            }
            else if (nivel == 1) {
                PORTB =
                    ~MODO_NORMAL[j];
            }
            else {
                PORTB =
                    ~MODO_DIFICIL[j];
            }

            _delay_ms(0.05);
        }

        // Cambiar nivel y reproducir sonido de confirmación.
        if (!(PINC & (1 << PC4))) {
            nivel =
                (nivel + 1) % 3;

            enviar_comando_pic(
                CMD_NIVEL
            );

            _delay_ms(200);
        }

        // Confirmar nivel y continuar con el juego.
        if (!(PINC & (1 << PC5))) {
            PORTD = 0x00;
            PORTB = 0xFF;

            _delay_ms(200);

            return nivel;
        }
    }
}

// ============================================================
// FUNCIÓN: estado_juego
//
// Ejecuta la lógica principal del Pong.
//
// La pelota rebota contra tres paredes.
// La paleta se encuentra en la última fila.
// El jugador gana al alcanzar 10 puntos.
// ============================================================

int estado_juego(int nivel) {
    int velocidad;

    // Ajustar velocidad según la dificultad elegida.
    if (nivel == 0) {
        velocidad = 150;
    }
    else if (nivel == 1) {
        velocidad = 100;
    }
    else {
        velocidad = 60;
    }

    // Posición inicial de la paleta.
    int columna_paleta =
        3;

    // Posición inicial de la pelota.
    int fila_bola =
        4;

    int columna_bola =
        3;

    // Dirección inicial de la pelota.
    int direccion_fila =
        1;

    int direccion_columna =
        1;

    // Puntaje acumulado.
    int puntos =
        0;

    // Contadores utilizados para controlar la velocidad.
    int contador_velocidad =
        0;

    int contador_paleta =
        0;

    // Avisar al PIC que la partida comenzó.
    enviar_comando_pic(
        CMD_INICIO
    );

    while (1) {
        // Crear una imagen temporal del juego.
        unsigned char frame[8] = {
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00
        };

        // Dibujar la paleta con dos LED consecutivos.
        frame[7] |=
            (1 << columna_paleta);

        frame[7] |=
            (1 << (columna_paleta + 1));

        // Dibujar la pelota.
        frame[fila_bola] |=
            (1 << columna_bola);

        // Mostrar el frame mediante multiplexación.
        for (int j = 0; j < 8; j++) {
            PORTD =
                posiciones[j];

            PORTB =
                ~frame[j];

            _delay_ms(0.05);
        }

        // ----------------------------------------------------
        // CONTROLAR EL MOVIMIENTO DE LA PALETA
        // ----------------------------------------------------

        contador_paleta++;

        if (contador_paleta >= 20) {
            contador_paleta = 0;

            // ADC6 reemplaza al pin PC2 original.
            // Permite desplazar la paleta en una dirección.
            if (boton_subir_presionado()) {
                if (columna_paleta > 0) {
                    columna_paleta--;
                }
            }

            // PC3 desplaza la paleta en la dirección contraria.
            if (!(PINC & (1 << PC3))) {
                if (columna_paleta < 6) {
                    columna_paleta++;
                }
            }
        }

        // ----------------------------------------------------
        // ACTUALIZAR LA POSICIÓN DE LA PELOTA
        // ----------------------------------------------------

        contador_velocidad++;

        if (contador_velocidad >= velocidad) {
            contador_velocidad = 0;

            // Calcular la siguiente posición de la pelota.
            int siguiente_fila =
                fila_bola +
                direccion_fila;

            int siguiente_columna =
                columna_bola +
                direccion_columna;

            // ------------------------------------------------
            // VERIFICAR CONTACTO CON LA PALETA
            // ------------------------------------------------

            if (fila_bola == 7) {
                if (
                    columna_bola == columna_paleta ||
                    columna_bola == columna_paleta + 1
                ) {
                    // Aumentar puntaje después de un rebote válido.
                    puntos++;

                    // Finalizar la partida al alcanzar 10 puntos.
                    if (puntos >= 10) {
                        enviar_comando_pic(
                            CMD_VICTORIA
                        );

                        return -1;
                    }

                    // Reproducir sonido de rebote contra paleta.
                    enviar_comando_pic(
                        CMD_PALETA
                    );

                    // Invertir dirección vertical.
                    direccion_fila =
                        -direccion_fila;

                    siguiente_fila =
                        fila_bola +
                        direccion_fila;

                    // Corregir trayectoria si también alcanza un borde.
                    if (
                        siguiente_columna <= 0 ||
                        siguiente_columna >= 7
                    ) {
                        direccion_columna =
                            -direccion_columna;

                        siguiente_columna =
                            columna_bola +
                            direccion_columna;
                    }
                }
                else {
                    // La pelota no coincidió con la paleta.
                    enviar_comando_pic(
                        CMD_DERROTA
                    );

                    return puntos;
                }
            }

            // ------------------------------------------------
            // REBOTE CONTRA LA PARED SUPERIOR
            // ------------------------------------------------

            else if (siguiente_fila <= 0) {
                direccion_fila =
                    -direccion_fila;

                siguiente_fila =
                    fila_bola +
                    direccion_fila;

                enviar_comando_pic(
                    CMD_PARED
                );

                // Corregir trayectoria si alcanza una esquina.
                if (
                    siguiente_columna <= 0 ||
                    siguiente_columna >= 7
                ) {
                    direccion_columna =
                        -direccion_columna;

                    siguiente_columna =
                        columna_bola +
                        direccion_columna;
                }
            }

            // ------------------------------------------------
            // REBOTE CONTRA PAREDES LATERALES
            // ------------------------------------------------

            else if (
                siguiente_columna <= 0 ||
                siguiente_columna >= 7
            ) {
                direccion_columna =
                    -direccion_columna;

                siguiente_columna =
                    columna_bola +
                    direccion_columna;

                enviar_comando_pic(
                    CMD_PARED
                );
            }

            // Guardar la nueva posición de la pelota.
            fila_bola =
                siguiente_fila;

            columna_bola =
                siguiente_columna;
        }
    }
}

// ============================================================
// FUNCIÓN: estado_ganador
//
// Desplaza el mensaje "GANADOR" tres veces.
// Se ejecuta después de alcanzar 10 puntos.
// ============================================================

void estado_ganador() {
    for (int rep = 0; rep < 3; rep++) {
        for (
            int i = 0;
            i <= TAM_GANADOR - 8;
            i++
        ) {
            for (int k = 0; k < 50; k++) {
                for (int j = 0; j < 8; j++) {
                    PORTD =
                        posiciones[j];

                    PORTB =
                        ~GANADOR[i + j];

                    _delay_ms(0.05);
                }
            }
        }
    }

    // Apagar matriz después del mensaje.
    PORTD = 0x00;
    PORTB = 0xFF;
}

// ============================================================
// FUNCIÓN: estado_puntuacion
//
// Construye un mensaje con el puntaje obtenido y la palabra
// "PUNTOS". Luego, desplaza el contenido en la matriz.
//
// Ejemplo:
// 4 PUNTOS
// ============================================================

void estado_puntuacion(int puntos) {
    int decenas =
        puntos / 10;

    int unidades =
        puntos % 10;

    // Asociar cada número con su matriz correspondiente.
    unsigned char *digitos[10] = {
        DIGITO_0,
        DIGITO_1,
        DIGITO_2,
        DIGITO_3,
        DIGITO_4,
        DIGITO_5,
        DIGITO_6,
        DIGITO_7,
        DIGITO_8,
        DIGITO_9
    };

    // Buffer temporal para construir el mensaje final.
    unsigned char buffer[80];

    int tam =
        0;

    // Agregar espacio inicial.
    for (int i = 0; i < 8; i++) {
        buffer[tam++] =
            0x00;
    }

    // Agregar decenas cuando el puntaje sea mayor o igual a 10.
    if (puntos >= 10) {
        for (int i = 0; i < 8; i++) {
            buffer[tam++] =
                digitos[decenas][i];
        }
    }

    // Agregar unidades.
    for (int i = 0; i < 8; i++) {
        buffer[tam++] =
            digitos[unidades][i];
    }

    // Agregar la palabra PUNTOS.
    for (int i = 0; i < 48; i++) {
        buffer[tam++] =
            PUNTOS[i];
    }

    // Agregar espacio final.
    for (int i = 0; i < 8; i++) {
        buffer[tam++] =
            0x00;
    }

    // Desplazar el mensaje completo en la matriz.
    for (
        int i = 0;
        i <= tam - 8;
        i++
    ) {
        for (int k = 0; k < 50; k++) {
            for (int j = 0; j < 8; j++) {
                PORTD =
                    posiciones[j];

                PORTB =
                    ~buffer[i + j];

                _delay_ms(0.05);
            }
        }
    }

    // Apagar matriz después del mensaje.
    PORTD = 0x00;
    PORTB = 0xFF;
}

// ============================================================
// FUNCIÓN PRINCIPAL
//
// Configura puertos y ejecuta el ciclo completo del juego:
//
// 1. Mostrar bienvenida.
// 2. Seleccionar dificultad.
// 3. Ejecutar partida.
// 4. Mostrar mensaje de ganador o puntaje.
// 5. Reiniciar flujo.
// ============================================================

int main(void) {
    // PORTD controla las filas de la matriz.
    DDRD =
        0xFF;

    // PORTB controla las columnas de la matriz.
    DDRB =
        0xFF;

    // PC0-PC2 funcionan como salidas hacia el PIC.
    // PC3-PC5 funcionan como entradas para pulsadores.
    DDRC =
        0x07;

    // Activar pull-ups internos en PC3-PC5.
    // Mantener el bus PC0-PC2 inicialmente en 000.
    PORTC =
        0x38;

    // Preparar ADC6 para leer el botón SUBIR.
    inicializar_adc();

    while (1) {
        // Mostrar bienvenida antes de cada partida.
        estado_bienvenido();

        _delay_ms(10);

        // Permitir que el usuario seleccione el nivel.
        int nivel =
            estado_seleccion_nivel();

        // Ejecutar el Pong y guardar el resultado.
        int puntos =
            estado_juego(nivel);

        // Mostrar mensaje de victoria o puntuación final.
        if (puntos == -1) {
            estado_ganador();
        }
        else {
            estado_puntuacion(
                puntos
            );
        }
    }

    return 0;
}
