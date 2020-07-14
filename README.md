# Pepas
Pepas es un módulo secuenciador generativo de CV, gates y triggers basado en Arduino para sintetizadores modulares. 
Está diseñado para ser económico y accesible. Su corazón es un Arduino UNO, y se opera a través de un teclado de computadora PS/2. El resto son componentes básicos de electrónica como resistencias, capacitores y demás.
Está diseñado en vistas a ser una herramienta de experimentación musical, mediante el caos controlado. Las secuencias producidas por Pepas son únicas e irrepetibles, y no son guardadas en ninguna memoria. Lo que sí es reproducible son los parámetros que controlan el caos. Por ello no está pensado para el compositor musical que sabe exactamente qué notas quiere y en dónde. Sino mas bien para el músico que quiere experimentar ágilmente con ciertos esquemas musicales. 

Actualmente posee 4 canales independientes y sincronizables:
- 2 canales de CV, y cada uno posee:
  - una salida analógica de CV controlable mediante el teclado
  - una salida analógica de CV aleatorio (por ejemplo para controlar cambios de amplitud de un VCA o variar aleatoriamente un VCF, etc)
  - una salida digital de trigger
  - una salida digital de gate
- 2 canales de square envelope (encontrar una traducción coherente para este termino):
  - una salida digital cada uno
- 1 entrada analógica de potenciómetro, para control fino de ciertos parámetros

