# Pepas
Pepas es un módulo secuenciador generativo de CV, gates y triggers basado en Arduino para sintetizadores modulares. 
Está diseñado para ser económico y accesible. Su corazón es un Arduino UNO, y se opera a través de un teclado de computadora PS/2. El resto son componentes básicos de electrónica como resistencias, capacitores y demás.
Está diseñado en vistas a ser una herramienta de experimentación musical, mediante el uso de la aleatoriedad. Las secuencias producidas por Pepas son únicas e irrepetibles, y no son guardadas en ninguna memoria. Por ello no está pensado para el compositor musical que sabe exactamente qué notas quiere y en dónde. Sino mas bien para el músico que quiere experimentar rápidamente con ciertos esquemas musicales.

Actualmente posee 4 canales independientes y sincronizables:
- 2 canales de CV, y cada uno posee:
  - una salida analógica de CV controlable mediante el teclado
  - una salida analógica de CV aleatorio (por ejemplo para controlar cambios de amplitud de un VCA o variar aleatoriamente un VCF, etc)
  - una salida digital de trigger
  - una salida digital de gate
- 2 canales de square envelope (encontrar una traducción coherente para este termino):
  - una salida digital cada uno

Modus operandi:
