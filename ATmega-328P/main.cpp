#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdint.h>

// ============================================================
// PONG LED PARA UN JUGADOR
// Microcontrolador: ATmega328P
// Frecuencia: 8 MHz
//
// MATRIZ LED 8x8
// PORTD -> F0-F7: filas activas en bajo
// PORTB -> C0-C7: columnas activas en alto
//
// PULSADORES
// PC0 -> SUBIR
// PC1 -> BAJAR
// PC2 -> INICIAR
//
// COMUNICACIÓN PARALELA CON PIC16F887
// PC3 -> RA0 del PIC
// PC4 -> RA1 del PIC
// PC5 -> RA2 del PIC
//
// RESET FÍSICO
// PC6 / RESET -> resistencia de 10 kΩ hacia VCC
// PC6 / RESET -> pulsador hacia GND
// ============================================================

// ------------------------ Pulsadores -------------------------

#define BTN_UP      PC0
#define BTN_DOWN    PC1
#define BTN_START   PC2

// ---------------- Parámetros generales -----------------------

#define MATRIX_SIZE       8
#define WIN_SCORE         8
#define DEBOUNCE_TIME_MS  30

// ============================================================
// COMANDOS ENVIADOS AL PIC16F887
// ============================================================
//
// PC5 PC4 PC3
//  0   0   0  -> Silencio
//  0   0   1  -> Inicio
//  0   1   0  -> Rebote contra pared
//  0   1   1  -> Rebote contra raqueta
//  1   0   0  -> Cambio de nivel
//  1   0   1  -> Derrota
//  1   1   0  -> Victoria
// ============================================================

typedef enum {
    PIC_NONE       = 0,
    PIC_START      = 1,
    PIC_WALL       = 2,
    PIC_PADDLE     = 3,
    PIC_LEVEL      = 4,
    PIC_GAME_OVER  = 5,
    PIC_VICTORY    = 6
} PicCommand;

// ============================================================
// ESTADOS DEL JUEGO
// ============================================================

typedef enum {
    STATE_WAITING,
    STATE_RUNNING,
    STATE_GAME_OVER,
    STATE_VICTORY
} GameState;

GameState gameState = STATE_WAITING;

// ============================================================
// VARIABLES DE LA MATRIZ
// ============================================================

volatile uint8_t displayBuffer[MATRIX_SIZE] = {0};
volatile uint8_t currentRow = 0;
volatile uint32_t milliseconds = 0;

// ============================================================
// VARIABLES DEL JUEGO
// ============================================================

uint8_t currentLevel = 1;
uint16_t ballInterval = 650;

uint8_t paddleTop = 2;
uint8_t paddleSize = 3;

int8_t ballX = 4;
int8_t ballY = 3;

int8_t directionX = -1;
int8_t directionY = 1;

uint8_t score = 0;
uint32_t lastBallMovement = 0;

// ============================================================
// VARIABLES PARA GENERAR TRAYECTORIAS VARIABLES
// ============================================================

// Define cada cuántos desplazamientos horizontales
// se mueve verticalmente la pelota.
//
// 1 = diagonal pronunciada
// 2 = pendiente media
// 3 = pendiente suave

uint8_t verticalPeriod = 1;
uint8_t verticalCounter = 0;

// Semilla del generador pseudoaleatorio.

uint16_t randomState = 0xACE1u;

// ============================================================
// IMÁGENES ESTÁTICAS
// ============================================================

// Número 1

const uint8_t LEVEL_1[MATRIX_SIZE] = {
    0x00,
    0x18,
    0x38,
    0x18,
    0x18,
    0x18,
    0x7E,
    0x00
};

// Número 2

const uint8_t LEVEL_2[MATRIX_SIZE] = {
    0x00,
    0x3C,
    0x66,
    0x06,
    0x1C,
    0x30,
    0x7E,
    0x00
};

// Número 3

const uint8_t LEVEL_3[MATRIX_SIZE] = {
    0x00,
    0x3C,
    0x66,
    0x06,
    0x1C,
    0x66,
    0x3C,
    0x00
};

// X para indicar derrota

const uint8_t GAME_OVER_IMAGE[MATRIX_SIZE] = {
    0x81,
    0x42,
    0x24,
    0x18,
    0x18,
    0x24,
    0x42,
    0x81
};

// V para indicar victoria

const uint8_t VICTORY_IMAGE[MATRIX_SIZE] = {
    0x81,
    0x81,
    0x42,
    0x42,
    0x24,
    0x24,
    0x18,
    0x00
};

// ============================================================
// INTERRUPCIÓN PARA MULTIPLEXAR LA MATRIZ LED
// ============================================================
//
// Polaridad confirmada durante la prueba:
// - Columnas activas en alto.
// - Filas activas en bajo.
// ============================================================

