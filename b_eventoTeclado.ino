// Recuenta cuantas notas estan fisicamente presionadas mirando presionadas[].
// Fuente unica de verdad: antes notasPresionadas se llevaba con ++/-- y bastaba
// perder un break code para que quedara desincronizado para siempre (y entonces
// el gesto de "una sola nota resetea la escala en mantener" dejaba de funcionar).
void recontarNotas()
{
  uint8_t n = 0;
  for (int8_t i = 0; i < cantPresionadas; i++)
    if (K2Midi(presionadas[i]) > 0) n++;
  notasPresionadas = n;
}

// Refleja el canal seleccionado en los LEDs del teclado (los 2 de la derecha, en binario 0..3).
// El LED de la izquierda queda libre. Usa enviar() (bloqueante) pero solo ante eventos raros
// (TAB, replug del teclado, factory reset), fuera del path de timing -> costo despreciable.
void actualizarLEDSelector()
{
  attachInterrupt(CLOCK_PIN_INT, ps2int_write, FALLING);
  enviar(0xED);
  uint8_t m = 0;
  if (selector == 1) bitWrite(m, 0, 1);
  else if (selector == 2) bitWrite(m, 2, 1);
  else if (selector == 3) { bitWrite(m, 0, 1); bitWrite(m, 2, 1); }
  enviar(m);
  estadosLED = m;
  attachInterrupt(CLOCK_PIN_INT, ps2int_read, FALLING);
  releaseClock();
  inhibiting = false;
}

// Aplica una llamada al canal seleccionado, o a todos si shift esta activo. Reemplaza el patron
// repetido ~18 veces: if(shift) for(i) pepas[i]->X; else pepas[selector]->X;
// Nota: el argumento NO puede referenciar miembros de Pepa sin calificar (solo se prefija la
// llamada externa con pepas[_b]->), por eso el bloque del teclado numerico sigue manual.
#define BROADCAST(call) do {                                        \
    if (shift) for (uint8_t _b = 0; _b < cantPepas; _b++) pepas[_b]->call; \
    else pepas[selector]->call;                                     \
  } while (0)