# Requerimientos
- Arduino UNO
- Teclado de computadora PS/2 (yo estoy usando un Genius en español, he probado otros que no funcionaban, no pude identificar por qué)
- Un circuito de conversion digital a analogico por cada salida analógica, en total son 4: 2 por cada canal de CV (yo estoy usando algo asi https://www.instructables.com/id/Another-MIDI-to-CV-Box-/)
- Fuente de 5VCC

# Modus operandi
En los canales de CV, básicamente, uno activa un grupo de notas mediante el uso del teclado, a las que llamaremos "escala", y Pepas dispara aleatoriamente cualquiera de esas notas. La cantidad de notas de una escala puede ser desde 1 hasta 16 notas (podrían ser más si pudiera, sospecho que hay un problema con el límite de memoria del Arduino UNO). 
De la misma forma, en los canales de square envelope, en lugar de notas se selecciona cuanto dura el ciclo digital entre 0 y 1, desde 0% (activando la "nota" más baja) hasta 100% (activando la "nota" más alta). Éstos canales pueden usarse como salida de clock para otros módulos, ya que son sincronizables en tempo con el resto de los canales. También pueden usarse para activar y desactivar ciertos módulos por medio de gates.
Hay 2 modos de secuencias, uno de secuencia completamente aleatoria y otro de secuencias fijas (generadas aleatoriamente, bajo parámetros definidos por el usuario). Pepas posee también una funcionalidad para mutar levemente las secuencias fijas, para que no se repitan exactamente iguales por siempre. Su forma de operar es alterar un paso de la secuencia cada vez que la secuencia cumple un ciclo. Pepas decide si alterarlo o no mediante una probabilidad, que por defecto esta en 0% pero el usuario puede aumentar esa probabilidad a 100% operando el potenciómetro.
Los parámetros más importantes son: 
- La escala (el grupo de notas activadas)
- La cantidad de notas de una escala (para el modo de secuencias fijas)
- La cantidad de pasos de una secuencia (para el modo de secuencias fijas)
- El tempo, configurable por BPM, por "tap tempo" o por potenciómetro. Es universal, todos los canales obedecen al mismo tempo, aunque las duraciones de notas de cada canal pueden ser distintas.
- La probabilidad de ejecutar una nota, configurable mediante el potenciómetro
- La probabilidad de alterar un paso de una secuencia (para el modo de secuencias fijas)

# Mapa de botones
- Notas musicales (para los canales de CV): 34 notas posibles se distribuyen a lo largo de las botones de letras y números de la siguiente manera:
  - notas "blancas" (de un teclado musical tradicional) en las fila desde "Z" hasta "-" y desde "Q" hasta "P"
  - notas "negras" (de un teclado musical tradicional) en las filas desde "A" hasta "Ñ" y desde "1" hasta "0" (excluyendo "A", "F", "K", "1", "4" y "8")
- Duración del ciclo de square envelope (para los canales de square envelope): 0-100% distribuido en las mismas botones que las notas musicales.
- "TAB" selecciona el canal actual (los 2 LEDs de la derecha del grupo de 3 LEDs del teclado funcionan como indicador del canal actual en formato binario desde el canal 0 al canal 3)
- El LED de la izquierda del grupo de 3 LEDs del teclado indica cuándo una nota de está disparando.
- "BLOCK MAYUS" alterna entre 2 modos de accionar notas:
  - uno en el que se accionan las notas que están siendo presionas
  - otro en el que se mantienen activas las notas que se presionaron desde que se presiona la primera nota hasta que ya no hay notas presionadas, luego de esto se reinicia el grupo de notas al presionar la primera de un nuevo grupo de notas.
- "ESPACIO" alterna entre el modo de secuencias aleatorias y secuencias fijas
- "BORRAR" resetea la secuencia (para el modo de secuencias fijas)
- " ' " (el botón a la izquierda del "1") sincroniza el tiempo de todos los canales tomando como master el canal actual (es decir, sin alterarlo)
- Manteniendo "CTRL" se puede:
  - ingresar la cantidad de pasos de la secuencia fija, desde 1 hasta 64, utilizando el teclado numérico
  - configurar el tempo mediante el uso del potenciómetro
  - guardar un preset (presionar "CTRL" > presionar "F5","F6","F7" o "F8" > soltar "CTRL")
- Manteniendo "ALT" se puede:
  - ingresar la cantidad de notas que tiene la escala, desde 1 hasta 16, utilizando el teclado numérico
  - configurar la probabilidad de ejecución de notas mediante el uso del potenciómetro, desde 0% hasta 100%
- "SHIFT" propaga todo lo que se hace en un canal a los demás canales
- "FLECHA ARRIBA" y "FLECHA ABAJO" cambia de escala (sólo para los canales de CV)
- "ESC" tap tempo (actualmente no está funcionando bien)
- "F1" configura el parámetro de mutación (para las secuencias fijas) mediante el uso del potenciómetro (presionar "F1" > mover potenciómetro > soltar "F1")
- "F2" configura el tempo en BPM usando el teclado numerico (presionar "F2" > entrar BPM > soltar "F2")
- "F5" hasta "F6" activar presets (actualmente sólo de escala, quizás en el futuro se implementen otros parámetros para presetear), esto permite progresiones de acordes (o más bien, arpegios basados en acordes)
- " * " y " / " (del teclado numérico) multiplica y divide (respectivamente) la velocidad relativa de los pasos de la secuencia por un número entero ingresado mediante el teclado numérico (presionar " * " o " / " > entrar un número > soltar " * " o " / "). Por defecto en 1, si se multiplica por 2, por ejemplo, se disparan 2 notas en el mismo tiempo que antes se disparaba una.

# Fallas y cuestiones a mejorar
- Creo que lo más urgente para optimizar es la comunicación con el teclado. Quizas probar otra librería.
- Ocasionalmente se cuelga. Quizás un problema de falta de memoria?
- Ocasionalmente no cierra la escala y agrega todas las notas que se activan, a veces se soluciona presionando "TAB" 2 veces (forzando a cerrar la escala, reiniciándola).
- El tempo no es preciso como un reloj, se desvía un poco (sospecho que la temperatura ambiente tiene que ver) aunque no es notorio luego de unos minutos de sincronizado. Se vuelve notorio luego de los 20-30 minutos aproximadamente, pero es improbable que se quiera mantener una precisión fuerte luego de tanto tiempo.
- Una buena incorporación sería una entrada de clock, para delegar el manejo del tiempo a otros sistemas más idóneos y precisos.
