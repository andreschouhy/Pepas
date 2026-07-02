#include "Arduino.h"
#include "ps2.h"

// ===== Driver PS/2 (transporte + decodificacion) =====
// Extraido de Pepas.ino. Las variables compartidas con las ISR son volatile.

#define PS2_BUFFER_SIZE 45

static const int8_t DataPin = 2;
static const int8_t ClockPin = 3;

static volatile uint8_t buffer[PS2_BUFFER_SIZE];
static volatile uint8_t head, tail;
volatile bool inhibiting;   // compartida con la app (extern en ps2.h)

// Se marca cuando el teclado manda su BAT (0xAA) al energizarse: replug / brownout / reset.
// La app la consume via ps2HuboReset() para re-sincronizar los LEDs, sin tocar la secuencia.
static bool tecladoReset = false;

// ----- Utilidades open-collector -----
static inline void holdClock() {
  digitalWrite(ClockPin, LOW); // pullup off
  pinMode(ClockPin, OUTPUT);   // pull clock low
}

void releaseClock() {
  pinMode(ClockPin, INPUT);    // release line
  digitalWrite(ClockPin, HIGH); // pullup on
}

static inline void holdData() {
  digitalWrite(DataPin, LOW);  // pullup off
  pinMode(DataPin, OUTPUT);    // pull data low
}

void releaseData() {
  pinMode(DataPin, INPUT);     // release line
  digitalWrite(DataPin, HIGH); // pullup on
}

// ----- ISR de lectura -----
void ps2int_read() {
  static uint8_t bitcount=0, incoming=0, parityCalc=0;
  static uint32_t prev_ms=0;
  uint32_t now_ms;
  uint8_t n, val;

  if(inhibiting)
    return; // do nothing when clock manipulated by Arduino

  val = digitalRead(DataPin);
  now_ms = millis();
  if (now_ms - prev_ms > 250) {
    bitcount = 0;
    incoming = 0;
    parityCalc = 0;
  }
  prev_ms = now_ms;
  n = bitcount - 1;
  if (n <= 7) {              // bitcount 1..8 -> bits de datos (LSB primero)
    incoming |= (val << n);
    parityCalc ^= val;       // acumular paridad de los 8 bits de datos
  } else if (bitcount == 9) {  // bit de paridad (impar): datos^paridad debe dar 1
    if ((parityCalc ^ val) != 1) {  // paridad mala -> frame corrupto (glitch de bus / hotplug), descartar
      bitcount = incoming = parityCalc = 0;
      return;
    }
  } else if (bitcount == 10) {  // stop bit debe ser 1
    if (val != 1) {               // framing malo -> descartar
      bitcount = incoming = parityCalc = 0;
      return;
    }
  }
  bitcount++;
  if (bitcount == 11) {
    uint8_t i = head + 1;
    if (i >= PS2_BUFFER_SIZE) i = 0;
    if (i != tail) {
      buffer[i] = incoming;
      head = i;
    }
    bitcount = 0;
    incoming = 0;
    parityCalc = 0;
  }
}

static volatile uint8_t writeByte;
static volatile uint8_t curbit = 0, parity = 0, ack;

// ----- ISR de escritura -----
void ps2int_write() {
  if(curbit < 8) {
    if(writeByte & 1) {
      parity ^= 1;
      digitalWrite(DataPin, HIGH);
    } else
      digitalWrite(DataPin, LOW);

    writeByte >>= 1;
  } else if(curbit == 8) { // parity
    if(parity)
      digitalWrite(DataPin, LOW);
    else
      digitalWrite(DataPin, HIGH);
  } else if(curbit == 9) { // time to let go
    releaseData();
  } else { // time to check device ACK and hold clock again
    holdClock();
    ack = !digitalRead(DataPin);
  }

  curbit++;
}

// ----- Ring buffer -----
void ps2Init() {
  head = tail = 0;
}

bool ps2Available() {
  return head != tail;
}

uint8_t ps2Read() {
  uint8_t c, i;

  i = tail;
  if (i == head) return 0;
  i++;
  if (i >= PS2_BUFFER_SIZE) i = 0;
  c = buffer[i];
  tail = i;
  return c;
}

static inline void ps2Write(uint8_t Byte) {
  writeByte = Byte;
  curbit = parity = ack = 0;
}

// ----- Enviar un byte al teclado (busy-wait con timeout) -----
void enviar(uint8_t valor) {
  inhibiting = true;
  holdClock();
  ps2Write(valor); // enviar el byte al dispositivo PS/2
  holdData();
  releaseClock();
  // Esperar a que termine la transmision (curbit llega a 11), con timeout para no colgarse.
  unsigned long inicioEnvio = millis();
  while(curbit < 11) {
    if(millis() - inicioEnvio > 50) break; // margen de sobra para 11 bits
  }
}

// ----- Decodificador: bytes crudos -> evento de tecla completo -----
// Drena el buffer acumulando prefijos E0/F0 hasta tener un scancode real.
// No introduce pausas: consume rafagas enteras para que el ring buffer no se desborde
// (la causa de los bytes perdidos que desincronizaban el conteo de notas).
bool ps2NextKey(TeclaEvento &ev) {
  static bool ext = false, brk = false;
  while (ps2Available()) {
    uint8_t b = ps2Read();
    switch (b) {
      case PS2_BAT_OK:
        tecladoReset = true; // el teclado se energizo/reinicio (replug/brownout)
        ext = brk = false;
        continue;
      case PS2_ACK:
      case PS2_PAUSE_PREFIX:
      case PS2_ERROR:
        ext = brk = false; // codigos de estado: descartar y resetear prefijos
        continue;
      case PS2_EXT:
        ext = true;
        continue;
      case PS2_BREAK:
        brk = true;
        continue;
      default:
        ev.scancode = b;
        ev.extendida = ext;
        ev.soltando = brk;
        ext = brk = false;
        return true;
    }
  }
  return false;
}

// Devuelve true una sola vez si el teclado mando su BAT (0xAA) desde la ultima consulta.
bool ps2HuboReset() {
  bool r = tecladoReset;
  tecladoReset = false;
  return r;
}
