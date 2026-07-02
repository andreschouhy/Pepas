#ifndef PS2_H
#define PS2_H

#include "Arduino.h"

// Numero de interrupcion externa para el pin de clock PS/2
#define CLOCK_PIN_INT 1

// Codigos de framing/estado del protocolo PS/2
#define PS2_EXT          0xE0  // prefijo de tecla extendida
#define PS2_BREAK        0xF0  // prefijo de tecla soltada
#define PS2_ACK          0xFA
#define PS2_BAT_OK       0xAA
#define PS2_PAUSE_PREFIX 0xE1
#define PS2_ERROR        0x00

// Estado compartido con las ISR / la app
extern volatile bool inhibiting;

// Evento de tecla ya decodificado
struct TeclaEvento {
  uint8_t scancode;
  bool extendida;  // venia precedida por E0
  bool soltando;   // venia precedida por F0 (break)
};

// Inicializacion / control de lineas
void ps2Init();
void releaseClock();
void releaseData();

// ISRs (se pasan a attachInterrupt desde la app)
void ps2int_read();
void ps2int_write();

// Buffer / IO de bajo nivel
bool ps2Available();
uint8_t ps2Read();
void enviar(uint8_t valor);

// Decodificador: drena el buffer y devuelve el siguiente evento de tecla completo.
// Devuelve false si no hay un evento completo disponible.
bool ps2NextKey(TeclaEvento &ev);

// True (una sola vez) si el teclado mando su BAT (0xAA): replug/brownout/reset.
bool ps2HuboReset();

#endif
