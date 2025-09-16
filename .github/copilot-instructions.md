# Copilot Instructions for Meshtastic Firmware

## Arquitectura general

- Firmware multiplataforma para dispositivos LoRa (ESP32, nRF52, RP2040, Linux).
- Estructura modular: cada plataforma tiene su propio subdirectorio bajo `arch/`.
- El núcleo de la lógica de red, mensajes y configuración está en `src/`.
- Scripts de construcción y utilidades en `bin/` y `extra_scripts/`.
- Configuración de compilación y targets en `platformio.ini`.

## Flujos de trabajo de desarrollo

- **Compilación:** Usa PlatformIO (`PlatformIO: Build` en VS Code o `pio run`).
- **Flasheo:** Sigue la [guía oficial](https://meshtastic.org/docs/getting-started/flashing-firmware/) o ejecuta los scripts en `bin/` (`device-install.bat`, `.sh`, `.ps1`).
- **Pruebas:** Los tests están en `test/` y pueden requerir hardware específico.
- **Actualización de protos:** Usa `bin/regen-protos.sh` o `.bat` para regenerar archivos protobuf.

## Convenciones y patrones

- **Soporte multiplataforma:** Cada arquitectura tiene scripts y configuraciones dedicadas.
- **Tablas de partición:** Archivos CSV en la raíz para diferentes tamaños de memoria.
- **Versionado:** Usa `version.properties` y scripts en `bin/` para gestión de versiones.
- **Configuración de usuario:** `userPrefs.jsonc` y scripts asociados para generación y validación.
- **Integración continua:** Workflows GitHub Actions definidos en `.github/workflows/` (ver repositorio principal si no existen localmente).

## Dependencias y puntos de integración

- **PlatformIO:** Principal sistema de build y gestión de dependencias.
- **Protobuf:** Usado para la definición de mensajes de red.
- **Scripts auxiliares:** Bash, Python y PowerShell para automatización.
- **Soporte Docker:** Archivos `Dockerfile` y `docker-compose.yml` para entornos reproducibles.

## Ejemplos de comandos útiles

- Compilar firmware: `pio run`
- Flashear dispositivo: `bin/device-install.bat` (Windows) o `bin/device-install.sh` (Linux/Mac)
- Regenerar protos: `bin/regen-protos.sh`
- Bump de versión: `bin/bump_version.py`

## Archivos/directorios clave

- `arch/` – Código específico por plataforma
- `src/` – Lógica principal del firmware
- `bin/` – Scripts de utilidad y automatización
- `platformio.ini` – Configuración de PlatformIO
- `test/` – Pruebas automatizadas
- `userPrefs.jsonc` – Configuración de usuario

## Notas adicionales

- Consulta la [documentación oficial](https://meshtastic.org/docs/) para detalles avanzados.
- Sigue los scripts y convenciones existentes para asegurar compatibilidad multiplataforma.
- Participa en la comunidad para soporte y contribuciones.
- Los comentario en el codigo están en inglés, mantén la coherencia.
- Estamos trabajando en el menu wifi y en el historial de mensajes en pantalla con los chats de node/channel.
- El proyecto está en constante evolución, revisa los cambios recientes en el repositorio para estar al día.
- Si tienes dudas sobre la estructura o el flujo de trabajo, revisa issues y pull requests previos en GitHub para contexto adicional.
- El proyecto esta orientado a pull requests
- No borramos para insertar el codigo nuevo, no movemos el codigo, solo insertamos.
- Traduccion de los comentarios que hay en castellano al ingles.
