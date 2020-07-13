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
- 1 entrada analógica de potenciómetro, para control fino de ciertos parámetros

Modus operandi:
En los canales de CV, básicamente, uno activa un grupo de notas mediante el uso del teclado, a las que llamaremos "escala", y Pepas dispara aleatoriamente cualquiera de esas notas. La cantidad de notas de una escala puede ser desde 1 hasta 16 notas (podrían ser más si pudiera, sospecho que hay un problema con el límite de memoria del Arduino UNO).
De la misma forma, en los canales de square envelope, en lugar de notas se selecciona cuanto dura el ciclo digital entre 0 y 1, desde 0% (activando la "nota" más baja) hasta 100% (activando la "nota" más alta). Éstos canales pueden usarse como salida de clock para otros módulos, ya que son sincronizables en tempo con el resto de los canales. También pueden usarse para activar y desactivar ciertos módulos por medio de gates.
Los parámetros más importantes son: 
- La escala (el grupo de notas activadas)
- La cantidad de notas de una escala (para el modo de secuencias fijas)
- La cantidad de pasos de una secuencia (para el modo de secuencias fijas)
- El tempo, configurable por BPM, por "tap tempo" o por potenciómetro
- La probabilidad de ejecutar una nota, configurable mediante el potenciómetro


- Notas musicales (para los canales de CV): 34 notas posibles se distribuyen a lo largo de las teclas de letras y números de la siguiente manera:
  - notas "blancas" (de un teclado musical tradicional) en las fila desde "Z" hasta "-" y desde "Q" hasta "P"
  - notas "negras" (de un teclado musical tradicional) en las filas desde "A" hasta "Ñ" y desde "1" hasta "0" (excluyendo "A", "F", "K", "1", "4" y "8")
- Duración del ciclo de square envelope (para los canales de square envelope): 0-100% distribuido en las mismas teclas que las notas musicales.
- "TAB" selecciona el canal actual (los 2 LEDs de la derecha del grupo de 3 LEDs del teclado funcionan como indicador del canal actual en formato binario desde el canal 0 al canal 3)
- El LED de la izquierda del grupo de 3 LEDs del teclado indica cuándo una nota de está disparando.
- "BLOCK MAYUS" alterna entre 2 modos de accionar notas:
  - uno en el que se accionan las notas que están siendo presionas
  - otro en el que se mantienen activas las notas que se presionaron desde que se presiona la primera nota hasta que ya no hay notas presionadas, luego de esto se reinicia el grupo de notas al presionar la primera de un nuevo grupo de notas.
- "ESPACIO"
- "BORRAR"
- "'" (la tecla a la izquierda del "1")
- "CTRL"
- "ALT"
- "SHIFT"
- "FLECHA ARRIBA" y "FLECHA ABAJO"
- "ESC"
- "F1"
- "F2"
- "F5" hasta "F6"
- "*" y "/" (del teclado numérico)
