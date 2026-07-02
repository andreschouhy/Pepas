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

## 2. Deduplicar el patron broadcast del dispatcher  (`b_eventoTeclado.ino`)  (PENDIENTE)
El bloque `if(shift) for(...) pepas[i]->X(arg); else pepas[selector]->X(arg);` se repite ~18 veces.
Reemplazar por un macro:
```cpp
#define BROADCAST(call) do { \
  if (shift) for (uint8_t i = 0; i < cantPepas; i++) pepas[i]->call; \
  else pepas[selector]->call; \
} while(0)
// uso: BROADCAST(controlarCTRL(0));
```
Tambien: el bloque del selector->LED en TAB son cuatro `if(selector==N)` casi identicos
(`b_eventoTeclado.ino`, presionando 0x0D); se puede reemplazar por una pequena tabla de bits.

## 3. Nombrar los scancodes  (`b_eventoTeclado.ino`)
Los scancodes son numeros magicos (`0x14`, `0x11`, `0x05`, `0x29`, `0x58`...). Los comentarios al
lado ayudan pero `#define SC_LCTRL 0x14` etc. harian el dispatcher autoexplicativo.

## 4. Largos de array por `sizeof`  (`Pepas.ino`)  (HECHO 2026-07-02)
`K2Midi` y `K2Num` ahora iteran con `sizeof(mapa)/sizeof(mapa[0])` y
`sizeof(mapaNum)/sizeof(mapaNum[0])`, asi el limite no se desincroniza de los datos si se
agregan/quitan filas.

## 5. Varios menores
- Declaraciones multi-variable enganosas: la linea `int8_t cantPresionadas, ..., F0Byte = 0;` ya se
  limpio (se quitaron `pausa`/`E0Key`/`F0Byte`, y `cantPresionadas`/`notasPresionadas` se inicializan).
  Queda la linea `boolean clockCheck, clockSwitch, controlarVelocidad, setTempo = 0;` (`Pepas.ino`):
  solo inicializa `setTempo`. Inofensivo (globales arrancan en 0) pero conviene separar/inicializar.
- `Pepa::triggerLoopCheck()` es un wrapper de paso directo a `triggerLoop()` privado; documentar por
  que existe o exponer `triggerLoop`.
- `insertarTap()` (HECHO 2026-06-30): desconectada de Shift+ESC. Sigue definida en `Pepas.ino` sin
  usar. Decidir si terminarla (tap tempo) o borrarla.

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

Pendiente:
- **Watchdog timer** (`avr/wdt.h`). Auto-reset si el sketch se cuelga. Cuidado: el boot animation
  hace `setup()` de ~10s, asi que habilitar el WDT DESPUES de setup y `wdt_reset()` en cada `loop()`.
  Cambio aislado para verificar boot en el dispositivo con limpieza.

## 6. Bug conocido a investigar: octava + shift  (`b_eventoTeclado.ino`)
Con `shift` activo, `subirOctava`/`bajarOctava` (flechas) siguen actuando solo sobre `pepas[selector]`,
no se propagan a todas. La reescritura mantuvo ese comportamiento a proposito (fix fiel). Revisar si
deberia ser broadcast como el resto de los controles.
