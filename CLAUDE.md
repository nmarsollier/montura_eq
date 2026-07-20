# Reglas de Desarrollo del Proyecto Montura

Mantener este archivo en formato simple, para que pueda leerse y editarse rapidamente

## Definicion del proyecto

- Logica para manejar montura ecuatorial DIY simple
- Utiliza un modulo Hosyond ESP32-S3 Development Board N16R8 con ESP32-S3-WROOM-1, WiFi y Bluetooth Dual-Mode,
  USB-C (ver seccion "Placa ESP32-S3" abajo para especificaciones completas y pinout)
- Utiliza 2 TMC2209 de BT https://global.bttwiki.com/TMC2209.html
- Utiliza 2 motores paso a paso Nema17 con las siguientes caracteristicas: Low Noise:15N.cm(21oz.in) holding torque,
  drive voltage 12V/ 24V, rated current 1.4A, resistance 3.5ohms
- Utiliza 2 poleas 20 dientes en el motor, 80 dientes en el eje de rotacion
- La montura posee 2 botones fisicos, Stop y Home
- Posee una pantalla minimalista OLED de 0.98 inch, se muestra verticalmente

## Placa ESP32-S3

### Identificacion

- **Modelo**: Hosyond 3Pack ESP32-S3 Development Board N16R8
- **SoC**: ESP32-S3-WROOM-1 (Xtensa 32-bit LX7, doble nucleo)
- **Chip USB-Serial**: Integrado en el SoC (USB Serial/JTAG nativo)
- **Factor de forma**: Compatible con protoboard estandar
- **Flash**: 16 MB (N16)
- **PSRAM**: 8 MB (R8, octal)
- **Antena**: Integrada en el modulo WROOM-1

### Especificaciones tecnicas

- **CPU**: Tensilica Xtensa 32-bit LX7, doble nucleo, hasta 240 MHz
- **WiFi**: 802.11 b/g/n (2.4 GHz)
- **Bluetooth**: v5.0 BLE + Bluetooth Mesh
- **Alimentacion**: 5V via USB-C (regulador 3.3V onboard)
- **Logica I/O**: 3.3V (no tolera 5V)
- **GPIO digitales**: 45 (configurables)
- **ADC**: 2 conversores SAR ADC de 12 bits, hasta 20 canales
- **UART**: 3 controladores UART
- **I2C**: 2 controladores I2C
- **SPI**: 4 controladores SPI
- **I2S**: 2 controladores I2S
- **PWM**: 8 canales LEDC independientes
- **Sensores capacitivos touch**: 14 pines
- **USB**: USB OTG 1.1 (nativo, sin chip externo)

### Memorias

- **ROM**: 384 KB
- **SRAM**: 512 KB
- **Flash SPI externa**: 16 MB (Quad SPI, modulo WROOM-1)
- **PSRAM**: 8 MB (octal SPI, modulo WROOM-1)

### Seguridad hardware

- Estandares IEEE 802.11: WFA, WPA/WPA2, WPA3
- Arranque seguro (Secure Boot v2)
- Aceleracion criptografica por hardware: AES-128/256, SHA, RSA, ECC, HMAC
- Flash Encryption (XTS-AES-128)
- Firma digital para firmware

### Layout de la placa con conexiones

```
LADO IZQUIERDO                LADO DERECHO
(USB-C abajo)                 (USB-C abajo)

3V3  ← TMC VIO (3.3V)         GND  ← TMC GND
EN   (reset)                  G46  (strapping, no tocar)
G1   (ADC, libre)             G0   (BOOT, no tocar)
G2   (ADC, libre)             G35  (libre)
G3   (ADC, strapping)         G36  (libre)
G4   ← LED externo            G37  (libre)
G5   (libre)                  G38  (libre)
G6   (libre)                  G39  (libre)
G7   ← DEC DIR                G40  (libre)
G8   (libre)                  G41  (libre)
G9   ← RA DIR                 G42  (libre)
G10  ← RA STEP                G43  (U0TXD, consola debug, no tocar)
G11  (libre)                  G44  (U0RXD, consola debug, no tocar)
G12  (libre)                  G45  (strapping, no tocar)
G13  (libre)                  G47  (libre)
G14  ← ENABLE                 G48  (libre)
G15  ← DEC STEP               GND  ← 12V GND (fuente ext)
G16  ← STOP button            G17  ← TMC TX (UART, con 1k en serie)
G18  ← TMC RX (UART)          V5   (5V USB, no usar)
G21  ← HOME button
```

