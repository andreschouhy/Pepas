# Tareas de limpieza pendientes

Registro de mejoras de estructura/limpieza identificadas el 2026-06-24 pero **no aplicadas todavia**.
Ninguna es urgente; el sketch compila y anda. Son refactors de codigo, sin cambio de comportamiento
buscado (salvo donde se indique). Conviene hacerlas de a una y verificarlas en el dispositivo por separado.

Ya hecho en esta pasada (no rehacer): borrado de codigo muerto (`state`, `fromHex`, `ledPin`,
`tamanoEscalaLimite`, `_pote`, `atualizarLED`, rama vacia `control==1`) y guardas de limites en
`Pepa::agregar()` y `Pepa::quitar()` (esta ultima arregla el desborde de `escalaSize` al soltar notas).

---

## 1. Extraer el driver PS/2 a su propio modulo  (la grande)
`Pepas.ino` hace 4 cosas a la vez: globales, driver PS/2, config de PWM y helpers de la app.
El driver PS/2 (~150 lineas) es autocontenido y no depende de la logica de Pepas:
- ISRs `ps2int_read` / `ps2int_write`
- ring buffer (`buffer`, `head`, `tail`, `ps2Available`, `ps2Read`, `ps2Write`)
- helpers open-collector (`holdClock`/`releaseClock`/`holdData`/`releaseData`)
- `BUFFER_SIZE`, `inhibiting`, pines `DataPin`/`ClockPin`/`CLOCK_PIN_INT`

Mover a `ps2.h`/`ps2.cpp` (o como minimo a un `d_ps2.ino`). Deja `Pepas.ino` como "glue" de la app.
Cuidado: las variables son `volatile` y compartidas con las ISR; mantener `volatile` al mover.

## 2. Deduplicar el patron broadcast del dispatcher  (`b_eventoTeclado.ino`)
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

## 4. Largos de array por `sizeof`  (`Pepas.ino`)
`K2Midi` itera `i < 34` y `K2Num` itera `i < 10` hardcodeado. Usar
`sizeof(mapa)/sizeof(mapa[0])` y `sizeof(mapaNum)/sizeof(mapaNum[0])` para que el limite no se
desincronice de los datos si se agregan/quitan filas.

## 5. Varios menores
- Declaraciones multi-variable enganosas: `int8_t cantPresionadas, ..., F0Byte = 0;` (`Pepas.ino`)
  solo inicializa `F0Byte`. Es inofensivo (globales arrancan en 0) pero confunde; idem la linea de
  `boolean clockCheck, ...`. Inicializar cada una o separar.
- `Pepa::triggerLoopCheck()` es un wrapper de paso directo a `triggerLoop()` privado; documentar por
  que existe o exponer `triggerLoop`.
- `insertarTap()` esta marcada "no funcional" pero esta cableada a Shift+ESC (`b_eventoTeclado.ino`).
  Decidir: terminarla o desconectarla para que no dispare comportamiento a medias.
