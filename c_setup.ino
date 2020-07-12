void setup() {
  //pinMode(ledPin, OUTPUT);
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
  
  pepas[0] = new Pepa(5, 4, 6, 7, analogRead(A0), 0, 0);
  pepas[1] = new Pepa(9, 8, 10, 12, analogRead(A0), 0, 1);
  pepas[2] = new Pepa(11, 11, 11, 11, analogRead(A0), 1, 2);
  pepas[3] = new Pepa(13, 13, 13, 13, analogRead(A0), 1, 3);
  
  releaseClock();
  releaseData();
  head = tail = 0;
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
  
  while(ps2Available()) uint8_t Byte = ps2Read();
  
  //Serial.begin(115200);
  //Serial.print("capacidad: ");
  //Serial.println(capacidad);
  //Serial.println(pepas[0]->velocidad);

  //prevMillis = millis() / multTemp;
}

void loop() {
  //delay(1 * multTemp);
  currentMillis = millis() / multTemp;
  diferencia = currentMillis - prevMillis;
  //diferencia /= multTemp;

  if(diferencia >= 1) prevMillis = currentMillis;
  
  for(uint8_t i = 0; i < cantPepas; i++) pepas[i]->actualizar();
  
  eventoTeclado();
  
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