### Pinout definitivo de Montura

| Label placa | GPIO | Funcion           | Modulo        | Notas                                 |
|-------------|------|-------------------|---------------|---------------------------------------|
| 3V3         | —    | TMC VIO / MS1/MS2 | tmc           | Alimentacion logica 3.3V              |
| GND         | —    | TMC GND           | tmc           | Tierra comun                          |
| GND         | —    | 12V GND externo   | motors        | Tierra de fuente de motores           |
| G4          | 4    | LED externo       | led           | LEDC PWM, indicador de estado         |
| G7          | 7    | DEC DIR           | motors_motion | Direccion eje declinacion             |
| G9          | 9    | RA DIR            | motors_motion | Direccion eje ascension recta         |
| G10         | 10   | RA STEP           | motors_motion | Pulso STEP eje ascension recta        |
| G14         | 14   | MOTORS ENABLE     | motors_motion | Enable global compartido RA y DEC     |
| G15         | 15   | DEC STEP          | motors_motion | Pulso STEP eje declinacion            |
| G16         | 16   | STOP button       | —             | Input con pull-up interno             |
| G17         | 17   | TMC UART TX       | tmc           | UART, con resistencia 1k en serie     |
| G18         | 18   | TMC UART RX       | tmc           | UART, single-wire bus                 |
| G21         | 21   | HOME button       | —             | Input con pull-up interno             |

**Uso futuro:**
| GPIO | Funcion     | Notas                              |
|------|-------------|------------------------------------|
| 5    | I2C SDA     | Sensor / display                   |
| 6    | I2C SCL     | Sensor / display                   |
| 1    | Hall limit  | Sensor de fin de carrera           |
| 2    | Hall limit  | Sensor de fin de carrera           |

### Pines con restricciones (NO USAR)

| GPIO | Restriccion                                 |
|------|---------------------------------------------|
| 0    | Boot: LOW en reset = modo flash (ROM boot)  |
| 3    | Strapping JTAG (LOW al boot)                |
| 43   | UART0 TX, consola debug (USB-Serial nativo) |
| 44   | UART0 RX, consola debug (USB-Serial nativo) |
| 45   | Strapping (VDD_SPI voltage)                 |
| 46   | Strapping (LOW al boot = log normal)        |

### Notas de desarrollo para esta placa

- El ESP32-S3 tiene USB-Serial/JTAG nativo integrado en el SoC. No requiere chip externo CP2102.
  En macOS los drivers son nativos. Para flashear y monitorear se usa el mismo puerto USB-C.
- La placa usa USB-C para alimentacion y programacion.
- Para flashear: `idf.py flash` usa el USB-Serial/JTAG nativo automaticamente. No es necesario
  manipular pines BOOT.
- El LED externo esta en GPIO 4, controlado via LEDC PWM. La placa tambien tiene un LED onboard
  (generalmente en GPIO 48 o similar, no usado en este proyecto).
- La antena en PCB del modulo WROOM-1 ofrece buen rendimiento, pero si la montura tiene partes
  metalicas cercanas, considerar orientacion.
- El regulador de voltaje onboard convierte 5V de USB a 3.3V para el ESP32-S3 y perifericos.
  La corriente maxima disponible en el pin 3V3 depende del regulador (tipicamente ~500-800 mA).
- El ESP32-S3 soporta PSRAM octal de 8 MB. Se puede habilitar en menuconfig para buffers grandes.
- La N16R8 tiene 16 MB de flash, suficiente para OTA y multiples particiones de firmware.