ISR(TIMER0_COMPA_vect) {
    // Apagar temporalmente la matriz para reducir ghosting.
    PORTB = 0x00;
    PORTD = 0xFF;

    // Activar columnas necesarias.
    PORTB = displayBuffer[currentRow];

    // Activar solamente una fila.
    PORTD = (uint8_t)(~(1 << currentRow));

    currentRow++;

    if (currentRow >= MATRIX_SIZE) {
        currentRow = 0;
    }

    milliseconds++;
}

// ============================================================
// CONFIGURAR TIMER0
// ============================================================

void initTimer0(void) {
    // Timer0 en modo CTC.
    TCCR0A = (1 << WGM01);

    // Prescaler de 64.
    TCCR0B = (1 << CS01) | (1 << CS00);

    // 8 MHz / 64 = 125000 Hz
    // Interrupción cada 1 ms:
    // 125000 / 1000 - 1 = 124

    OCR0A = 124;

    // Activar interrupción por comparación.
    TIMSK0 = (1 << OCIE0A);
}

// ============================================================
// OBTENER TIEMPO EN MILISEGUNDOS
// ============================================================

uint32_t getMilliseconds(void) {
    uint32_t value;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        value = milliseconds;
    }

    return value;
}

// ============================================================
// GENERADOR PSEUDOALEATORIO
// ============================================================

uint8_t randomByte(void) {
    randomState ^= randomState << 7;
    randomState ^= randomState >> 9;
    randomState ^= randomState << 8;

    // Evitar que el estado quede en cero.
    if (randomState == 0) {
        randomState = 0xACE1u;
    }

    return (uint8_t)(randomState & 0xFF);
}

void randomizeTrajectory(void) {
    // Elegir si la pelota sube o baja.
    directionY = (randomByte() & 0x01) ? 1 : -1;

    // Seleccionar una pendiente entre 1, 2 y 3.
    verticalPeriod = 1 + (randomByte() % 3);

    verticalCounter = 0;
}

// ============================================================
// FUNCIONES DE VISUALIZACIÓN
// ============================================================

void copyImageToDisplay(const uint8_t image[MATRIX_SIZE]) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        for (uint8_t row = 0; row < MATRIX_SIZE; row++) {
            displayBuffer[row] = image[row];
        }
    }
}

// Invertir los bits para corregir números mostrados en espejo.

uint8_t reverseBits(uint8_t value) {
    value = ((value & 0xF0) >> 4) | ((value & 0x0F) << 4);
    value = ((value & 0xCC) >> 2) | ((value & 0x33) << 2);
    value = ((value & 0xAA) >> 1) | ((value & 0x55) << 1);

    return value;
}

void copyMirroredImageToDisplay(const uint8_t image[MATRIX_SIZE]) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        for (uint8_t row = 0; row < MATRIX_SIZE; row++) {
            displayBuffer[row] = reverseBits(image[row]);
        }
    }
}

void setPixel(uint8_t buffer[MATRIX_SIZE], int8_t x, int8_t y) {
    if (
        x < 0 ||
        x >= MATRIX_SIZE ||
        y < 0 ||
        y >= MATRIX_SIZE
    ) {
        return;
    }

    buffer[y] |= (1 << x);
}

void renderGame(void) {
    uint8_t gameImage[MATRIX_SIZE] = {0};

    // Dibujar raqueta vertical en la pared izquierda.
    for (uint8_t i = 0; i < paddleSize; i++) {
        setPixel(gameImage, 0, paddleTop + i);
    }

    // Dibujar pelota.
    setPixel(gameImage, ballX, ballY);

    copyImageToDisplay(gameImage);
}

// ============================================================
// COMUNICACIÓN PARALELA CON PIC16F887
// ============================================================

void setPicCommand(uint8_t command) {
    // Conservar los pull-ups internos de PC0-PC2.
    PORTC &= 0x07;

    // Colocar comando en PC3-PC5.
    PORTC |= ((command & 0x07) << 3);
}

void sendPicCommand(PicCommand command) {
    setPicCommand(command);

    // Mantener el comando activo para que el PIC pueda leerlo.
    _delay_ms(60);

    // Regresar a silencio.
    setPicCommand(PIC_NONE);
}

// ============================================================
// ANTIRREBOTE DE PULSADORES
// ============================================================

typedef struct {
    uint8_t lastRawState;
    uint8_t stableState;
    uint32_t changeTime;
} ButtonState;

ButtonState upButton = {0, 0, 0};
ButtonState downButton = {0, 0, 0};
ButtonState startButton = {0, 0, 0};

