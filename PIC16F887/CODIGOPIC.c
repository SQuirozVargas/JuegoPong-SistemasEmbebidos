/* ============================================================
   DEBER 2: VIDEOJUEGO PONG
   PIC16F887 - MikroC for PIC
   Frecuencia: 8 MHz

   CONEXIONES:
   RA0 <- PC0 del ATmega328P
   RA1 <- PC1 del ATmega328P
   RA2 <- PC2 del ATmega328P

   RC3 -> Speaker -> GND
   ============================================================ */

// ============================================================
// FUNCIÓN GENERAL PARA REPRODUCIR TONOS
// ============================================================

void reproducir_tono(
    unsigned int frecuencia,
    unsigned int duracion
) {
    Sound_Play(frecuencia, duracion);
    Delay_ms(10);
}

// ============================================================
// SONIDOS CORTOS
// ============================================================

void sonido_pared() {
    reproducir_tono(523, 35);   // Do5
}

void sonido_paleta() {
    reproducir_tono(784, 60);   // Sol5
}

void sonido_nivel() {
    reproducir_tono(659, 70);   // Mi5
    reproducir_tono(784, 90);   // Sol5
}

// ============================================================
// MELODÍAS PRINCIPALES
// ============================================================

void melodia_inicio() {
    reproducir_tono(523, 80);   // Do5
    reproducir_tono(659, 80);   // Mi5
    reproducir_tono(784, 130);  // Sol5
}

void melodia_derrota() {
    reproducir_tono(392, 130);  // Sol4
    reproducir_tono(330, 160);  // Mi4
    reproducir_tono(262, 260);  // Do4
}

void melodia_victoria() {
    reproducir_tono(523, 90);    // Do5
    reproducir_tono(659, 90);    // Mi5
    reproducir_tono(784, 110);   // Sol5
    reproducir_tono(1046, 260);  // Do6
}

// ============================================================
// MÚSICA DE BIENVENIDA
// ============================================================
//
// Se utilizan notas breves para que el PIC pueda comprobar
// frecuentemente si el ATmega ya retiró el comando 111.
// ============================================================


void musica_bienvenida() {
    if ((PORTA & 0x07) == 7) {
        reproducir_tono(262, 140);   // Do4
    }

    if ((PORTA & 0x07) == 7) {
        Delay_ms(80);
        reproducir_tono(330, 140);   // Mi4
    }

    if ((PORTA & 0x07) == 7) {
        Delay_ms(80);
        reproducir_tono(392, 180);   // Sol4
    }

    if ((PORTA & 0x07) == 7) {
        Delay_ms(80);
        reproducir_tono(440, 180);   // La4
    }

    if ((PORTA & 0x07) == 7) {
        Delay_ms(80);
        reproducir_tono(370, 180);   // Fa_s4
    }

    if ((PORTA & 0x07) == 7) {
        Delay_ms(80);
        reproducir_tono(311, 180);   // re_s4
    }

    Delay_ms(500);
}





// ============================================================
// LEER COMANDO PARALELO
// ============================================================

unsigned short leer_comando() {
    return PORTA & 0x07;
}

// ============================================================
// PROGRAMA PRINCIPAL
// ============================================================

void main() {
    unsigned short comando;

    // Desactivar entradas analógicas.
    ANSEL = 0x00;
    ANSELH = 0x00;

    // Desactivar comparadores.
    C1ON_bit = 0;
    C2ON_bit = 0;

    // RA0-RA2 como entradas.
    TRISA = 0xFF;

    // PORTC como salida.
    TRISC = 0x00;

    PORTA = 0x00;
    PORTC = 0x00;

    // Speaker conectado a RC3.
    Sound_Init(&PORTC, 3);

    while (1) {
        comando = leer_comando();

        if (comando == 1) {
            melodia_inicio();

            // Esperar a que el ATmega retire el comando.
            while ((PORTA & 0x07) != 0) {
                Delay_ms(1);
            }
        }
        else if (comando == 2) {
            sonido_pared();

            while ((PORTA & 0x07) != 0) {
                Delay_ms(1);
            }
        }
        else if (comando == 3) {
            sonido_paleta();

            while ((PORTA & 0x07) != 0) {
                Delay_ms(1);
            }
        }
        else if (comando == 4) {
            sonido_nivel();

            while ((PORTA & 0x07) != 0) {
                Delay_ms(1);
            }
        }
        else if (comando == 5) {
            melodia_derrota();

            while ((PORTA & 0x07) != 0) {
                Delay_ms(1);
            }
        }
        else if (comando == 6) {
            melodia_victoria();

            while ((PORTA & 0x07) != 0) {
                Delay_ms(1);
            }
        }
        else if (comando == 7) {
            // La melodía se repite mientras el mensaje BIENVENIDO
            // permanezca visible en la matriz.
            musica_bienvenida();
        }

        Delay_ms(1);
    }
}