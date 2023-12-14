void eventoTeclado()
{
  if(pausa == 0){
  while(ps2Available()) {
    uint8_t Byte = ps2Read(); // read the next key
    unsigned int durPausa = 8 * multTemp; // 15 * multTemp
    if(Byte == 0xE1 || Byte == 0xFA || Byte == 0xAA || Byte == 0x00) // codigos ignorados
    {
      futurMillis = currentMillis + (durPausa); // sin esta pausa manda fruta
      pausa = 1;
      break;
    }
      
    if(Byte == 0xE0)
    {
      E0Key = 1;
      futurMillis = currentMillis + (durPausa); // sin esta pausa manda fruta
      pausa = 1;
      break;
    }
    
    if(Byte == 0xF0) 
    {
      F0Byte = 1;
      futurMillis = currentMillis + (durPausa); // sin esta pausa manda fruta
      pausa = 1;
      break;
    }
    
    /////////////////////////////////////////////////////// SOLTANDO //////////////////////////////////////////////////////////////////////////
    if(F0Byte == 1)
    {
      int8_t indice = buscar(Byte);
      while (indice > -1)
      {
        for(uint8_t i = indice; i < cantPresionadas; i++) presionadas[i] = presionadas[i+1];
        indice = buscar(Byte); //en caso de que un valor este duplicado, chequear de nuevo, aunque no deberia pasar nunca
      }
      presionadas[cantPresionadas] = 0;
      cantPresionadas--;
      //atualizarLED();
      
      if(K2Midi(Byte) > 0 && E0Key == 0) // soltando una nota
      {
        notasPresionadas--;
        if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->quitar(Byte); 
        else pepas[selector]->quitar(Byte);
      }
      else if(Byte == 0x14)  // soltando L CTRL
      {
        controlarVelocidad = 0;
        if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->controlarCTRL(0); 
        else pepas[selector]->controlarCTRL(0);
      }
      else if(Byte == 0x11) // soltando L ALT
      {
        if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->controlarALT(0); 
        else pepas[selector]->controlarALT(0); 
      }
      else if(Byte == 0x05) // soltando F1
      {
        if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->controlarF1(0); 
        else pepas[selector]->controlarF1(0); 
      }
      else if(Byte == 0x06) // soltando F2
      {
        setTempo = 0;
        if(tempo > 0)
        {
          float bpms = float(tempo * 1.0004) / 60000.0; // tempo / (60 seg / 1000 miliseg) // por alguna razon tengo que multiplicar el tempo por ~1.00045
          velocidadGeneral = capacidad * bpms;
          tempo = 0;
        }
      }
      //else if(Byte == 0x03 || Byte == 0x0B || Byte == 0x83 || Byte == 0x0A) ajusteFinoVel = 1.0; // soltando F5, F6, F7, F8 // reemplazadas las teclas para presets
      else if(Byte == 0x7C) // soltando KP * (para multiplicar velocidad)
      {
        if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->controlarMult(0); 
        else pepas[selector]->controlarMult(0);
      }
      if(Byte == 0x4A && E0Key == 1) // soltando KP / (para dividir velocidad)
      {
        if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->controlarDiv(0); 
        else pepas[selector]->controlarDiv(0);
      }
      else if(Byte == 0x12) // soltando L SHIFT
      {
        if(buscar(0x14) >= 0) for(uint8_t i = 0; i < cantPepas; i++) if(i != selector)pepas[i]->controlarCTRL(0); 
        if(buscar(0x11) >= 0) for(uint8_t i = 0; i < cantPepas; i++) if(i != selector)pepas[i]->controlarALT(0); 
        if(cantPresionadas > 0) for(uint8_t i = 0; i < cantPresionadas; i++) if(K2Midi(i) > 0) for(uint8_t j = 0; j < cantPepas; j++) pepas[j]->quitar(K2Midi(i)); 
        for(uint8_t i = 0; i < cantTaps; i++) tap[i] = 0;
        shift = 0;
      }
      
      F0Byte = 0;
    }
    /////////////////////////////////////////////////////// PRESIONANDO //////////////////////////////////////////////////////////////////////////
    else if(Byte > 0)
    {
      if(buscar(Byte) == -1)
      {
        cantPresionadas++;
        presionadas[cantPresionadas - 1] = Byte;
        //atualizarLED();
        
        if(K2Midi(Byte) > 0 && E0Key == 0)  // presionando una nota
        {
          notasPresionadas++;
          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->agregar(Byte); 
          else pepas[selector]->agregar(Byte);
        }
        else if(Byte == 0x58) // presionando capslock
        {
          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->mantenerSwitch(); 
          else pepas[selector]->mantenerSwitch(); 
        }
        else if(Byte == 0x29) // presionando espacio
        {
          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->secuenciarSwitch(); 
          else pepas[selector]->secuenciarSwitch();
        }
        else if(Byte == 0x66) // presionando backspace
        {
          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->resetearSecuencia(); 
          else pepas[selector]->resetearSecuencia();
        }
        else if(Byte == 0x14 && pepas[selector]->control == 0) // presionando L CTRL
        {
          controlarVelocidad = 1;
          poteSnapshotGral = velocidadGeneral - ((long)pote * precision);
          
          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->controlarCTRL(1); 
          else pepas[selector]->controlarCTRL(1);
        }
        else if(Byte == 0x11 && pepas[selector]->control == 0) // presionando L ALT
        {
          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->controlarALT(1); 
          else pepas[selector]->controlarALT(1);
        }
        else if(Byte == 0x05 && pepas[selector]->control == 0) // presionando F1
        {
          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->controlarF1(1); 
          else pepas[selector]->controlarF1(1);
        }
        else if(Byte == 0x06) // presionando F2
        {
          setTempo = 1;
        }
        else if(Byte == 0x03) // presionando F5 (preset de escala 1)
        {
          //ajusteFinoVel = 0.95; // reemplazado por presets, se podría ubicar en otra tecla
//          pepas[selector]->preset(1);
//          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->preset(1); 
        }
        else if(Byte == 0x0B) // presionando F6 (preset de escala 2)
        {
          //ajusteFinoVel = 0.99; // reemplazado por presets, se podría ubicar en otra tecla
//          pepas[selector]->preset(2);
//          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->preset(2); 
        }
        else if(Byte == 0x83) // presionando F7 (preset de escala 3)
        {
          //ajusteFinoVel = 1.01; // reemplazado por presets, se podría ubicar en otra tecla
//          pepas[selector]->preset(3);
//          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->preset(3); 
        }
        else if(Byte == 0x0A) // presionando F8 (preset de escala 4)
        {
          //ajusteFinoVel = 1.05; // reemplazado por presets, se podría ubicar en otra tecla
//          pepas[selector]->preset(4);
//          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->preset(4); 
        }
        else if(Byte == 0x0E) // presionando ` (sincronizar)
        {
          for(uint8_t i = 0; i < cantPepas; i++) 
          {
            if(i != selector) 
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
        else if(Byte == 0x76) // presionando ESC (resetear timingCap en todas las pepas) /////////////////////// agregar funcion de tap tempo cuando esta presionado SHIFT
        {
          if (shift == 1)
          {
            insertarTap();
          }
          else 
          {
            for(uint8_t i = 0; i < cantPepas; i++) 
            {
              pepas[i]->reiniciarCabezal();
            }
          }
        }
        else if(Byte == 0x12) // presionando L SHIFT
        {
          if(buscar(0x14) >= 0) for(uint8_t i = 0; i < cantPepas; i++) if(i != selector) pepas[i]->controlarCTRL(1); //checkear si esta presionado L CTRL
          if(buscar(0x11) >= 0) for(uint8_t i = 0; i < cantPepas; i++) if(i != selector) pepas[i]->controlarALT(1); //checkear si esta presionado L ALT
          if(cantPresionadas > 0) for(uint8_t i = 0; i < cantPresionadas; i++) if(K2Midi(i) > 0) for(uint8_t j = 0; j < cantPepas; j++) pepas[j]->agregar(K2Midi(i)); 
          shift = 1;
        }
        else if(Byte == 0x0D) // presionando TAB
        {
          selector++;
          if(selector > cantPepas - 1) selector = 0;
          
          // indicar selector en los LEDs en formato binario (solo se usan los 2 de la derecha, el primero de la izquierda queda libre para ser indicador de trigger)
          attachInterrupt(CLOCK_PIN_INT, ps2int_write, FALLING);
          enviar(0xED);
          //enviar(selector); 
          uint8_t selectorModificado = 0;
          if(selector==0){
            enviar(selectorModificado);
          }
          if(selector==1){
            bitWrite(selectorModificado,0,1);
            enviar(selectorModificado);
          }
          if(selector==2){
            bitWrite(selectorModificado,2,1);
            enviar(selectorModificado);
          }
          if(selector==3){
            bitWrite(selectorModificado,0,1);
            bitWrite(selectorModificado,2,1);
            enviar(selectorModificado);
          }
          estadosLED = selectorModificado;
          attachInterrupt(CLOCK_PIN_INT, ps2int_read, FALLING);
          releaseClock();
          inhibiting = false;
        }
        else if(K2Num(Byte) > -1)  // presionando KP num
        {
          if(pepas[selector]->control > 0)
          {
            if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->numero = concatenar(pepas[i]->numero, K2Num(Byte));
            else pepas[selector]->numero = concatenar(pepas[selector]->numero, K2Num(Byte));
          }
          
          if(setTempo == 1) tempo = concatenar(tempo, K2Num(Byte));
        }
        else if(Byte == 0x7C) // presionando KP * (para multiplicar velocidad)
        {
          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->controlarMult(1); 
          else pepas[selector]->controlarMult(1);
        }
        if(Byte == 0x4A && E0Key == 1) // presionando KP / (para dividir velocidad)
        {
          if(shift == 1) for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->controlarDiv(1); 
          else pepas[selector]->controlarDiv(1);
        }
        if(Byte == 0x75 && E0Key == 1) // presionando UP Arrow
        {
          pepas[selector]->subirOctava();
        }
        if(Byte == 0x72 && E0Key == 1) // presionando DOWN Arrow
        {
          pepas[selector]->bajarOctava();
        }
      }
    }
    E0Key = 0;
  }
}
if(currentMillis >= futurMillis) pausa = 0;
}
