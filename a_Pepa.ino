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
      mantener = secuenciar = cabezal = timingCap = timingCapPrev = disparar = control = dividiendo = 0;
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
      resetearEscala();
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
        
        if (secuenciar == 0) // secuencia aleatoria
        {
          if (random(1024) <= probabilidad)
          {
            if (modoSqrEnv == 0)
            {
              uint8_t notaOut = escala[random(0, escalaSize)]; 
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
              uint8_t notaOut = escala[random(0, escalaSize)]; 
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
        else // secuencia fija
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
    
    void secuenciarSwitch()
    {
      if (secuenciar == 0)
      {
        resetearSecuencia();
        secuenciar = 1;
      }
      else 
        secuenciar = 0;
    }
    
    void resetearSecuencia()
    {
      secuenciaCant = secuenciaCantTemp;
        
      for (uint8_t i = 0; i < secuenciaCant; i++)
      {
        uint8_t gate = (random(1024) <= probabilidad) ? 1 : 0;
        escribirPaso(i, random(notasSec), gate, random(255));
      }
    }

    void reiniciarCabezal(){
      timingCap = 0L;
      cabezal = 0;
      disparar = 1;
      clockCount = 0;
      multiplicador = multiplicadorTemporal;
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
      EEPROM.update(addr++, secuenciar);
      EEPROM.update(addr++, escalaSize);
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
      secuenciar        = EEPROM.read(addr++);
      escalaSize        = EEPROM.read(addr++);
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
      if (probabilidad < 1 || probabilidad > 1024) probabilidad = 1024;
      if (mutacion < 0 || mutacion > 1024) mutacion = 0;
      return addr;
    }

    private:
    uint8_t puertoT, puertoCV, puertoG, puertoCV2, secuenciar, cabezal, escala[16], secuenciaCant, secuenciaCantTemp, notasSec; // tratar de usar una variable para el tamaño de escala[]

    // Cada paso de la secuencia ocupa 2 bytes (antes eran 3) para ahorrar RAM:
    //   secuencia[paso][0] -> bit 7 = gate (1 = suena, 0 = silencio), bits 0-6 = indice de nota
    //   secuencia[paso][1] -> valor de CV aleatorio (0-255)
    uint8_t secuencia[64][2];

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
};

Pepa *pepas[cantPepas];
