// logre eliminar los delay de eventoTeclado, pero se cuelga al soltar una nota sin el "mantener" activo (aparentemente solo a veces)
// con shift, no esta propagando los cambios de octava
// ver si se generan bardos al volverse loco con las notas y el shift

#include "Arduino.h"

const int8_t ledPin = 13;
const int8_t multTemp = 8; // 8
const unsigned int precision = 1000; // esto aumenta la precision matematica en divisiones. 1000
const unsigned long capacidad = 65536L * precision; // (2 ^ 16)
//int8_t ledState = LOW;
int8_t cantPresionadas, notasPresionadas, pausa, E0Key, F0Byte = 0;
const uint8_t cantPepas = 4;
uint8_t presionadas[20];
uint8_t selector = 0;
uint8_t shift = 0;
uint8_t estadosLED = 0;
long pote = 0;
long prevMillis, futurMillis, futureMillis, currentMillis, diferencia = 0;
boolean controlarVelocidad, setTempo = 0;
long velocidadGeneral = 512L * precision;
long poteSnapshotGral;
const unsigned int cantTaps = 16;
unsigned long tap[cantTaps];
unsigned int tempo = 0;
float ajusteFinoVel = 1.0;

uint8_t mapa[34][2] = {
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
  for(uint8_t i = 0; i < 34; i++) if(val == mapa[i][0]) return mapa[i][1];
  return 0;
}

uint8_t mapaNum[10][2] = {
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
  for(uint8_t i = 0; i < 10; i++) if(val == mapaNum[i][0]) return mapaNum[i][1];
  return -1;
}

#define CLOCK_PIN_INT 1
const int8_t DataPin = 2;
const int8_t ClockPin = 3;

#define BUFFER_SIZE 45
static volatile uint8_t buffer[BUFFER_SIZE];
static volatile uint8_t head, tail;
static volatile bool inhibiting;

// Open collector utility routines
static inline void holdClock() {
  digitalWrite(ClockPin, LOW); // pullup off
  pinMode(ClockPin, OUTPUT); // pull clock low
}

static inline void releaseClock() {
  pinMode(ClockPin, INPUT); // release line
  digitalWrite(ClockPin, HIGH); // pullup on
}

static inline void holdData() {
  digitalWrite(DataPin, LOW); // pullup off
  pinMode(DataPin, OUTPUT); // pull clock low
}

static inline void releaseData() {
  pinMode(DataPin, INPUT); // release line
  digitalWrite(DataPin, HIGH); // pullup on
}

// The ISR for the external interrupt in write mode
void ps2int_read() {
  static uint8_t bitcount=0, incoming=0;
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
  }
  prev_ms = now_ms;
  n = bitcount - 1;
  if (n <= 7) {
    incoming |= (val << n);
  }
  bitcount++;
  if (bitcount == 11) {
    uint8_t i = head + 1;
    if (i >= BUFFER_SIZE) i = 0;
    if (i != tail) {
      buffer[i] = incoming;
      head = i;
    }
    bitcount = 0;
    incoming = 0;
  }
}

static volatile uint8_t writeByte;
static volatile uint8_t curbit = 0, parity = 0, ack;

// The ISR for the external interrupt in read mode
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

// Check if data available in ring buffer
bool ps2Available() {
  return head != tail;
}

// Read a byte from ring buffer (or return \0 if empty)
static inline uint8_t ps2Read() {
  uint8_t c, i;

  i = tail;
  if (i == head) return 0;
  i++;
  if (i >= BUFFER_SIZE) i = 0;
  c = buffer[i];
  tail = i;
  return c;
}

// Prepare a byte for sending to PS/2 device
static inline void ps2Write(uint8_t Byte) {
  writeByte = Byte;
  curbit = parity = ack = 0;
}

// Utility function to convert hex into number
int fromHex(char ch) {
  if(ch >= '0' && ch <= '9')
    return ch - '0';
  else if(ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  else if(ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;

  return 0;
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

void enviar(uint8_t valor)
{
  inhibiting = true;
  holdClock();
  ps2Write(valor); // send byte to PS/2 device
  holdData();
  releaseClock();
  while(curbit < 11) {} // wait until receive complete - MAY HANG!
}

void triggerLED(uint8_t _id, uint8_t _estado)
{
  if(_id == selector){
    attachInterrupt(CLOCK_PIN_INT, ps2int_write, FALLING);
    uint8_t estadosLEDcopia = estadosLED;
    bitWrite(estadosLEDcopia, 1, _estado);
    enviar(0xED);
    enviar(estadosLEDcopia);
    attachInterrupt(CLOCK_PIN_INT, ps2int_read, FALLING);
    releaseClock();
    inhibiting = false;
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
  if(a < 6553) //65535/10
  {
    return ((a * 10) + b);
  }
}

//void atualizarLED()
//{
  //if (cantPresionadas > 0) digitalWrite(ledPin, HIGH);
  //else digitalWrite(ledPin, LOW);
//}
