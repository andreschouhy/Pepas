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
- Notas musicales (para los canales de CV): 34 notas posibles se distribuyen a lo largo de las teclas de letras y números de la siguiente manera:
  - notas "blancas" (de un teclado musical tradicional) en las fila desde "Z" hasta "-" y desde "Q" hasta "P"
  - notas "negras" (de un teclado musical tradicional) en las filas desde "A" hasta "Ñ" y desde "1" hasta "0" (excluyendo "A", "F", "K", "1", "4" y "8")
- Duración del ciclo de square envelope (para los canales de square envelope): 0-100% distribuido en las mismas teclas que las notas musicales.
- "TAB" selecciona el canal actual
- "BLOCK MAYUS" alterna entre 2 modos de accionar notas:
  - uno en el que se accionan las notas que están siendo presionas
  - otro en el que se mantienen activas las notas que se presionaron desde que se presiona la primera nota hasta que ya no hay notas presionadas, luego de esto se reinicia el grupo de notas al presionar la primera de un nuevo grupo de notas.