uint8_t buttonPressed(uint8_t pin, ButtonState *button) {
    uint8_t rawState;
    uint32_t now;

    // El pulsador está activo cuando conecta el pin hacia GND.
    rawState = !(PINC & (1 << pin));

    now = getMilliseconds();

    if (rawState != button->lastRawState) {
        button->lastRawState = rawState;
        button->changeTime = now;
    }

    if ((now - button->changeTime) >= DEBOUNCE_TIME_MS) {
        if (rawState != button->stableState) {
            button->stableState = rawState;

            if (button->stableState) {
                return 1;
            }
        }
    }

    return 0;
}

// ============================================================
// CONFIGURACIÓN DE NIVELES
// ============================================================

void applyLevelSettings(void) {
    switch (currentLevel) {
        case 1:
            // Fácil:
            // velocidad baja y raqueta de 3 LED.
            ballInterval = 650;
            paddleSize = 3;
            break;

        case 2:
            // Medio:
            // velocidad intermedia y raqueta de 3 LED.
            ballInterval = 400;
            paddleSize = 3;
            break;

        case 3:
            // Difícil:
            // velocidad elevada y raqueta de 2 LED.
            ballInterval = 230;
            paddleSize = 2;
            break;

        default:
            currentLevel = 1;
            ballInterval = 650;
            paddleSize = 3;
            break;
    }

    // Evitar que la raqueta salga del borde inferior.
    if (paddleTop > MATRIX_SIZE - paddleSize) {
        paddleTop = MATRIX_SIZE - paddleSize;
    }
}

void showCurrentLevel(void) {
    if (currentLevel == 1) {
        copyMirroredImageToDisplay(LEVEL_1);
    }

    if (currentLevel == 2) {
        copyMirroredImageToDisplay(LEVEL_2);
    }

    if (currentLevel == 3) {
        copyMirroredImageToDisplay(LEVEL_3);
    }

    sendPicCommand(PIC_LEVEL);

    _delay_ms(700);

    renderGame();
}

void selectNextLevel(void) {
    currentLevel++;

    if (currentLevel > 3) {
        currentLevel = 1;
    }

    applyLevelSettings();

    showCurrentLevel();
}

void selectPreviousLevel(void) {
    if (currentLevel == 1) {
        currentLevel = 3;
    } else {
        currentLevel--;
    }

    applyLevelSettings();

    showCurrentLevel();
}

// ============================================================
// INICIAR PARTIDA
// ============================================================

void startGame(void) {
    applyLevelSettings();

    // Centrar raqueta.
    paddleTop = (MATRIX_SIZE - paddleSize) / 2;

    // Ubicar pelota cerca del centro.
    ballX = 4;
    ballY = 3;

    // Variar semilla según el instante de pulsación.
    randomState ^= (uint16_t)getMilliseconds();
    randomState ^= ((uint16_t)PINC << 8);

    // La dirección horizontal inicial puede variar.
    directionX = (randomByte() & 0x01) ? 1 : -1;

    // Elegir dirección vertical y pendiente inicial.
    randomizeTrajectory();

    score = 0;

    lastBallMovement = getMilliseconds();

    gameState = STATE_RUNNING;

    sendPicCommand(PIC_START);

    renderGame();
}

// ============================================================
// FINALIZAR PARTIDA
// ============================================================

void finishGameAsDefeat(void) {
    gameState = STATE_GAME_OVER;

    sendPicCommand(PIC_GAME_OVER);

    copyImageToDisplay(GAME_OVER_IMAGE);
}

void finishGameAsVictory(void) {
    gameState = STATE_VICTORY;

    sendPicCommand(PIC_VICTORY);

    copyImageToDisplay(VICTORY_IMAGE);
}

// ============================================================
// MOVER RAQUETA
// ============================================================

void movePaddleUp(void) {
    if (paddleTop > 0) {
        paddleTop--;

        renderGame();
    }
}

void movePaddleDown(void) {
    if (paddleTop < MATRIX_SIZE - paddleSize) {
        paddleTop++;

        renderGame();
    }
}

// ============================================================
// ACTUALIZAR PELOTA
// ============================================================

