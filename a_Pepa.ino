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
      futureMillisT = escalaSize = 0;
      mantener = secuenciar = cabezal = timingCap = timingCapPrev = disparar = control = dividiendo = 0;
      secuenciaCantTemp = 8;
      notasSec = 8;
      octava = 3;
      probabilidad = 1024;
      mutacion = 0;
      clockCount = 0;
      sqrEnvCycle = capacidad * .5;
      velocidad  = velocidadGeneral;
      numero = 0;
      multiplicador = multiplicadorTemporal = 1;
    }
    
    uint8_t escalaSize, mantener, control, dividiendo, octava, modoSqrEnv, id, disparar;
    long multiplicador, velocidad, timingCap;
    int probabilidad, mutacion, clockCount;
    uint8_t numero;

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
              triggerLED(id, 1);
              triggerLED(id, 0);
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
                triggerLED(id,1);
                triggerLED(id,0);
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
              triggerLED(id, 1);
              triggerLED(id, 0);
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
              triggerLED(id,1);
              triggerLED(id,0);
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
        {
          if (mantener == 1)
          {
            disparar = 0;
            if (clockSwitch == false)
              reiniciarCabezal();
          }
          else
          {
            if (clockSwitch == false)
              reiniciarCabezal();
            else
              disparar = 0;
          } 
        }
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
          multiplicadorTemporal = numero;
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
          multiplicadorTemporal = numero;
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
