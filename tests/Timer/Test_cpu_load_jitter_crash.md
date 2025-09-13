# Error

Er trad bij test_cpu_load_jitter_crash een error op:

```text
-------------------------------------------------- test_cpu_load_jitter -------------------------------------------------- [jitter+load] dt=: 12019 assertion "error != osFlagsErrorResource" failed: file "../Libraries/CleanRTOS/src/crt_Task.h", line 89, function: void crt::Task::setEventBits(uint32_t)
```

# Opgelost mbv chatgpt

In FreeRTOSConfig aangepast:

```c
#define configUSE_TIMERS                     1
#define configTIMER_QUEUE_LENGTH             32      // bv. 32 voor 2ms periodiek
#define configTIMER_TASK_PRIORITY            (configMAX_PRIORITIES - 1)
#define configTIMER_TASK_STACK_DEPTH         (configMINIMAL_STACK_SIZE * 4)

// (voor de CMSIS wrapper – dan werkt osEventFlagsSet ook vanuit ISR)
#define configUSE_OS2_EVENTFLAGS_FROM_ISR    1
```

De oude, default waardes zijn:

```c
#define configUSE_TIMERS                     1
#define configTIMER_QUEUE_LENGTH             10      // bv. 32 voor 2ms periodiek
#define configTIMER_TASK_PRIORITY            (2)
#define configTIMER_TASK_STACK_DEPTH         256

// (voor de CMSIS wrapper – dan werkt osEventFlagsSet ook vanuit ISR)
#define configUSE_OS2_EVENTFLAGS_FROM_ISR    1
```

Dat blijkt te werken.

# Analyse door cgpt van waarom het werkt

(MAV is mijn paraaf, daar licht ik het aangegeven alternatief toe)

## Wat ging er mis (kort)

- Jouw **timer-ISR** zet via `Task::setEventBits(...)` een **event flag**.

- In FreeRTOS/CMSIS gebeurt dat in ISR via **`xEventGroupSetBitsFromISR`**, en **die plant altijd een “deferred call”** in de **Timer Service Task** (via `xTimerPendFunctionCallFromISR`).

- Als die Timer Service infrastructuur **uit staat** of **het pend-queue volloopt**, faalt de call:
  
  - `osEventFlagsSet(...)` (CMSIS) geeft **`osErrorResource`**, of
  
  - `xEventGroupSetBitsFromISR(...)` (jouw fallback) geeft **`pdFAIL`**.

- In jouw test “CPU load + jitter” (periodiek 2 ms) kwam dat snel naar boven → assert klapte.

## Wat heeft het opgelost

Je hebt in `FreeRTOSConfig.h` de Timer Service “goed” aangezet en ruimer gemaakt:

- `configUSE_TIMERS = 1` (Timer Service Task aan)

- **Grotere queue**: `configTIMER_QUEUE_LENGTH` (bijv. 32)

- **Hogere prioriteit** voor de Timer Service Task: `configTIMER_TASK_PRIORITY` (je zette ‘m hoog)

- (Daarnaast) `configUSE_OS2_EVENTFLAGS_FROM_ISR = 1` voor de CMSIS-weg

Gevolg: `xEventGroupSetBitsFromISR` kan z’n deferred werk kwijt, en jouw eventbits vanuit de ISR falen niet meer. De test loopt nu door.

## Mogelijke nadelen / aandachtspunten van de wijziging

- **Hoge prioriteit Timer Service Task**
  
  - Als je ‘m op `(configMAX_PRIORITIES-1)` zet en je hebt ook eigen “kritische” taken, kan de Timer Task die **preëmpten**.
  
  - Richtlijn: geef ’m **net boven** de prio van “gewone” taken, maar **onder** je echte real-time taken (motor control e.d.). Test dit met je systeembelasting.

- **Grotere queue = extra RAM**
  
  - `configTIMER_QUEUE_LENGTH` bepaalt de command-queue van de Timer Task. Groter is robuuster tegen bursts, maar kost RAM.

- **Meer verkeer via de Timer Service**
  
  - Elke ISR-eventbit gaat via de Timer Task. Bij hoge frequentie of veel bronnen kan dat **jitter** of **queue pressure** geven.
  
  - Alternatief (heb je al in huis): routeer desnoods **alle** timer-events via je **Relay** (ISR → queue → task zet bit). Dat omzeilt de Timer Service volledig, met een paar µs extra latency.

**MAV**: Met dat alternatief bedoelt hij dus: ook "non-longTimer" events via de Relay laten lopen. Als je in de ISR immers de relay aanroept, worden geen eventbits maar een queue gebruikt. En queues gebruiken weer niet de Timer Service van FreeRTOS. Alleen seteventbits aangeroepen vanuit een ISR gebruikt de Timer Service van FreeRTOS.
Zo voorkom je dus het gebruik van de Timer Service in zijn geheel, maar wel ten koste van een extra indirectie voor korte timers.
... en verder werkt het niet met CleanRTOS Queue as is:momenteel doet CleanRTOS Queue ook een setEventBits tbv waitall/waitany. Daar zal dan dus weer een workaround voor bedacht moeten worden.

Enfin - mits de prioriteiten van de tasks gelijk worden genomen, lijkt alles via de Relay laten lopen ook de jitter-crash te kunnen voorkomen, zonder dat daar de aanpassingen in de freertos config voor nodig zijn. Voor nu pas ik dat dus toe.

Maar goed om van dit alles bewust te zijn vandaar deze .md file erbij.