void manejarPresionar(TeclaEvento &ev)
{
  uint8_t sc = ev.scancode;

  if (buscar(sc) != -1) return;       // ya estaba presionada (typematic repeat), ignorar
  if (cantPresionadas >= 20) return;  // presionadas[] lleno, no desbordar el array

  presionadas[cantPresionadas] = sc;
  cantPresionadas++;
  recontarNotas();

  if (K2Midi(sc) > 0 && !ev.extendida)  // presionando una nota
  {
    BROADCAST(agregar(sc));
  }
  else if (sc == SC_CAPS) // mantener
  {
    BROADCAST(mantenerSwitch());
  }
  else if (sc == SC_SPACE) // secuenciar
  {
    BROADCAST(secuenciarSwitch());
  }
  else if (sc == SC_BKSP) // resetear secuencia (Ctrl+Shift: cargar el estado guardado)
  {
    if (buscar(SC_LCTRL) != -1 && shift == 1)
    {
      cargarEstado(); // Ctrl+Shift+Backspace: recuperar el ultimo patch guardado en EEPROM
      // Ctrl sigue fisicamente apretado -> controlarVelocidad==1 y el loop reescribiria
      // velocidadGeneral desde el pote; re-anclar el snapshot para que el tempo cargado no se pise.
      poteSnapshotGral = velocidadGeneral - ((long)pote * precision);
    }
    else
      BROADCAST(resetearSecuencia());
  }
  else if (sc == SC_LCTRL && pepas[selector]->control == 0)
  {
    controlarVelocidad = 1;
    poteSnapshotGral = velocidadGeneral - ((long)pote * precision);
    BROADCAST(controlarCTRL(1));
  }
  else if (sc == SC_LALT && pepas[selector]->control == 0)
  {
    BROADCAST(controlarALT(1));
  }
  else if (sc == SC_F1 && pepas[selector]->control == 0)
  {
    BROADCAST(controlarF1(1));
  }
  else if (sc == SC_F2)
  {
    setTempo = 1;
  }
  // F5-F8 quedaron libres al sacar los presets, disponibles para futuras funciones
  else if (sc == SC_BACKTICK) // sincronizar
  {
    for (uint8_t i = 0; i < cantPepas; i++)
    {
      if (i != selector)
      {
        // Cuentas en 64 bits: aT (hasta ~capacidad) * multiplicadores (hasta 32*32) desborda
        // un long de 32 bits. Antes se hacia en long y el while podia quedar con basura.
        long long aT = pepas[selector]->timingCap;
        long aM = pepas[selector]->multiplicador;
        long long bT = pepas[i]->timingCap;
        long bM = pepas[i]->multiplicador;

        if (pepas[selector]->dividiendo == 0)
          bT = aT / aM;
        else
          bT = aT * aM;

        if (pepas[i]->dividiendo == 0)
          bT = bT * bM;
        else
          bT = bT / bM;

        bT %= (long long)capacidad;      // normalizar sin desbordar (reemplaza el while)
        if (bT < 0) bT += capacidad;

        pepas[i]->timingCap = (long)bT;
        pepas[i]->velocidad = pepas[selector]->velocidad;
      }
    }
  }
  else if (sc == SC_ESC)
  {
    if (buscar(SC_LCTRL) != -1 && shift == 1)
      factoryReset(); // Ctrl+Shift+Esc: reset de fabrica (todo a valores de encendido)
    else if (shift == 0)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->reiniciarCabezal(); // Esc: reiniciar cabezal en todas las pepas
    // Shift+Esc (sin Ctrl): tap tempo en desarrollo, desconectado (ver insertarTap en Pepas.ino)
  }
  else if (sc == SC_ENTER)
  {
    if (buscar(SC_LCTRL) != -1 && shift == 1)
      guardarEstado(); // Ctrl+Shift+Enter: guardar el estado actual en EEPROM
  }
  else if (sc == SC_LSHIFT)
  {
    if (buscar(SC_LCTRL) >= 0)
      for (uint8_t i = 0; i < cantPepas; i++)
        if (i != selector)
          pepas[i]->controlarCTRL(1); // checkear si esta presionado L CTRL

    if (buscar(SC_LALT) >= 0)
      for (uint8_t i = 0; i < cantPepas; i++)
        if (i != selector)
          pepas[i]->controlarALT(1); // checkear si esta presionado L ALT

    // agregar a todas las pepas las notas que siguen fisicamente presionadas (se itera el scancode guardado)
    if (cantPresionadas > 0)
      for (uint8_t i = 0; i < cantPresionadas; i++)
        if (K2Midi(presionadas[i]) > 0)
          for (uint8_t j = 0; j < cantPepas; j++)
            pepas[j]->agregar(presionadas[i]);

    shift = 1;
  }
  else if (sc == SC_TAB) // cambiar selector
  {
    selector++;
    if (selector > cantPepas - 1) selector = 0;
    actualizarLEDSelector();
  }
  else if (K2Num(sc) > -1) // teclado numerico
  {
    if (pepas[selector]->control > 0)
    {
      if (shift == 1)
        for (uint8_t i = 0; i < cantPepas; i++)
          pepas[i]->numero = concatenar(pepas[i]->numero, K2Num(sc));
      else
        pepas[selector]->numero = concatenar(pepas[selector]->numero, K2Num(sc));
    }

    if (setTempo == 1) tempo = concatenar(tempo, K2Num(sc));
  }
  else if (sc == SC_KP_MULT) // multiplicar velocidad
  {
    BROADCAST(controlarMult(1));
  }

  // estos quedan como if independientes: comparten scancode con teclas no extendidas (E0)
  if (sc == SC_KP_DIV && ev.extendida) // dividir velocidad
  {
    BROADCAST(controlarDiv(1));
  }
  if (sc == SC_UP && ev.extendida)   // flecha arriba: subir octava (broadcast con shift)
  {
    BROADCAST(subirOctava());
  }
  if (sc == SC_DOWN && ev.extendida) // flecha abajo: bajar octava (broadcast con shift)
  {
    BROADCAST(bajarOctava());
  }
  if (sc == SC_DEL && ev.extendida)  // Supr: soft reset / panico con Ctrl+Alt (mimetiza Ctrl+Alt+Del)
  {
    if (buscar(SC_LCTRL) != -1 && buscar(SC_LALT) != -1)
      softReset();
  }
}

