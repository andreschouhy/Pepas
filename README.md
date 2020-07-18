# Pepas
###### Versión español (scroll down for the english version)
Pepas es un módulo secuenciador generativo de CV, gates y triggers basado en Arduino para sintetizadores modulares. 
Está diseñado para ser económico y accesible. Su corazón es un Arduino UNO, y se opera a través de un teclado de computadora PS/2. El resto son componentes básicos de electrónica como resistencias, capacitores y demás.
Está diseñado en vistas a ser una herramienta de experimentación musical, mediante el caos controlado. Las secuencias producidas por Pepas son únicas e irrepetibles, y no son guardadas en ninguna memoria. Lo que sí es reproducible son los parámetros que controlan el caos. Por ello no está pensado para el compositor musical que sabe exactamente qué notas quiere y en dónde. Sino mas bien para el músico que quiere experimentar ágilmente con ciertos esquemas musicales. El sistema carece de pantallas y menúes complicados, todo está siempre al alcance de los botones. Los unicos indicadores que hay son los LEDs que posee el teclado (uno para indicar que se esta ejecutando una nota en el canal actual y los 2 restantes para indicar el canal actual en formato binario del 0 al 3, o en otras palabras del primero al cuarto). 

Para una introducción más didáctica, vea el siguiente video:
https://www.youtube.com/watch?v=Qqi9xfu0wNE

