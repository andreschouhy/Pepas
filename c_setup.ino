void setup()
{
  // Apagar el watchdog apenas arranca: si un reset lo dejo activo, el boot (~10s) es mas largo
  // que el timeout y entraria en loop de reinicio. Se re-habilita al final de setup().
  wdt_disable();

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  
  pinMode(4, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(12, OUTPUT);
  digitalWrite(8, LOW);
  digitalWrite(12, LOW);
  
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  setPwmFrequency(5, 1); // 1
  setPwmFrequency(6, 8); // 8
  setPwmFrequency(9, 8); // 8
  setPwmFrequency(10, 1); // 1
  //setPwmFrequency(11, 8);
  analogWrite(5, 0);
  analogWrite(6, 0);
  analogWrite(9, 0);
  analogWrite(10, 0);
  //digitalWrite(10, LOW);
  //analogWrite(11, 0);
  digitalWrite(11, LOW);

  pinMode(A0, INPUT);

  pinMode(notaLedPin, OUTPUT);      // LED de feedback de nota-on (canal seleccionado)
  digitalWrite(notaLedPin, LOW);

  pinMode(extClockSwitchPin, INPUT);
  pinMode(extClockPin, INPUT_PULLUP);
  clockSwitch = digitalRead(extClockSwitchPin);
  
  pepas[0] = new Pepa(5, 4, 6, 7, 0, 0);
  pepas[1] = new Pepa(9, 8, 10, 12, 0, 1);
  pepas[2] = new Pepa(11, 11, 11, 11, 1, 2);
  pepas[3] = new Pepa(13, 13, 13, 13, 1, 3);
  
  releaseClock();
  releaseData();
  ps2Init();
  attachInterrupt(CLOCK_PIN_INT, ps2int_read, FALLING);
  
  delay(500 * multTemp);
  
  // animacion de bienvenida
  attachInterrupt(CLOCK_PIN_INT, ps2int_write, FALLING);
  for(uint8_t i = 0x00; i < 0x08; i++)
  {
    enviar(0xED);
    enviar(i);
    delay(100 * multTemp);
  }
  enviar(0xED);
  enviar(0x00); 
  attachInterrupt(CLOCK_PIN_INT, ps2int_read, FALLING);
  releaseClock();
  inhibiting = false;
  
  while (ps2Available())
  {
    uint8_t Byte = ps2Read();
  }

  cargarEstado();          // recuperar el patch guardado (si hay uno valido) antes de arrancar
                           // (ya refleja el canal cargado en los LEDs del teclado)
  actualizarLEDSelector(); // por si no habia patch guardado: reflejar el canal 0 de arranque

  // Watchdog: si el loop se cuelga y no se resetea el WDT en 4s, el micro se reinicia solo.
  // Se habilita ACA, despues del boot (~10s de animacion + delays): antes lo dispararia.
  // Timeout holgado (4s) sobre el peor caso del loop, que es el guardado en EEPROM (~1-2s);
  // igual guardarEstado() hace wdt_reset() entre voces para no arriesgar.
  wdt_enable(WDTO_4S);

  //Serial.begin(115200);
  //Serial.print("capacidad: ");
  //Serial.println(capacidad);
  //Serial.println(pepas[0]->velocidad);

  //prevMillis = millis() / multTemp;
}

void loop()
{
  wdt_reset(); // patear el watchdog: si el loop sigue vivo, no se reinicia

  currentMillis = millis() / multTemp;
  deltaMillis = currentMillis - prevMillis;

  if (deltaMillis >= 1) 
  {
    prevMillis = currentMillis;
  }

  boolean clockInput = digitalRead(extClockPin);
  clockSwitch = digitalRead(extClockSwitchPin);
  
  if (clockSwitch == true)
  {
    for (uint8_t i = 0; i < cantPepas; i++) 
    {
      pepas[i]->triggerLoopCheck(); // actualizar triggerLoop
    }
    
    if (clockInput == HIGH && clockCheck == false)
    {
      clockCheck = true;
      
      // resetear secuencia luego de que se haya interrumpido el clock externo
      clockDifCurrent = currentMillis - clockMillisPrev;
      clockDifPrev = clockMillisPrev - clockMillisPrevPrev;
      if (clockDifCurrent > (clockDifPrev * 8)) 
      {
        for (uint8_t i = 0; i < cantPepas; i++) 
        {
          pepas[i]->reiniciarCabezal();
        }
      }
      clockMillisPrevPrev = clockMillisPrev;
      clockMillisPrev = currentMillis;
      
      for (uint8_t i = 0; i < cantPepas; i++) 
      {
        pepas[i]->actualizar(); // actualizar las pepas
      }
    }
    else if (clockInput == LOW && clockCheck == true)
    {
      clockCheck = false;
    }
  }
  else
  {
    for(uint8_t i = 0; i < cantPepas; i++) 
    {
      pepas[i]->actualizar(); // actualizar las pepas
    }
  }
  
  eventoTeclado();

  notaLedLoop(); // apagar el LED de nota-on cuando venza su parpadeo (no bloqueante)

  pote = analogRead(A0);
  
  if(controlarVelocidad == 1)
  {
    velocidadGeneral = (pote * precision) + poteSnapshotGral;

    if(velocidadGeneral > (1024L * precision)) 
    {
      velocidadGeneral = 1024L * precision;
      poteSnapshotGral = velocidadGeneral - (pote * precision);
    }
    else if(velocidadGeneral <= (1L * precision)) 
    {
      velocidadGeneral = 1L * precision;
      poteSnapshotGral = velocidadGeneral - (pote * precision);
    }
  }
}