void updateBall(void) {
    int8_t nextX;
    int8_t nextY;

    uint8_t wallCollision = 0;
    uint8_t moveVertically = 0;

    // La pelota siempre se mueve horizontalmente.
    nextX = ballX + directionX;

    // Mantener temporalmente la posición vertical.
    nextY = ballY;

    verticalCounter++;

    // Aplicar movimiento vertical según la pendiente seleccionada.
    if (verticalCounter >= verticalPeriod) {
        verticalCounter = 0;

        moveVertically = 1;

        nextY = ballY + directionY;
    }

    // --------------------------------------------------------
    // Rebote contra pared superior o inferior.
    // --------------------------------------------------------

    if (
        moveVertically &&
        (nextY < 0 || nextY >= MATRIX_SIZE)
    ) {
        directionY = -directionY;

        nextY = ballY + directionY;

        // Cambiar pendiente después del rebote.
        verticalPeriod = 1 + (randomByte() % 3);

        wallCollision = 1;
    }

    // --------------------------------------------------------
    // Rebote contra pared derecha.
    // --------------------------------------------------------

    if (nextX >= MATRIX_SIZE) {
        directionX = -directionX;

        nextX = ballX + directionX;

        // Variar pendiente después del rebote.
        verticalPeriod = 1 + (randomByte() % 3);

        wallCollision = 1;
    }

    if (wallCollision) {
        sendPicCommand(PIC_WALL);
    }

    // --------------------------------------------------------
    // Verificar contacto con pared izquierda.
    // --------------------------------------------------------

    if (nextX == 0 && directionX < 0) {
        uint8_t hitsPaddle;

        hitsPaddle =
            nextY >= paddleTop &&
            nextY < paddleTop + paddleSize;

        if (hitsPaddle) {
            ballX = 0;
            ballY = nextY;

            // Rebotar hacia la derecha.
            directionX = 1;

            score++;

            // Determinar dirección según zona de impacto.
            if (nextY == paddleTop) {
                // Parte superior de la raqueta.
                directionY = -1;
            } else if (
                nextY == paddleTop + paddleSize - 1
            ) {
                // Parte inferior de la raqueta.
                directionY = 1;
            } else {
                // Centro de la raqueta.
                directionY =
                    (randomByte() & 0x01) ? 1 : -1;
            }

            // Variar pendiente.
            verticalPeriod = 1 + (randomByte() % 3);

            verticalCounter = 0;

            sendPicCommand(PIC_PADDLE);

            if (score >= WIN_SCORE) {
                finishGameAsVictory();

                return;
            }

            renderGame();

            return;
        }

        // Pelota fuera de la raqueta.
        finishGameAsDefeat();

        return;
    }

    // Actualizar posición normal.
    ballX = nextX;
    ballY = nextY;

    renderGame();
}

// ============================================================
// FUNCIÓN PRINCIPAL
// ============================================================

int main(void) {
    // --------------------------------------------------------
    // Configurar matriz LED.
    // --------------------------------------------------------

    // PORTB controla columnas C0-C7.
    DDRB = 0xFF;

    // PORTD controla filas F0-F7.
    DDRD = 0xFF;

    // Estado apagado para esta matriz:
    // columnas en bajo y filas en alto.

    PORTB = 0x00;
    PORTD = 0xFF;

    // --------------------------------------------------------
    // Configurar pulsadores y comunicación con PIC.
    // --------------------------------------------------------

    // PC0-PC2: entradas.
    // PC3-PC5: salidas.

    DDRC = 0x38;

    // Activar pull-ups internos para botones PC0-PC2.

    PORTC = 0x07;

    // --------------------------------------------------------
    // Inicializar temporizador.
    // --------------------------------------------------------

    initTimer0();

    sei();

    // --------------------------------------------------------
    // Mostrar nivel inicial.
    // --------------------------------------------------------

    applyLevelSettings();

    showCurrentLevel();

    // ========================================================
    // BUCLE PRINCIPAL
    // ========================================================

    while (1) {
        uint8_t pressUp;
        uint8_t pressDown;
        uint8_t pressStart;

        pressUp =
            buttonPressed(BTN_UP, &upButton);

        pressDown =
            buttonPressed(BTN_DOWN, &downButton);

        pressStart =
            buttonPressed(BTN_START, &startButton);

        // ----------------------------------------------------
        // Juego en ejecución.
        // ----------------------------------------------------

        if (gameState == STATE_RUNNING) {
            if (pressUp) {
                movePaddleUp();
            }

            if (pressDown) {
                movePaddleDown();
            }

            // INICIAR también permite reiniciar la ronda.
            if (pressStart) {
                startGame();
            }

            if (
                getMilliseconds() - lastBallMovement
                >= ballInterval
            ) {
                lastBallMovement = getMilliseconds();

                updateBall();
            }
        }

        // ----------------------------------------------------
        // Juego detenido.
        // ----------------------------------------------------

        else {
            // SUBIR: siguiente nivel.
            if (pressUp) {
                selectNextLevel();
            }

            // BAJAR: nivel anterior.
            if (pressDown) {
                selectPreviousLevel();
            }

            // INICIAR: comenzar partida.
            if (pressStart) {
                startGame();
            }
        }
    }

    return 0;
}