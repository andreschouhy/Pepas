// Reset/panico y persistencia de estado.
//   - softReset()     Ctrl+Alt+Supr  : silenciar notas trabadas + soltar modificadores (recuperacion)
//   - factoryReset()  Ctrl+Shift+Esc : todo a los valores de encendido
//   - guardarEstado() Ctrl+Shift+Enter: escribir el patch actual en EEPROM
//   - cargarEstado()  (en el arranque): recuperar el patch guardado, si hay uno valido
//
// EEPROM.h se incluye en Pepas.ino (primer archivo del sketch), asi esta disponible en todos.

// Magic al inicio del EEPROM: distingue "hay un patch guardado" de un EEPROM en blanco (0xFF).
// Subir el segundo byte si cambia el layout de guardado, para invalidar patches viejos.
#define EEPROM_MAGIC0 0x50  // 'P'
#define EEPROM_MAGIC1 0x33  // version de layout

static void limpiarTeclasFisicas()
{
  cantPresionadas = 0;
  notasPresionadas = 0;
  for (uint8_t i = 0; i < 20; i++) presionadas[i] = 0;
  shift = 0;
  controlarVelocidad = 0;
  setTempo = 0;
}

// Soft reset / panico (Ctrl+Alt+Supr). Recupera de notas/gates trabados (p.ej. un break code
// perdido) y libera modificadores atascados. NO toca secuencias, tempo, octavas ni parametros.
void softReset()
{
  limpiarTeclasFisicas();
  for (uint8_t i = 0; i < cantPepas; i++) pepas[i]->silenciar();
}

// Factory reset (Ctrl+Shift+Esc). Todo a como arranca el sistema.
void factoryReset()
{
  limpiarTeclasFisicas();
  tempo = 0;
  selector = 0;
  velocidadGeneral = 512L * precision;
  for (uint8_t i = 0; i < cantPepas; i++) pepas[i]->reset();
  actualizarLEDSelector();
}

// Guardar el estado actual en EEPROM (Ctrl+Shift+Enter). Bloquea ~1-2s mientras escribe
// (el secuenciador se congela ese rato); es una accion deliberada del usuario, aceptable.
//
// NOTA A FUTURO (requiere hardware): un guardado AUTOMATICO al apagar necesita un detector de
// brownout que dispare una interrupcion apenas cae la tension, mas un capacitor reservorio que
// mantenga vivo el micro lo suficiente para terminar de escribir el EEPROM (~1-2s son demasiados
// sin ese cap). Sin esa modificacion de hardware no es posible; por eso el guardado es por combo.
void guardarEstado()
{
  int addr = 0;
  EEPROM.update(addr++, EEPROM_MAGIC0);
  EEPROM.update(addr++, EEPROM_MAGIC1);
  EEPROM.put(addr, velocidadGeneral); addr += sizeof(velocidadGeneral);
  EEPROM.update(addr++, selector);
  for (uint8_t i = 0; i < cantPepas; i++)
  {
    addr = pepas[i]->guardarEEPROM(addr);
    wdt_reset(); // cada voz es ~0.5s de escritura; patear el WDT para que no dispare durante el guardado
  }
}

// Cargar el patch guardado en el arranque. Si el magic no coincide (EEPROM en blanco o layout
// viejo), no hace nada y quedan los defaults del constructor.
void cargarEstado()
{
  if (EEPROM.read(0) != EEPROM_MAGIC0 || EEPROM.read(1) != EEPROM_MAGIC1)
    return;

  int addr = 2;
  EEPROM.get(addr, velocidadGeneral); addr += sizeof(velocidadGeneral);
  selector = EEPROM.read(addr++);
  if (selector >= cantPepas) selector = 0;
  for (uint8_t i = 0; i < cantPepas; i++) addr = pepas[i]->cargarEEPROM(addr);

  // arrancar limpio el timing de cada voz con el patch ya cargado
  for (uint8_t i = 0; i < cantPepas; i++) pepas[i]->reiniciarCabezal();
}