![Referencia Frente](https://github.com/andreschouhy/Pepas/blob/master/frente%20prototipo.jpg?raw=true)

Referencias 3 y 4 sin uso actualmente, fueron parte de un estadío previo de desarrollo.

Actualmente posee 4 canales independientes y sincronizables:
- 2 canales de CV, y cada uno posee:
  - una salida analógica de CV controlable mediante el teclado (referencias 5 y 9)
  - una salida analógica de CV aleatorio, por ejemplo para controlar cambios de amplitud de un VCA o variar aleatoriamente un VCF, etc (referencias 7 y 11)
  - una salida digital de trigger (referencias 8 y 12)
  - una salida digital de gate (referencias 6 y 10)
- 2 canales de square envelope (encontrar una traducción coherente para este termino):
  - una salida digital cada uno (referencias 13 y 14)
- 1 entrada analógica de potenciómetro, para control fino de ciertos parámetros (referencia 2)
- 1 puerto PS/2 (referencia 1)

## Galería de música creada con Pepas
Mucha cháchara, mucha cháchara... pero vamo a lo bifes:
- https://soundcloud.com/andreschouhy

## Requerimientos
- Arduino UNO
- Teclado de computadora PS/2 (yo estoy usando un Genius KB-06XE en español, he probado otros que no funcionaban, no pude identificar por qué)
- Un circuito de conversion digital a analogico por cada salida analógica, en total son 4: 2 por cada canal de CV (yo estoy usando algo asi https://www.instructables.com/id/Another-MIDI-to-CV-Box-/)
- Fuente de 5V CC
- Básicos conocimientos de electrónica (para el armado de los circuitos y la integracion del Arduino)

## Instalación
De momento no hay archivos compilados, la forma de cargarlo en Arduino es la habitual, por medio de Arduino IDE. Simplemente hay bajar los archivos de este repositorio y uploadear desde el Arduino IDE como cualquier sketch.
Para la parte de electrónica seguir el link pasado con anterioridad.

## Modus operandi
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

## Mapa de botones
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
- "FLECHA ARRIBA" y "FLECHA ABAJO" cambia la octava de escala (sólo para los canales de CV)
- "ESC" tap tempo (actualmente no está funcionando bien)
- "F1" configura el parámetro de mutación (para las secuencias fijas) mediante el uso del potenciómetro (presionar "F1" > mover potenciómetro > soltar "F1")
- "F2" configura el tempo en BPM usando el teclado numerico (presionar "F2" > entrar BPM > soltar "F2")
- "F5" hasta "F8" activar presets (actualmente sólo de escala, quizás en el futuro se implementen otros parámetros para presetear), esto permite progresiones de acordes (o más bien, arpegios basados en acordes)
- " * " y " / " (del teclado numérico) multiplica y divide (respectivamente) la velocidad relativa de los pasos de la secuencia por un número entero ingresado mediante el teclado numérico (presionar " * " o " / " > entrar un número > soltar " * " o " / "). Por defecto en 1, si se multiplica por 2, por ejemplo, se disparan 2 notas en el mismo tiempo que antes se disparaba una.

## Fallas y cuestiones a mejorar
- Creo que lo más urgente para optimizar es la comunicación con el teclado. Quizas probar otra librería.
- Algunos teclados no funcionan, no pude identificar por qué. Quizás algun tema de protocolos. También podría solucionarse utilizando otra librería.
- Ocasionalmente se cuelga. Quizás un problema de falta de memoria?
- Ocasionalmente no cierra la escala y agrega todas las notas que se activan, a veces se soluciona presionando "TAB" 2 veces (forzando a cerrar la escala, reiniciándola).
- El tempo no es preciso como un reloj, se desvía un poco (sospecho que la temperatura ambiente tiene que ver) aunque no es notorio luego de unos minutos de sincronizado. Se vuelve notorio luego de los 20-30 minutos aproximadamente, pero es improbable que se quiera mantener una precisión fuerte luego de tanto tiempo.
- Una buena incorporación sería una entrada de clock, para delegar el manejo del tiempo a otros sistemas más idóneos y precisos.
- Quizás estaría bien agregar un modo de arpegio regular, no aleatorio, en el cual se podrían elegir arpegios ascendentes, descendentes, ping-pong, y algún otro.
- Quizás alguna forma de guardar información más permanentemente, accesible luego de reiniciar el Arduino o de haber cambiado mucho las cosas. Aunque no sé si hecha a perder un poco el sentido de Pepas.
- Entradas/salidas MIDI/OSC?

# Pepas
###### English version
Pepas is a CV, gate and trigger generative sequencer module based on Arduino for modular synthesizers. It is designed to be cheap and accessible. It's heart is an Arduino UNO and it relies on a PS/2 computer keyboard to interact with it. The rest are basic electronics components, such as capacitors, resistors and such. 
It is designed to be a musical experimentation tool, based on a controlled chaos. Sequences produced by Pepas are unique and unrepeatable, and are not saved in any memory. What it is repeatable are the parameters that control the chaos. So it's not made for the musical composer that knows exactly which notes and when does he or she want them, but for the musician that wants to experiment nimbly with certain musical schemes. The system lacks screens and complicated menues, all is allways to the reach of the user. The only indicators are the LEDs that has the keyboard (one to indicate that a note is being executed in the selected channel and the other 2 to indicate what channel is selected in a binary format from 0 to 3, or in other words from the first to the fourth).

For a more didactic introduction, see the next video:
https://www.youtube.com/watch?v=Qqi9xfu0wNE

![References Front](https://github.com/andreschouhy/Pepas/blob/master/frente%20prototipo.jpg?raw=true)

References 3 and 4 currently not used, were part of a previous developing state.

By now it has 4 independant and synchronizable channels:
- 2 CV channels, each one has:
  - one analog CV output controllable through the keyboard (references 5 and 9)
  - one analog random CV output, useful for controlling amplitud changes in a VCA or vary randomly a VCF, or such (references 7 and 11)
  - one digital trigger output (references 8 and 12)
  - one digital gate output (references 6 and 10)
- 2 square envelope channels, each one has:
  - one digital output (references 13 and 14)
- 1 analog input from a potentiometer, to fine control some parameters (reference 2)
- 1 PS/2 port (reference 1)

## Gallery of music created with Pepas
Enough with the chatter... let's get into the shit yo!:
- https://soundcloud.com/andreschouhy

## Requirements
- Arduino UNO
- PS/2 computer keyboard (I'm using a Genius KB-06XE in spanish, I've tryied some keyboards that didn't work, couldn't identify why)
- A digital to analog converter circuit for each of the analog outputs, 4 in total: 2 for each CV channel (I'm using something like this https://www.instructables.com/id/Another-MIDI-to-CV-Box-/)
- 5V DC power supply
- Basic electronics knowledge (for putting together the circuits and Arduino integration)

## Installing
By now there are no compiled files, the way to load it in the Arduino is the usual, trough the Arduino IDE. Simply download the files in this repository and upload them using the Arduino IDE as any sketch.
For the electronics, follow the link shown before.

## Modus operandi
In the CV channels, the user activates a grup of notes, through the use of the keyboard, to wich we are calling "scale", and Pepas randomly execute any of those notes. The amount of notes in a scale can be from 1 to 16 notes (could be more if I could manage it, I suspect there is a memory limit issue on the Arduino UNO).
On the same way, at the square envelope channels, instead of notes, the user selects the length of the duty cycle, from 0% (activating the lowest "note") to 100% (activating the highest "note"). These channels can be used as a clock output for using in other modules, being tempo synchronizable with the rest of the channels. They can also be used as a way to gate other modules.
There are 2 sequence modes, one of completly random sequences and other of fixed sequences (randomly generated, under user defined parameters). Pepas has also a functionality to slowly mutate fixed sequences, to avoid constant repeat forever. It's way to work is to alter a step of the sequence every time a cycle is completed. Pepas decides whether to alter it or not based on the mutation probability parameter, wich is in 0% by default but the user can set it up to 100% using the potentiometer.

Most important parameters are:
- The scale (the group of activated notes)
- The amount of notes in a scale (for the fixed sequences mode)
- The amount of steps of a sequence (for the fixed sequences mode)
- The tempo, settable by BPM, by "tap tempo" or by potentiomenter. It is universal, every channels obey to the same tempo, although relative durations on each channel can be different.
- The probability of executing a note, settable by potentiometer.
- The probability of alter a step of a sequence (for the fixed sequences mode)

## Keymap
- Musical notes (for the CV channels): 34 possible notes are spread through the letters and numbers buttons in this way:
  - "white" notes (from a traditional musical keyboard) on the row from "Z" to "/" and from "Q" to "P"
  - "black" notes (from a traditional musical keyboard) on the row from "A" to ";" and from "1" to "0" (excluding "A", "F", "K", "1", "4", and "8")
- Duty cycle length (for the square envelope channels): 0-100% spread through the same buttons as musical notes.
- "TAB" selects the channel (the 2 LEDs from the right on the 3 LEDs group of the keyboard acts as indicators for the current channel in a binary format from channel 0 to channel 3)
- The first LED to the left of the 3 LEDs group in the keyboard indicates when a note is being executed.
- "CAPS LOCK" switches between 2 modes fo executing notes:
  - one in wich the executed notes are the ones that are being phisically pressed
  - other in wich those notes are held active despites of being or not phisically pressed until the first note of a new group of notes is pressed.
- "SPACEBAR" switches between the random sequences mode and the fixed sequences mode.
- "BACKSPACE" resets the sequence (for the fixed sequences mode)
- " ` " (the button to the left of the "1") synchronizes the time on every channels taking the current channel as the master (meaning that it is not altered)
- Holding "CTRL" the user can:
  - enter the amount of steps in a fixed sequence, from 1 to 64, using the numpad
  - set the tempo using the potentiometer
  - store a preset (also pressing "F5","F6","F7" or "F8")
- Holding "ALT" the user can:
  - enter the amount of notes in the scale, from 1 to 16, using the numpad
  - set the notes execution probability through the use of the potentiometer, from 0% to 100%
- "SHIFT" propagates all to the other channels
- "UP ARROW" and "DOWN ARROW" changes the octave of the scale (only for CV channels)
- "ESC" tap tempo (not working currently)
- "F1" sets the mutation probability parameter (for fixed secuences) through the use of the potentiometer (hold "F1" > work the potentiometer > release "F1")
- "F2" sets the tempo in BPM using the numpad (hold "F2" > enter BPM > release "F2")
- "F5" to "F8" activate stored presets (only scale is stored, maybe in a future more paremeters gets implemented in the presets), this allows for chords progressions (actually, progressions of chord-based arpeggios).
- " * " and " / " (from the numpad) multiplies or divides (respectively) the relative speed of the sequence steps by an integer number entered through the numpad (hold " * " or " / " > enter a number > release " * " or " / "). By default it's 1, if you multiply it by 2, for instance, 2 notes are executed in the same time that 1 was being executed.

## Issues to improve
- Most urgent issue is to optimize keyboard communication. Maybe try another library.
- Some keyboards don't work, couldn't identify why. Maybe some protocol issue. Also might be solved using another library.
- Occasionally it hangs. Maybe lack of enough memory?
- Occasioanlly scales are not closed and keeps adding notes, sometimes it finally closes the scale when pressing "TAB" 2 times (hence forcing to close the scale, reseting it)
- Tempo is not clock precise, it deviates a little (I suspect that ambient temperature has something to do) although is not so notorious after a few minutes from synchronization. It gets notorious after something like 20-30 minutes, but it is not so likely that someone wants that strong precision after that much time.
- A good incorporation would be a clock input, to delegate time handling to other more suitable and precise systems.
- Maybe it would be nice to add a regular arpegio mode, nor tandom, in wich the user could pick from acsending , descending, ping-pong, and other tipes of arpeggios.
- Maybe a way to store information more permanently, accesible after rebooting the Arduino or after having changed things a lot. Although I'm not sure if this spoils the point of Pepas.
- MIDI/OSC inputs/outputs?
