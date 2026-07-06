# Tareas de limpieza pendientes

Registro de mejoras de estructura/limpieza identificadas el 2026-06-24 pero **no aplicadas todavia**.
Ninguna es urgente; el sketch compila y anda. Son refactors de codigo, sin cambio de comportamiento
buscado (salvo donde se indique). Conviene hacerlas de a una y verificarlas en el dispositivo por separado.

Ya hecho en esta pasada (no rehacer): borrado de codigo muerto (`state`, `fromHex`, `ledPin`,
`tamanoEscalaLimite`, `_pote`, `atualizarLED`, rama vacia `control==1`) y guardas de limites en
`Pepa::agregar()` y `Pepa::quitar()` (esta ultima arregla el desborde de `escalaSize` al soltar notas).

Hecho 2026-06-30 (FALTA PROBAR EN EL DISPOSITIVO):
- #1 Driver PS/2 extraido a `ps2.h`/`ps2.cpp` (incluye el decodificador `ps2NextKey`).
- #3 Scancodes con nombre (`SC_*` en `Pepas.ino`), dispatcher reescrito con ellos.
- Reescritura de `eventoTeclado` en `manejarPresionar`/`manejarSoltar` sobre eventos decodificados.
- Fix de transporte: el decodificador drena el buffer entero, sin la pausa de ~512ms que
  desbordaba el ring buffer y perdia break codes (causa de la escala que no se reseteaba).
- Conteo de notas via `recontarNotas()` desde `presionadas[]` (fuente unica de verdad);
  soltar es simetrico y no corrompe `presionadas[]` con un break huerfano.
- #5 `insertarTap()` desconectado de Shift+ESC (sigue definido en `Pepas.ino`, sin usar).

---

## 1. Extraer el driver PS/2 a su propio modulo  (HECHO 2026-06-30)
Movido a `ps2.h`/`ps2.cpp`: ISRs, ring buffer, helpers open-collector, `inhibiting`, pines,
`enviar` y el decodificador `ps2NextKey`. `Pepas.ino` queda como glue de la app. Las variables
`volatile` compartidas con las ISR siguen `volatile`. `head`/`tail` se resetean via `ps2Init()`.

## 2. Deduplicar el patron broadcast del dispatcher  (`b_eventoTeclado.ino`)  (HECHO 2026-07-02)
El bloque `if(shift) for(...) pepas[i]->X(arg); else pepas[selector]->X(arg);` (repetido ~18 veces)
se reemplazo por el macro `BROADCAST(call)` (definido arriba de `manejarPresionar`). El bloque del
teclado numerico sigue manual: su argumento referencia el miembro `numero`, que el macro no puede
calificar. El bloque selector->LED del TAB ya se habia extraido a `actualizarLEDSelector()`.

## 3. Nombrar los scancodes  (`b_eventoTeclado.ino`)  (HECHO)
Los scancodes eran numeros magicos (`0x14`, `0x11`, `0x05`, `0x29`, `0x58`...). Ahora hay `#define
SC_*` en `Pepas.ino` y el dispatcher los usa; queda autoexplicativo.

## 4. Largos de array por `sizeof`  (`Pepas.ino`)  (HECHO 2026-07-02)
`K2Midi` y `K2Num` ahora iteran con `sizeof(mapa)/sizeof(mapa[0])` y
`sizeof(mapaNum)/sizeof(mapaNum[0])`, asi el limite no se desincroniza de los datos si se
agregan/quitan filas.

## 5. Varios menores
- Declaraciones multi-variable enganosas (HECHO 2026-07-06): la linea `int8_t cantPresionadas, ...,
  F0Byte = 0;` ya se habia limpiado. Ahora tambien la linea `boolean clockCheck, ...` y la linea
  `long prevMillis, ...` (`Pepas.ino`) inicializan cada variable explicitamente (antes solo la ultima).
  Inofensivo (globales arrancan en 0) pero ya no es un trip-wire para futuros edits.
- `Pepa::triggerLoopCheck()` (HECHO 2026-07-06): documentado. Es el accesor publico de `triggerLoop()`
  (private), que loop() llama cada iteracion para cerrar el pulso de trigger al expirar su duracion.
- `insertarTap()` (HECHO 2026-07-06): tap tempo TERMINADO, montado sobre Esc (Esc reinicia cabezal
  Y tapea; un toque suelto solo reinicia). Se corrigieron dos bugs del stub: `dif` sin inicializar
  (acumulaba basura) y un `* 10` espurio (tempo 10x lento). Modelo alineado con el path de BPM:
  `velGen = capacidad/periodo_ms`, con guarda de division por cero y clamp a [1, 1024]*precision.
  `cantTaps` bajado de 16 a 4: solo se promedian los ultimos 4 taps (mover tempo en vivo) y se
  ahorra RAM. `TAP_TIMEOUT` (2s): si pasa mucho entre taps, el proximo Esc arranca serie nueva, asi
  un reinicio aislado no pisa el tempo. Con Ctrl apretado no se tapea (el pote maneja el tempo).
  FALTA PROBAR EN DISPOSITIVO.

