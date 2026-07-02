// Reescritura de la lectura de teclado (PROBAR EN EL DISPOSITIVO):
//   - driver PS/2 movido a ps2.h/ps2.cpp (transporte + decodificador)
//   - el decodificador (ps2NextKey) drena el buffer entero sin la pausa de ~512ms; antes
//     esa pausa dejaba desbordar el ring buffer y se perdian break codes -> el conteo de
//     notas se desincronizaba y el gesto de "una nota resetea la escala en mantener" fallaba
//   - conteo de notas derivado de presionadas[] (recontarNotas), no de un contador ++/--
//   - soltar es simetrico/seguro: ya no corrompe presionadas[] con un break huerfano
// Pendiente/conocido: con shift no se propagan los cambios de octava (subir/bajarOctava solo
//   actuan sobre pepas[selector]). Presets sacados por sospecha de saturar memoria.

#include "Arduino.h"
#include "ps2.h"
#include <EEPROM.h>  // persistencia de estado (guardar/cargar patch). Ver d_estado.ino

const int8_t extClockPin = 0;
const int8_t extClockSwitchPin = 1;
const int8_t multTemp = 8; // 8
const unsigned int precision = 1000; // esto aumenta la precision matematica en divisiones. 1000
const unsigned long capacidad = 65536L * precision; // (2 ^ 16)
int8_t cantPresionadas = 0, notasPresionadas = 0;
const uint8_t cantPepas = 4;
uint8_t presionadas[20];
uint8_t selector = 0;
uint8_t shift = 0;
uint8_t estadosLED = 0;
long pote = 0;
long prevMillis, currentMillis, clockMillisPrev, clockMillisPrevPrev, clockDifCurrent, clockDifPrev, deltaMillis = 0;
boolean clockCheck, clockSwitch, controlarVelocidad, setTempo = 0;
long velocidadGeneral = 512L * precision;
long poteSnapshotGral;
const unsigned int cantTaps = 16;
unsigned long tap[cantTaps];
unsigned int tempo = 0;

// Scancodes PS/2 (set 2) usados por el dispatcher. Antes eran numeros magicos.
#define SC_LCTRL    0x14
#define SC_LALT     0x11
#define SC_LSHIFT   0x12
#define SC_F1       0x05
#define SC_F2       0x06
#define SC_CAPS     0x58  // mantener
#define SC_SPACE    0x29  // secuenciar
#define SC_BKSP     0x66  // resetear secuencia
#define SC_TAB      0x0D  // cambiar selector
#define SC_ESC      0x76  // reiniciar cabezal (shift+ESC: tap tempo, desconectado)
#define SC_BACKTICK 0x0E  // sincronizar
#define SC_KP_MULT  0x7C  // multiplicar velocidad
#define SC_KP_DIV   0x4A  // dividir velocidad (con E0)
#define SC_UP       0x75  // subir octava (con E0)
#define SC_DOWN     0x72  // bajar octava (con E0)
#define SC_ENTER    0x5A  // enter (ctrl+shift+enter: guardar estado en EEPROM)
#define SC_DEL      0x71  // supr/delete extendida (ctrl+alt+supr: soft reset). Con E0; sin E0 es el "." del pad

// Mapa de scancode PS/2 -> nota MIDI. Vive en flash (PROGMEM) para no gastar RAM.
const uint8_t mapa[34][2] PROGMEM = {
{0x1A,21}, // z
{0x1B,22}, // s
{0x22,23}, // x
{0x23,24}, // d
{0x21,25}, // c
{0x2A,26}, // v
{0x34,27}, // g
{0x32,28}, // b
{0x33,29}, // h
{0x31,30}, // n
{0x3B,31}, // j
{0x3A,32}, // m
{0x41,33}, // ,
{0x4B,34}, // l
{0x49,35}, // .
{0x4C,36}, // ;
{0x4A,37}, // /
{0x15,33}, // q
{0x1E,34}, // 2
{0x1D,35}, // w
{0x26,36}, // 3
{0x24,37}, // e
{0x2D,38}, // r
{0x2E,39}, // 5
{0x2C,40}, // t
{0x36,41}, // 6
{0x35,42}, // y
{0x3D,43}, // 7
{0x3C,44}, // u
{0x43,45}, // i
{0x46,46}, // 9
{0x44,47}, // o
{0x45,48}, // 0
{0x4D,49} // p
};

