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
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->agregar(sc);
    else
      pepas[selector]->agregar(sc);
  }
  else if (sc == SC_CAPS) // mantener
  {
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->mantenerSwitch();
    else
      pepas[selector]->mantenerSwitch();
  }
  else if (sc == SC_SPACE) // secuenciar
  {
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->secuenciarSwitch();
    else
      pepas[selector]->secuenciarSwitch();
  }
  else if (sc == SC_BKSP) // resetear secuencia
  {
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->resetearSecuencia();
    else
      pepas[selector]->resetearSecuencia();
  }
  else if (sc == SC_LCTRL && pepas[selector]->control == 0)
  {
    controlarVelocidad = 1;
    poteSnapshotGral = velocidadGeneral - ((long)pote * precision);

    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->controlarCTRL(1);
    else
      pepas[selector]->controlarCTRL(1);
  }
  else if (sc == SC_LALT && pepas[selector]->control == 0)
  {
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->controlarALT(1);
    else
      pepas[selector]->controlarALT(1);
  }
  else if (sc == SC_F1 && pepas[selector]->control == 0)
  {
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->controlarF1(1);
    else
      pepas[selector]->controlarF1(1);
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
        long aT = pepas[selector]->timingCap;
        long aM = pepas[selector]->multiplicador;
        long bT = pepas[i]->timingCap;
        long bM = pepas[i]->multiplicador;

        if (pepas[selector]->dividiendo == 0)
          bT = aT / aM;
        else
          bT = aT * aM;

        if (pepas[i]->dividiendo == 0)
          bT = bT * bM;
        else
          bT = bT / bM;

        while (bT > capacidad)
          bT -= capacidad;

        pepas[i]->timingCap = bT;
        pepas[i]->velocidad = pepas[selector]->velocidad;
      }
    }
  }
  else if (sc == SC_ESC) // reiniciar cabezal en todas las pepas
  {
    if (shift == 0)
    {
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->reiniciarCabezal();
    }
    // shift+ESC: tap tempo en desarrollo, desconectado por ahora (ver insertarTap en Pepas.ino)
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

    // indicar selector en los LEDs en binario (solo se usan los 2 de la derecha,
    // el de la izquierda queda libre como indicador de trigger)
    attachInterrupt(CLOCK_PIN_INT, ps2int_write, FALLING);
    enviar(0xED);
    uint8_t selectorModificado = 0;
    if (selector == 0) { enviar(selectorModificado); }
    if (selector == 1) { bitWrite(selectorModificado, 0, 1); enviar(selectorModificado); }
    if (selector == 2) { bitWrite(selectorModificado, 2, 1); enviar(selectorModificado); }
    if (selector == 3) { bitWrite(selectorModificado, 0, 1); bitWrite(selectorModificado, 2, 1); enviar(selectorModificado); }
    estadosLED = selectorModificado;
    attachInterrupt(CLOCK_PIN_INT, ps2int_read, FALLING);
    releaseClock();
    inhibiting = false;
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
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->controlarMult(1);
    else
      pepas[selector]->controlarMult(1);
  }

  // estos quedan como if independientes: comparten scancode con teclas no extendidas (E0)
  if (sc == SC_KP_DIV && ev.extendida) // dividir velocidad
  {
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->controlarDiv(1);
    else
      pepas[selector]->controlarDiv(1);
  }
  if (sc == SC_UP && ev.extendida)   // flecha arriba: subir octava
  {
    pepas[selector]->subirOctava();
  }
  if (sc == SC_DOWN && ev.extendida) // flecha abajo: bajar octava
  {
    pepas[selector]->bajarOctava();
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
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->quitar(sc);
    else
      pepas[selector]->quitar(sc);
  }
  else if (sc == SC_LCTRL)
  {
    controlarVelocidad = 0;
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->controlarCTRL(0);
    else
      pepas[selector]->controlarCTRL(0);
  }
  else if (sc == SC_LALT)
  {
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->controlarALT(0);
    else
      pepas[selector]->controlarALT(0);
  }
  else if (sc == SC_F1)
  {
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->controlarF1(0);
    else
      pepas[selector]->controlarF1(0);
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
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->controlarMult(0);
    else
      pepas[selector]->controlarMult(0);
  }

  if (sc == SC_KP_DIV && ev.extendida) // soltando KP /
  {
    if (shift == 1)
      for (uint8_t i = 0; i < cantPepas; i++)
        pepas[i]->controlarDiv(0);
    else
      pepas[selector]->controlarDiv(0);
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
}