## 7. Robustez (foco 2026-07-02)

Hecho 2026-07-02 (batch defensivo, sin cambio de comportamiento buscado, FALTA PROBAR EN DISPOSITIVO):
- `Pepa` inicializa `secuenciaCant` y `poteSnapshot` en el constructor. `new Pepa` no limpia la
  memoria; hoy funcionan solo porque la maquina de estados los escribe antes de leerlos, pero era
  un trip-wire para futuros edits.
- Cap del multiplicador: `controlarMult`/`controlarDiv` hacen `min(numero, 32)`. `numero` es uint8_t
  (llega a 255) y `multiplicadorTemporal` es int8_t: valores >127 se iban a negativo y corrompian
  la matematica de timing (y `24 / min(mult,12)` con mult negativo). El modo clock ya estaba
  guardado con `min(...)`; el free-run no.

Hecho 2026-07-02 (Tier-1 + feature de persistencia, FALTA PROBAR EN DISPOSITIVO):
- **Hardening de framing PS/2.** `ps2int_read` ahora valida paridad (impar) y stop bit; los frames
  corruptos (glitch de bus / transiente de hotplug) se descartan en vez de entrar como scancode
  fantasma. Antes se ignoraban ambos bits.
- **Recuperacion de hotplug.** El BAT (0xAA) del teclado al reconectarse setea un flag
  (`ps2HuboReset()`); `eventoTeclado()` lo consume y re-sincroniza los LEDs del canal con
  `actualizarLEDSelector()`. NO se toca el secuenciador: desenchufar no resetea nada y al
  reconectar se retoma como estaba (comportamiento pedido por el usuario).
- **Soft reset / panico** -> Ctrl+Alt+Supr (`softReset()`): silencia notas/gates trabados y suelta
  modificadores, sin tocar secuencias/tempo/params. `Pepa::silenciar()`.
- **Factory reset** -> Ctrl+Shift+Esc (`factoryReset()`): todo a valores de encendido. Se refactorizo
  el constructor de `Pepa` para compartir `Pepa::reset()` con el factory reset (quedan identicos).
- **Guardar/cargar estado en EEPROM** -> Ctrl+Shift+Enter (`guardarEstado()`), carga en `setup()`
  (`cargarEstado()`) y tambien en runtime con Ctrl+Shift+Backspace. Layout con magic para distinguir
  EEPROM en blanco; clamps de sanidad al cargar. `Pepa::guardarEEPROM/cargarEEPROM`. Ver
  `d_estado.ino`. El primer guardado bloquea ~1-2s (escribe ~630 B); los siguientes casi no, porque
  `EEPROM.update` solo escribe lo que cambia. En la carga por combo se re-ancla `poteSnapshotGral`
  porque Ctrl sigue apretado (si no, el loop pisaria el tempo cargado desde el pote).
  NOTA: guardado automatico al apagar necesita hardware (detector de brownout + cap reservorio).

Hecho 2026-07-02 (segunda pasada de robustez):
- **Watchdog timer** (`avr/wdt.h`). `wdt_enable(WDTO_4S)` al final de `setup()` (despues del boot de
  ~10s, si no lo dispararia), `wdt_reset()` al inicio de cada `loop()`. Si el loop se cuelga >4s, el
  micro se reinicia solo. `guardarEstado()` hace `wdt_reset()` entre voces para no dispararlo durante
  el guardado en EEPROM (~1-2s). VERIFICAR EN DISPOSITIVO que el boot completa y que el guardado no
  reinicia.
- **Overflow en la sync (backtick).** `bT = aT * aM` (y `* bM`) podia desbordar el `long` de 32 bits
  con multiplicadores grandes. Ahora se hace en `long long` y se normaliza con `%= capacidad` en vez
  del `while` (que con basura desbordada podia no converger).
- **#6 octava + shift** (abajo): resuelto. Con el macro BROADCAST, `subirOctava`/`bajarOctava` ahora
  se propagan a todas las voces con shift, consistente con el resto de los controles.

## 6. Bug conocido: octava + shift  (`b_eventoTeclado.ino`)  (HECHO 2026-07-02)
Con `shift`, `subirOctava`/`bajarOctava` (flechas) ahora se propagan a todas las voces (via
`BROADCAST`), consistente con el resto de los controles. Antes solo afectaban `pepas[selector]`.