void manejarSoltar(TeclaEvento &ev)
{
  uint8_t sc = ev.scancode;

  // Sacar de presionadas[] SOLO si la tecla estaba registrada. Antes se decrementaba
  // cantPresionadas siempre, lo que con un break huerfano lo mandaba negativo y la
  // siguiente presion escribia en presionadas[-1] (corrupcion de memoria).
  int8_t indice = buscar(sc);
  if (indice > -1)
  {
    for (uint8_t i = indice; i < cantPresionadas - 1; i++) // i+1 nunca sale del array
      presionadas[i] = presionadas[i + 1];
    cantPresionadas--;
    presionadas[cantPresionadas] = 0; // limpiar el slot que quedo libre
    recontarNotas();
  }

  if (K2Midi(sc) > 0 && !ev.extendida) // soltando una nota
  {
    BROADCAST(quitar(sc));
  }
  else if (sc == SC_LCTRL)
  {
    controlarVelocidad = 0;
    BROADCAST(controlarCTRL(0));
  }
  else if (sc == SC_LALT)
  {
    BROADCAST(controlarALT(0));
  }
  else if (sc == SC_F1)
  {
    BROADCAST(controlarF1(0));
  }
  else if (sc == SC_F2)
  {
    setTempo = 0;
    if (tempo > 0)
    {
      float bpms = float(tempo * 1.0004) / 60000.0; // por alguna razon hay que multiplicar el tempo por ~1.00045
      velocidadGeneral = capacidad * bpms;
      tempo = 0;
    }
  }
  else if (sc == SC_KP_MULT)
  {
    BROADCAST(controlarMult(0));
  }

  if (sc == SC_KP_DIV && ev.extendida) // soltando KP /
  {
    BROADCAST(controlarDiv(0));
  }
  else if (sc == SC_LSHIFT)
  {
    if (buscar(SC_LCTRL) >= 0)
      for (uint8_t i = 0; i < cantPepas; i++)
        if (i != selector) pepas[i]->controlarCTRL(0);

    if (buscar(SC_LALT) >= 0)
      for (uint8_t i = 0; i < cantPepas; i++)
        if (i != selector) pepas[i]->controlarALT(0);

    // quitar de todas las pepas las notas que siguen fisicamente presionadas
    if (cantPresionadas > 0)
      for (uint8_t i = 0; i < cantPresionadas; i++)
        if (K2Midi(presionadas[i]) > 0)
          for (uint8_t j = 0; j < cantPepas; j++)
            pepas[j]->quitar(presionadas[i]);

    for (uint8_t i = 0; i < cantTaps; i++) tap[i] = 0;
    shift = 0;
  }
}

void eventoTeclado()
{
  // Drenar el buffer entero en cada pasada: el decodificador resuelve los prefijos
  // E0/F0 inline, sin la pausa de ~512ms que antes dejaba desbordar el ring buffer.
  TeclaEvento ev;
  while (ps2NextKey(ev))
  {
    if (ev.soltando) manejarSoltar(ev);
    else manejarPresionar(ev);
  }

  // Si el teclado se reinicio (replug/brownout), re-sincronizar sus LEDs al canal actual.
  // NO se toca el estado del secuenciador: la musica sigue exactamente como estaba.
  if (ps2HuboReset())
    actualizarLEDSelector();
}