## Arquitectura

Capas del sistema, de afuera hacia adentro:

```
Cliente Web (Alpine.js) → REST API → Mount (orquestacion) → Motors → TMC2209 (hardware)
```

- **www/** — UI Web embebida programada con Alpine.js. Compila con `node www/build.js`, genera `www/dist/`.
- **REST API** (`main/rest/`) — Expone endpoints HTTP para control de la montura.
- **Mount** (`main/mount/`) — Orquestacion logica del montaje: estado, coordenadas, sincronizacion.
- **Runtime** (`main/runtime/`) — Inicializacion y ciclo de vida del sistema.
- **Motors** (`main/motors/`) — Control de motores de alto nivel.
- **Motors Motion** (`main/motors_motion/`) — Ejecucion hardware: generacion de pulsos STEP/DIR.
- **TMC** (`main/tmc/`) — Driver TMC2209 via UART. Unica fuente de verdad para configuracion de microsteps.
- **LED** (`main/led/`) — Control PWM del LED externo en GPIO 4. Estados: tenue (normal), brillante (slewing), respiracion (error).
- **Network** (`main/network/`) — Conectividad WiFi.
- **USB Net** (`main/usb_net/`) — Interfaz de red USB Ethernet via TinyUSB en modo ECM/RNDIS. Permite conectar
  la montura a una computadora por USB-C y acceder a la API REST y Alpaca sin WiFi. IP estatica del ESP32-S3:
  192.168.7.1, DHCP server para el host.
- **Tools** (`main/tools/`) — Utilidades transversales (parser, validacion).

## Reglas generales

- Nunca hacer commit o push a github sin autorizacion explicita
- Commits solo cuando la feature completa este verificada y compilando
- Siempre basarse en codigo desde el disco como unica fuente de verdad. El codigo fuente es la unica referencia valida
  para entender detalles de implementacion
- Antes de cualquier cambio leer el estado actual del archivo a modificar
- Proyecto programado en C, siguiendo estilo funcional cuando sea posible
- Modulos separados por dominio, cada dominio en una carpeta dentro de `main/`
- Cada dominio expone:
    - `dominio.h` — API publica (funciones que otros modulos pueden llamar)
    - `dominio_internal.h` — API interna (funciones compartidas solo entre archivos del mismo modulo)
- Un archivo `.c` por caso de uso dentro de cada modulo
- Las funciones publicas comienzan con el nombre del modulo, seguido de `_`
- Las variables de estado de un modulo nunca se acceden directamente desde afuera. Solo a traves de funciones publicas
  del modulo
- Respetar principios: DRY - YAGNI - KISS
- La documentacion en headers describe el problema de negocio o caso de uso que resuelve la funcion, no los detalles de
  implementacion
- Solo comentar codigo si es complejo de comprender para un humano
- El codigo y sus comentarios dentro de archivos con extension .c y .h se escriben en ingles
- Archivos markdown .md se escriben en idioma español
- Los tags de comentarios deben definirse como constante y su valor es el mismo nombre del archivo en mayuscular y sin
  la extension .c

## Convenciones de Codigo

- **Lenguaje**: C (no C++)
- **snake_case** para funciones y variables
- **UPPER_CASE** para macros y defines
- **Headers**: `#pragma once`, includes organizados: primeros los propios del modulo, luego librerias del framework
- **Logging**: usar `ESP_LOGI()`, `ESP_LOGW()`, `ESP_LOGE()` con tag estatico por archivo
- **Resultados entre capas**: usar los tipos `MotorResultCode` y `MountResult` definidos en el proyecto

## Relaciones entre modulos (dependencias)

- Network es autocontenido y expone WiFi STA + AP fallback
- USB Net depende de TinyUSB (componente gestionado `espressif/esp_tinyusb`) y esp_netif
- REST API y Alpaca se enlazan a INADDR_ANY, accesibles tanto por WiFi como por USB Net
