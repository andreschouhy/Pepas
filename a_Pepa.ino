class Pepa
{
  public:
    Pepa(uint8_t _puertoCV, uint8_t _puertoT, uint8_t _puertoCV2, uint8_t _puertoG, uint8_t _modoSqrEnv, uint8_t _id)
    {
      id = _id;
      modoSqrEnv = _modoSqrEnv;
      puertoCV = _puertoCV;
      puertoT = _puertoT;
      puertoCV2 = _puertoCV2;
      puertoG = _puertoG;
      reset(); // el resto de los campos a sus valores de encendido (ver reset())
    }

    // Estado de encendido de la voz. Lo usa el constructor y el factory reset (Ctrl+Shift+Esc),
    // asi ambos quedan identicos. No toca puertos, modoSqrEnv ni id (config de hardware).
    // Inicializa TODO campo: new no limpia la memoria, no dejar nada en basura.
    void reset()
    {
      futureMillisT = escalaSize = 0;
      mantener = cabezal = timingCap = timingCapPrev = disparar = control = dividiendo = 0;
      secuenciaCantTemp = 8;
      secuenciaCant = secuenciaCantTemp;
      notasSec = 8;
      octava = 3;
      probabilidad = 1024;
      mutacion = 0;
      clockCount = 0;
      sqrEnvCycle = capacidad * .5;
      velocidad  = velocidadGeneral;
      numero = 0;
      poteSnapshot = 0;
      multiplicador = multiplicadorTemporal = 1;
      arpModo = 0; arpNota = 0; arpDir = 1; arpPaso = 0;
      limpiarArpegio();
      resetearEscala();
      resetearSecuencia(); // arpModo 0 (aleatorio) corre sobre el motor de secuencia desde el
                           // arranque: mutacion=0 -> patron fijo; subir mutacion -> totalmente aleatorio
      digitalWrite(puertoG, LOW);
      analogWrite(puertoCV2, 0);
    }

    // Soft reset / panico (Ctrl+Alt+Supr): apagar salidas y soltar notas trabadas de la escala,
    // sin tocar secuencia, tempo, octava ni parametros.
    void silenciar()
    {
      resetearEscala();
      disparar = 0;
      control = 0;
      digitalWrite(puertoG, LOW);
      analogWrite(puertoCV2, 0);
    }
    
    uint8_t escalaSize, mantener, control, dividiendo, octava, modoSqrEnv, id, disparar;
    long multiplicador, velocidad, timingCap;
    int probabilidad, mutacion, clockCount;
    uint8_t numero;
    uint8_t arpModo, arpNota; // modo del paso (flechas izq/der): 0=aleatorio (motor de secuencia,
                              // mutacion morfea fijo<->random), 1=up, 2=down, 3=pingpong. arpNota = ultima nota del arpegio
    int8_t  arpDir;           // pingpong: sentido actual del recorrido (+1 sube, -1 baja)
    uint8_t arpPaso;          // posicion dentro del ciclo del arpegio; indexa los overrides (arpOv)

    // Accesor publico de triggerLoop() (que es private): loop() lo llama en cada iteracion
    // para cerrar el pulso de trigger cuando expira su duracion. La logica vive en triggerLoop
    // (private) porque tambien se dispara internamente al lanzar una nota; este wrapper solo
    // la expone al exterior sin abrir toda la implementacion.
    void triggerLoopCheck()
    {
      triggerLoop();
    }
    
    void actualizar()
    {
      velocidad = velocidadGeneral; // la velocidad de cada pepa sigue a la general

      // control == 1 (velocidad por pote) ya no se maneja aca: la velocidad sigue a velocidadGeneral
      if (control == 2)
      {
        probabilidad = pote + poteSnapshot;
        
        if(probabilidad > 1024) 
        {
          probabilidad = 1024;
          poteSnapshot = probabilidad - pote;
        }
        else if (probabilidad <= 1) 
        {
          probabilidad = 1;
          poteSnapshot = probabilidad - pote;
        }
      }
      else if (control == 5)
      {
        mutacion = pote + poteSnapshot;
        
        if (mutacion > 1024) 
        {
          mutacion = 1024;
          poteSnapshot = mutacion - pote;
        }
        else if (mutacion <= 0) 
        {
          mutacion = 0;
          poteSnapshot = mutacion - pote;
        }
      }
      
      // timingCap simula un capacitor, permite modificar la frecuencia entre intervalos
      timingCapPrev = long(timingCap);
      
      if (deltaMillis > 0) 
      {
        for (unsigned long d = 1; d <= deltaMillis; d++)
        {
          if (dividiendo == 0) 
            timingCap += (velocidad * multiplicador);
          else if (dividiendo == 1) 
            timingCap += (velocidad / multiplicador);
            
          timingCap = timingCap % capacidad;
        }
      }

      if (clockSwitch == false)
      {
        if (timingCap < timingCapPrev) 
        {
          disparar = 1;
          multiplicador = multiplicadorTemporal;
        }
      }
      else
      {
        // avanzar conteo de pulsos de clock, destinado a usar con los 24 pulsos por cuarto de nota de las especificaciones MIDI
        if (dividiendo == 0) 
          clockCount = (clockCount + 1) % (24 / min(multiplicador, 12));
        if (dividiendo == 1) 
          clockCount = (clockCount + 1) % (24 * min(multiplicador, 24));
        
        if (clockCount == 0) 
        {
          disparar = 1;
          multiplicador = multiplicadorTemporal;
        }
      }
      
      if (escalaSize > 0 && disparar == 1)
      {
        disparar = 0;
        
        if (arpModo != 0) // arpegio up/down/pingpong (arpModo 1/2/3)
        {
          // Avanzar el recorrido limpio SIEMPRE, aunque la probabilidad lo silencie: asi bajar la
          // probabilidad hace un arpegio con huecos en vez de solo estirarlo.
          uint8_t limpio = proximaNota();

          // Mutacion del arpegio: sobre el recorrido limpio, aplica overrides fijos por paso. La
          // mutacion (F1) va reescribiendo pasos al azar; el patron persiste y solo backspace lo
          // limpia (mutacion=0 lo congela). Ver mutarArpegio / arpOv.
          uint8_t periodo = arpPeriodo();
          if (arpPaso >= periodo) arpPaso = 0; // el acorde pudo achicarse desde el ultimo paso
          mutarArpegio(periodo);
          int8_t ov = arpOv[arpPaso];
          uint8_t notaOut = (ov >= 0) ? escala[ov % escalaSize] : limpio;
          arpPaso++;
          if (arpPaso >= periodo) arpPaso = 0;

          if (random(1024) <= probabilidad)
          {
            if (modoSqrEnv == 0)
            {
              notaOut += (12*(octava-1));
              notaOut = min(notaOut, 81);
              analogWrite(puertoCV, map(notaOut, 21, 81, 0, 255));
              trigger(10);
              triggerLED(id);
              analogWrite(puertoCV2, random(255));
              digitalWrite(puertoG, HIGH);
            }
            else if (modoSqrEnv == 1)
            {
              sqrEnvCycle = capacidad * (map(notaOut, 21, 49, 0, 100)/100.0);
              
              if (sqrEnvCycle > (capacidad * (map(21, 21, 49, 0, 100)/100.0))) // este condicional es para que no dispare con notaOut = 21, no deberia de todas formas
              { 
                digitalWrite(puertoG, HIGH);
                triggerLED(id);
              }
              
              if (clockSwitch == true) 
                sqrEnvCycle = map(notaOut, 21, 49, 0, 24);
            }
          }
          else 
          {
            digitalWrite(puertoG, LOW);
          }
        }
        else // arpModo 0 = "aleatorio": motor de secuencia. mutacion (F1) morfea de patron fijo
             // (mutacion=0, default) a totalmente aleatorio (mutacion=max). Gate/nota/CV por paso.
        {
          if (pasoGate(cabezal) == 1)
          {
            if (modoSqrEnv == 0)
            {
              uint8_t notaOut = escala[pasoNota(cabezal) % escalaSize];
              notaOut += (12*(octava-1));
              notaOut = min(notaOut, 81);
              analogWrite(puertoCV, map(notaOut, 21, 81, 0, 255));
              trigger(10);
              triggerLED(id);
              analogWrite(puertoCV2, pasoCV(cabezal));
              digitalWrite(puertoG, HIGH);
            }
            else if (modoSqrEnv == 1)
            {
              uint8_t notaOut = escala[pasoNota(cabezal) % escalaSize];
              sqrEnvCycle = capacidad * (map(notaOut, 21, 49, 0, 100)/100.0);
              
              if (clockSwitch == true) 
                sqrEnvCycle = map(notaOut, 21, 49, 0, 24);
              
              digitalWrite(puertoG, HIGH);
              triggerLED(id);
            }
          }
          else digitalWrite(puertoG, LOW);

          mutarSecuencia(); // mutar sueciencia en cada paso de la secuencia
          
          cabezal++;
          if (cabezal >= secuenciaCant)
            cabezal = 0;
        }
      }

      if (modoSqrEnv == 1)
      {
        if (clockSwitch == true)
        {
          if (clockCount > sqrEnvCycle) 
            digitalWrite(puertoG, LOW);
        }
        else 
        {
          if (timingCap > sqrEnvCycle) 
            digitalWrite(puertoG, LOW);
        }
      }
      
      if (escalaSize == 0) 
      {
        analogWrite(puertoCV2, 0);
        digitalWrite(puertoG, LOW);
      }
      
      if (modoSqrEnv == 0) 
        triggerLoop();
    }
    
    void trigger(int duracion)
    {
      digitalWrite(puertoT, LOW);
      digitalWrite(puertoT, HIGH);
      futureMillisT = currentMillis + duracion;
    }
    
    void agregar(uint8_t tecla)
    {
      if (K2Midi(tecla) > 0)
      {
        if (mantener == 1)
        {
          if (escalaSize > 0 && notasPresionadas == 1) 
            resetearEscala(); //resetear escala cuando esta en mantener
        }
        
        if (escalaSize >= 16) return; // escala[] llena (16 notas), no agregar mas para no pasarse del array

        escalaSize++;
        escala[escalaSize - 1] = K2Midi(tecla);

        if (escalaSize == 1)
          disparar = 0; // NO reiniciar el timer: la nueva escala entra en fase con el timingCap
                        // que ya corre y suena en el proximo wrap. Asi se mantiene el sync entre
                        // canales (antes reiniciarCabezal() ponia timingCap=0 en cada escala nueva
                        // y desincronizaba). disparar=0 evita un disparo inmediato fuera de grilla.
      }
    }
    
    void quitar(uint8_t tecla)
    {
      if (mantener == 0)
      {
        uint8_t nota = K2Midi(tecla);
        int8_t index = buscarNota(nota);

        // decrementar SOLO si la nota estaba en esta escala. Antes se decrementaba siempre,
        // lo que con escalaSize==0 desbordaba el uint8_t a 255 (probable causa del cuelgue al soltar).
        while (index > -1 && escalaSize > 0)
        {
          for (int8_t i = index; i < escalaSize - 1; i++) // i+1 nunca sale del array
          {
            escala[i] = escala[i+1];
          }

          escalaSize--;
          escala[escalaSize] = 0; // limpiar el slot que quedo libre

          index = buscarNota(nota); //en caso de que un valor este duplicado, chequear de nuevo
        }
      }
    }
    
    void resetearEscala()
    {
      for (uint8_t i = 0; i < escalaSize; i++) 
      {
        escala[i] = 0;
      }
      
      escalaSize = 0;
    }
    
    void mantenerSwitch()
    {
      if (mantener == 0) 
        mantener = 1;
      else 
      {
        mantener = 0;
        resetearEscala();
      }
    }
    
    void resetearSecuencia()
    {
      secuenciaCant = secuenciaCantTemp;
        
      for (uint8_t i = 0; i < secuenciaCant; i++)
      {
        uint8_t gate = (random(1024) <= probabilidad) ? 1 : 0;
        escribirPaso(i, random(notasSec), gate, random(255));
      }
      limpiarArpegio(); // backspace (resetearSecuencia) tambien vuelve el arpegio a limpio
    }

    void reiniciarCabezal(){
      timingCap = 0L;
      cabezal = 0;
      arpNota = 0; arpDir = 1; arpPaso = 0; // reiniciar el recorrido del arpegio (arranca desde abajo); overrides intactos
      disparar = 1;
      clockCount = 0;
      multiplicador = multiplicadorTemporal;
    }

    // Reinicia solo la posicion del paso (cabezal de secuencia + recorrido del arpegio), sin tocar
    // el timer (timingCap). Se usa al sincronizar (`) para que todo arranque junto manteniendo la fase.
    void reiniciarPaso(){
      cabezal = 0;
      arpNota = 0; arpDir = 1; arpPaso = 0; // arpegios: reiniciar el recorrido (sync); overrides intactos
    }
    
    void mutarSecuencia()
    {
      if (random(1024) < mutacion)
      {
        uint8_t mutado = random(secuenciaCant);
        uint8_t gate = (random(1024) <= probabilidad) ? 1 : 0;
        escribirPaso(mutado, random(notasSec), gate, random(255));
      }
    }

    // ---- Arpegio con mutacion (analogo a mutarSecuencia para el motor de secuencia) ----
    // Periodo del ciclo del arpegio: up/down = escalaSize; pingpong = 2*escalaSize-2. Los overrides
    // se indexan contra este periodo para que "el paso k" signifique lo mismo en cada vuelta.
    uint8_t arpPeriodo()
    {
      if (escalaSize <= 1) return 1;
      if (arpModo == 3) return (uint8_t)(2 * escalaSize - 2); // pingpong
      return escalaSize;                                      // up/down
    }

    // Con prob. mutacion reescribe UN paso al azar del arpegio a una nota aleatoria de la escala
    // (mismo dado que mutarSecuencia). El override queda fijo -> loop consistente; mutacion=0 no
    // reescribe nada -> congela el patron acumulado. Guarda el indice de escala y se lee con
    // %escalaSize, asi el patron se arrastra (remapea) al cambiar de acorde.
    void mutarArpegio(uint8_t periodo)
    {
      if (escalaSize <= 1) return;
      if (random(1024) < mutacion)
        arpOv[random(periodo)] = random(escalaSize);
    }

    // Vuelve el arpegio a limpio (borra todos los overrides). Unico reset total del patron: backspace.
    void limpiarArpegio()
    {
      for (uint8_t i = 0; i < 30; i++) arpOv[i] = -1;
      arpPaso = 0;
    }
    
    void controlarCTRL(uint8_t estado)
    {
      if (estado == 1)
      {
        control = 1;
        poteSnapshot = velocidad - ((long)pote * precision);
        numero = 0;
      }
      else if (estado == 0) 
      {
        control = 0;
        if (numero != 0) 
          secuenciaCantTemp = numero;
        if (secuenciaCantTemp > 64) 
          secuenciaCantTemp = 64; // limite para la cantidad de pasos de la secuencia
        numero = 0;
      }
    }
    
    void controlarALT(uint8_t estado)
    {
      if (estado == 1)
      {
        control = 2;
        poteSnapshot = probabilidad - pote;
        numero = 0;
      }
      else if (estado == 0) 
      {
        control = 0;
        if (numero != 0) 
          notasSec = numero;
        if (notasSec > 16) 
          notasSec = 16; // limite para la cantidad de notas posibles, dictado por el tamaño de escala[]
        numero = 0;
      }
    }
    
    void controlarF1(uint8_t estado)
    {
      if (estado == 1)
      {
        control = 5;
        poteSnapshot = mutacion - pote;
      }
      else if (estado == 0) 
      {
        control = 0;
      }
    }
    
    void controlarMult(uint8_t estado)
    {
      if (estado == 1)
      {
        control = 3;
        numero = 0;
      }
      else if (estado == 0) 
      {
        control = 0;
        if (numero != 0)
        {
          dividiendo = 0;
          multiplicadorTemporal = min(numero, 32); // cap: numero llega a 255; int8_t se iria a negativo
        }
        numero = 0;
      }
    }
    
    void controlarDiv(uint8_t estado)
    {
      if (estado == 1)
      {
        control = 4;
        numero = 0;
      }
      else if (estado == 0) 
      {
        control = 0;
        if (numero != 0)
        {
          dividiendo = 1;
          multiplicadorTemporal = min(numero, 32); // cap: numero llega a 255; int8_t se iria a negativo
        }
        numero = 0;
      }
    }
    
    void subirOctava()
    {
      if (octava < 4) 
        octava++;
      else 
        octava = 4;
    }
    
    void bajarOctava()
    {
      if (octava > 1)
        octava--;
      else
        octava = 1;
    }

    // Arpegio: flecha der/izq cicla el modo (0=aleatorio, 1=up, 2=down, 3=pingpong). Al cambiar
    // de modo se reinicia el sentido del pingpong; arpNota se deja como esta para que el recorrido
    // continue desde la altura actual en vez de saltar de golpe al piso de la escala.
    void arpSiguiente() { arpModo = (arpModo + 1) % 4; arpDir = 1; }
    void arpAnterior()  { arpModo = (arpModo + 3) % 4; arpDir = 1; } // +3 == -1 mod 4

    // ---- Persistencia (EEPROM) ----
    // Serializan/deserializan la voz a partir de una direccion y devuelven la siguiente.
    // EEPROM.update solo escribe bytes que cambian (cuida el limite de ~100k escrituras).
    int guardarEEPROM(int addr)
    {
      EEPROM.update(addr++, octava);
      EEPROM.update(addr++, secuenciaCantTemp);
      EEPROM.update(addr++, notasSec);
      EEPROM.update(addr++, (uint8_t)multiplicadorTemporal);
      EEPROM.update(addr++, dividiendo);
      EEPROM.update(addr++, mantener);
      EEPROM.update(addr++, escalaSize);
      EEPROM.update(addr++, arpModo);
      EEPROM.put(addr, probabilidad); addr += sizeof(probabilidad);
      EEPROM.put(addr, mutacion);     addr += sizeof(mutacion);
      for (uint8_t i = 0; i < 16; i++) EEPROM.update(addr++, escala[i]);
      for (uint8_t i = 0; i < 64; i++) { EEPROM.update(addr++, secuencia[i][0]); EEPROM.update(addr++, secuencia[i][1]); }
      return addr;
    }

    int cargarEEPROM(int addr)
    {
      octava            = EEPROM.read(addr++);
      secuenciaCantTemp = EEPROM.read(addr++);
      notasSec          = EEPROM.read(addr++);
      multiplicadorTemporal = (int8_t)EEPROM.read(addr++);
      dividiendo        = EEPROM.read(addr++);
      mantener          = EEPROM.read(addr++);
      escalaSize        = EEPROM.read(addr++);
      arpModo           = EEPROM.read(addr++);
      EEPROM.get(addr, probabilidad); addr += sizeof(probabilidad);
      EEPROM.get(addr, mutacion);     addr += sizeof(mutacion);
      for (uint8_t i = 0; i < 16; i++) escala[i] = EEPROM.read(addr++);
      for (uint8_t i = 0; i < 64; i++) { secuencia[i][0] = EEPROM.read(addr++); secuencia[i][1] = EEPROM.read(addr++); }

      // Sanidad: si el EEPROM quedo raro (version vieja, corrupcion), no dejar valores fuera de rango
      if (secuenciaCantTemp == 0 || secuenciaCantTemp > 64) secuenciaCantTemp = 8;
      secuenciaCant = secuenciaCantTemp;
      if (notasSec == 0 || notasSec > 16) notasSec = 8;
      if (octava < 1 || octava > 4) octava = 3;
      if (multiplicadorTemporal < 1 || multiplicadorTemporal > 32) multiplicadorTemporal = 1;
      multiplicador = multiplicadorTemporal;
      if (escalaSize > 16) escalaSize = 16;
      if (arpModo > 3) arpModo = 0;      // EEPROM viejo/corrupto: volver a aleatorio
      arpNota = 0; arpDir = 1;           // estado runtime del arpegio, no se persiste
      limpiarArpegio();                  // los overrides de mutacion son runtime: arrancan limpios
      if (probabilidad < 1 || probabilidad > 1024) probabilidad = 1024;
      if (mutacion < 0 || mutacion > 1024) mutacion = 0;
      return addr;
    }

    private:
    uint8_t puertoT, puertoCV, puertoG, puertoCV2, cabezal, escala[16], secuenciaCant, secuenciaCantTemp, notasSec; // tratar de usar una variable para el tamaño de escala[]

    // Cada paso de la secuencia ocupa 2 bytes (antes eran 3) para ahorrar RAM:
    //   secuencia[paso][0] -> bit 7 = gate (1 = suena, 0 = silencio), bits 0-6 = indice de nota
    //   secuencia[paso][1] -> valor de CV aleatorio (0-255)
    uint8_t secuencia[64][2];

    // Overrides de mutacion del arpegio: por cada paso del ciclo, -1 = nota limpia del recorrido,
    // si no un indice de escala fijo. Analogo al buffer de secuencia: mutarArpegio() reescribe un
    // paso al azar y el patron persiste (loop consistente); mutacion=0 lo congela y solo backspace
    // lo limpia. Tamaño 30 = periodo maximo (pingpong con 16 notas = 2*16-2). Se lee con %escalaSize,
    // asi el patron se remapea al cambiar de acorde ("carry over"). Estado runtime: no va a EEPROM.
    int8_t arpOv[30];

    // Lectura/escritura de un paso (encapsulan el empaquetado de bits)
    uint8_t pasoNota(uint8_t paso) { return secuencia[paso][0] & 0x7F; }
    uint8_t pasoGate(uint8_t paso) { return secuencia[paso][0] >> 7; }
    uint8_t pasoCV(uint8_t paso)   { return secuencia[paso][1]; }
    void escribirPaso(uint8_t paso, uint8_t nota, uint8_t gate, uint8_t cv)
    {
      secuencia[paso][0] = (gate ? 0x80 : 0x00) | (nota & 0x7F);
      secuencia[paso][1] = cv;
    }
    int8_t multiplicadorTemporal;
    long poteSnapshot, timingCapPrev;
    unsigned long futureMillisT, sqrEnvCycle;
    
    void triggerLoop()
    {
      if (futureMillisT <= currentMillis && futureMillisT != 0) 
      {
        futureMillisT = 0;
        digitalWrite(puertoT, LOW);
      }
    }
    
    int8_t buscarNota(uint8_t _nota)
    {
      for(int8_t i = 0; i < escalaSize; i++) if(escala[i] == _nota) return i;
      return -1;
    }

    // ---- Arpegio: seleccion de la proxima nota ----
    // proximaNota() devuelve el valor de escala a tocar segun arpModo y AVANZA el estado del
    // arpegio. Solo se llama con arpModo 1/2/3 (up/down/pingpong): arpModo 0 corre el motor de
    // secuencia en actualizar(), no pasa por aca. Los arpegios recorren la escala por altura
    // (pitch) escaneando la proxima nota mas aguda/grave respecto de arpNota; asi no hace falta
    // ordenar escala[] (romperia la secuencia y el EEPROM) ni RAM extra, y el recorrido se adapta
    // solo si se agregan/quitan notas en vivo. El caso arpModo 0 queda como guardia defensiva.
    uint8_t proximaNota()
    {
      if (arpModo == 0) return escala[random(0, escalaSize)];
      if (escalaSize <= 1) { arpNota = escala[0]; return arpNota; } // una nota: no hay arpegio

      if (arpModo == 1)                              // up
        arpNota = notaArriba(arpNota);
      else if (arpModo == 2)                         // down
        arpNota = notaAbajo(arpNota);
      else                                           // pingpong (arpModo == 3)
      {
        if (arpDir > 0)
        {
          uint8_t sig = notaArriba(arpNota);
          if (sig <= arpNota) { arpDir = -1; sig = notaAbajo(arpNota); } // rebota en el tope
          arpNota = sig;
        }
        else
        {
          uint8_t sig = notaAbajo(arpNota);
          if (sig >= arpNota) { arpDir = 1; sig = notaArriba(arpNota); } // rebota en el piso
          arpNota = sig;
        }
      }
      return arpNota;
    }

    // Nota mas grave de escala estrictamente mayor que v; si no hay (v es la mas aguda), envuelve a
    // la mas grave. notaAbajo es el espejo. Escaneo O(escalaSize), sin ordenar escala[]. Notas
    // repetidas colapsan a un solo paso; con una sola altura distinta el arpegio queda quieto ahi.
    uint8_t notaArriba(uint8_t v)
    {
      uint8_t menor = 255, prox = 255; bool hay = false;
      for (uint8_t i = 0; i < escalaSize; i++)
      {
        uint8_t n = escala[i];
        if (n < menor) menor = n;
        if (n > v && n < prox) { prox = n; hay = true; }
      }
      return hay ? prox : menor;
    }
    uint8_t notaAbajo(uint8_t v)
    {
      uint8_t mayor = 0, prev = 0; bool hay = false;
      for (uint8_t i = 0; i < escalaSize; i++)
      {
        uint8_t n = escala[i];
        if (n > mayor) mayor = n;
        if (n < v && n > prev) { prev = n; hay = true; }
      }
      return hay ? prev : mayor;
    }
};

Pepa *pepas[cantPepas];