uint8_t K2Midi(uint8_t val)
{
  for(uint8_t i = 0; i < sizeof(mapa)/sizeof(mapa[0]); i++) if(val == pgm_read_byte(&mapa[i][0])) return pgm_read_byte(&mapa[i][1]);
  return 0;
}

// Mapa de scancode PS/2 -> digito del teclado numerico. Tambien en flash.
const uint8_t mapaNum[10][2] PROGMEM = {
{0x70,0}, // 0
{0x69,1}, // 1
{0x72,2}, // 2
{0x7A,3}, // 3
{0x6B,4}, // 4
{0x73,5}, // 5
{0x74,6}, // 6
{0x6C,7}, // 7
{0x75,8}, // 8
{0x7D,9}, // 9
};

int8_t K2Num(uint8_t val)
{
  for(uint8_t i = 0; i < sizeof(mapaNum)/sizeof(mapaNum[0]); i++) if(val == pgm_read_byte(&mapaNum[i][0])) return pgm_read_byte(&mapaNum[i][1]);
  return -1;
}

void setPwmFrequency(int pin, int divisor)
{
  uint8_t mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) 
  {
    switch(divisor) 
    {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
  
    if(pin == 5 || pin == 6) 
    {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } 
    else 
    {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } 
  else if(pin == 3 || pin == 11) 
  {
    switch(divisor) 
    {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

int8_t buscar(uint8_t valor)
{
  for(int8_t i = 0; i < cantPresionadas; i++) if(presionadas[i] == valor) return i;
  return -1;
}

// Feedback de nota-on del canal seleccionado. Antes parpadeaba el LED de Num Lock del
// teclado por PS/2: cada nota disparaba enviar() bloqueante (con swap de interrupcion e
// inhibicion del bus) DENTRO de actualizar(), metiendo varios ms de jitter en la secuencia.
// Ahora es un LED en GPIO comun con parpadeo temporizado no bloqueante, al estilo de
// trigger()/triggerLoop(): triggerLED() solo prende el LED y anota cuando apagarlo, y
// notaLedLoop() (llamada en loop()) lo apaga sin bloquear. digitalWrite es ~microsegundos.
const int8_t notaLedPin = A1;        // GPIO libre (analogico A1 = digital 15)
const uint8_t notaLedDuracion = 10;  // en unidades de currentMillis (~80ms con multTemp=8)
unsigned long notaLedOffMillis = 0;

void triggerLED(uint8_t _id)
{
  if (_id == selector)
  {
    digitalWrite(notaLedPin, HIGH);
    notaLedOffMillis = currentMillis + notaLedDuracion;
  }
}

void notaLedLoop()
{
  if (notaLedOffMillis != 0 && currentMillis >= notaLedOffMillis)
  {
    notaLedOffMillis = 0;
    digitalWrite(notaLedPin, LOW);
  }
}

void insertarTap() // esta funcion esta en desarrollo, aun no esta funcional
{
  for(uint8_t i = cantTaps-1; i > 0; i--) tap[i] = tap[i-1];
  tap[0] = currentMillis;
  
  unsigned long dif, prom = 0;
  uint8_t cant = 0;
  for(uint8_t i = 0; i < cantTaps-1; i++) 
  {
    if(tap[i] != 0 && tap[i+1] != 0)
    {
      dif += (tap[i] - tap[i+1]);
      cant++;
    }
  }
  if (cant > 0) 
  {
    prom = dif / cant;
    prom *= 10; // no tengo idea por que tengo que multiplicarlo por 10 (?????)
    velocidadGeneral = capacidad / prom; 
  }
}

unsigned int concatenar(unsigned int a, uint8_t b)
{
  if(a < 6553) //65535/10, evita que el unsigned int se desborde al agregar un digito
    return ((a * 10) + b);
  return a; // si ya no entra otro digito, devolver el valor sin cambios (antes no devolvia nada)
}
