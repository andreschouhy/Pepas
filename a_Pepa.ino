class Pepa
{
  public:
    Pepa(uint8_t _puertoCV, uint8_t _puertoT, uint8_t _puertoCV2, uint8_t _puertoG, int _pote, uint8_t _modoSqrEnv, uint8_t _id)
    {
      id = _id;
      modoSqrEnv = _modoSqrEnv;
      puertoCV = _puertoCV;
      puertoT = _puertoT;
      puertoCV2 = _puertoCV2;
      puertoG = _puertoG;
      futureMillisT = escalaSize = 0;
//      escalaSizePreset1 = escalaSizePreset2 = escalaSizePreset3 = escalaSizePreset4 = 0;
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
    
    uint8_t escalaSize, tamanoEscalaLimite, mantener, control, dividiendo, octava, modoSqrEnv, id, disparar;
//    uint8_t escalaSizePreset1, escalaSizePreset2, escalaSizePreset3, escalaSizePreset4;
    long multiplicador, velocidad, timingCap;
    int probabilidad, mutacion, clockCount;
    uint8_t numero;

    void triggerLoopCheck()
    {
      triggerLoop();
    }
    
    void actualizar()
    {
      velocidad = velocidadGeneral * ajusteFinoVel; // reemplazando velocidad individual por general
      
      if(control == 1)
      {/*( // condicional comentado para reemplazar con velocidad general
        velocidad = (pote * precision) + poteSnapshot;
        
        if(velocidad > (1024L * precision)) 
        {
          velocidad = 1024L * precision;
          poteSnapshot = velocidad - (pote * precision);
        }
        else if(velocidad <= (1L * precision)) 
        {
          velocidad = 1L * precision;
          poteSnapshot = velocidad - (pote * precision);
        }*/
      }
      else if (control == 2)
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
          if (secuencia[cabezal][1] == 1)
          {
            if (modoSqrEnv == 0)
            {
              uint8_t notaOut = escala[secuencia[cabezal][0] % escalaSize]; 
              notaOut += (12*(octava-1));
              notaOut = min(notaOut, 81);
              analogWrite(puertoCV, map(notaOut, 21, 81, 0, 255));
              trigger(10);
              triggerLED(id, 1);
              triggerLED(id, 0);
              analogWrite(puertoCV2, secuencia[cabezal][2]);
              digitalWrite(puertoG, HIGH);
            }
            else if (modoSqrEnv == 1) 
            {
              uint8_t notaOut = escala[secuencia[cabezal][0] % escalaSize]; 
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
        if (escalaSize > 0 && mantener == 1 && notasPresionadas == 1) 
          resetearEscala(); //resetear escala cuando esta en mantener
        
        escalaSize++;
        escala[escalaSize - 1] = K2Midi(tecla);
      }
      
      if (escalaSize == 1 && mantener != 1) 
      {
        reiniciarCabezal();
      }
    }
    
    void quitar(uint8_t tecla)
    {
      if (mantener == 0)
      {
        uint8_t nota = K2Midi(tecla);
        int8_t index = buscarNota(nota);
        
        while (index > -1)
        {
          for (int8_t i = index; i < escalaSize; i++) 
          {
            escala[i] = escala[i+1];
          }
          
          index = buscarNota(nota); //en caso de que un valor este duplicado, chequear de nuevo
        }
        
        escala[escalaSize] = 0;
        escalaSize--;
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
        secuencia[i][0] = random(notasSec);
        secuencia[i][2] = random(255);
        
        if (random(1024) <= probabilidad) 
          secuencia[i][1] = 1;
        else 
          secuencia[i][1] = 0;
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
        secuencia[mutado][0] = random(notasSec);
        secuencia[mutado][2] = random(255);
        
        if (random(1024) <= probabilidad) 
          secuencia[mutado][1] = 1;
        else 
          secuencia[mutado][1] = 0;
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

    /*void preset(uint8_t presetID)
    {
      if(control == 1) // guardar preset
      {
        switch (presetID){
          case 1:
            escalaSizePreset1 = escalaSize;
            for (uint8_t i = 0; i < 16; i++) escalaPreset1[i] = escala[i];
            break;
          case 2:
            escalaSizePreset2 = escalaSize;
            for (uint8_t i = 0; i < 16; i++) escalaPreset2[i] = escala[i];
            break;
          case 3:
            escalaSizePreset3 = escalaSize;
            for (uint8_t i = 0; i < 16; i++) escalaPreset3[i] = escala[i];
            break;
          case 4:
            escalaSizePreset4 = escalaSize;
            for (uint8_t i = 0; i < 16; i++) escalaPreset4[i] = escala[i];
            break;
        }
      }
      else if (control == 0) //cargarPreset
      {
        switch (presetID){
          case 1:
            escalaSize = escalaSizePreset1;
            for (uint8_t i = 0; i < 16; i++) escala[i] = escalaPreset1[i];
            break;
          case 2:
            escalaSize = escalaSizePreset2;
            for (uint8_t i = 0; i < 16; i++) escala[i] = escalaPreset2[i];
            break;
          case 3:
            escalaSize = escalaSizePreset3;
            for (uint8_t i = 0; i < 16; i++) escala[i] = escalaPreset3[i];
            break;
          case 4:
            escalaSize = escalaSizePreset4;
            for (uint8_t i = 0; i < 16; i++) escala[i] = escalaPreset4[i];
            break;
        }
      }
    }*/
    
    private:
    uint8_t puertoT, puertoCV, puertoG, puertoCV2, secuenciar, cabezal, secuencia[64][3], escala[16], secuenciaCant, secuenciaCantTemp, notasSec; // tratar de usar una variable para el tamaño de escala[]
//    uint8_t escalaPreset1[16], escalaPreset2[16], escalaPreset3[16], escalaPreset4[16]; // tratar de usar una variable para el tamaño de escalaPreset#[] // y ver por qué no se pueden agregar 8 presets, quizas sobrepasa el limite de memoria
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
