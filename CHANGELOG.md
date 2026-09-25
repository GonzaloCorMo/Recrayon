# Changelog

Cambios de cada versión de Recrayon. El proyecto sigue
[Semantic Versioning](https://semver.org/lang/es/).

## [Unreleased]

### Added
- **Plegar la barra** contra el borde más cercano: el botón de la barra que antes minimizaba
  ahora la pliega y deja una pestaña pequeña con una flecha que la devuelve. La pestaña se puede
  arrastrar como la barra. Una barra vertical se pliega a izquierda o derecha y una horizontal
  arriba o abajo; recuerda si la dejaste plegada. Minimizar a la barra de tareas sigue disponible
  en el menú de la bandeja. Útil sobre todo en Linux y macOS, donde la barra no se puede ocultar
  de las capturas.
- **Avisos al terminar**: la notificación de captura o grabación dice dónde se guardó (y que la
  captura está en el portapapeles) y, al pulsarla, abre la carpeta con el archivo seleccionado.
- **Mostrar la barra de herramientas en las capturas** y **en los vídeos**, por separado
  (Configuración → Captura; desmarcadas por defecto, como hasta ahora). Solo Windows puede dejar
  una ventana fuera de las capturas.

### Fixed
- Los menús de los botones con varias opciones podían quedar **detrás de la barra**: al abrirse
  cambiaba el foco y la barra se ponía delante de su propio menú.

### Changed
- Toda la app habla del **cursor** en vez del "puntero".
- Los botones con varias opciones (pizarra, captura, grabación) **abren su menú con el clic**.
  En Configuración → Barra de herramientas puedes volver al comportamiento anterior: el clic
  ejecuta el destino por defecto y el menú se abre con el clic derecho. El clic derecho abre el
  menú siempre, en los dos modos; y mientras se graba (o con la pizarra abierta), el clic para la
  grabación o cierra la pizarra en vez de abrir el menú.

## [1.0.0] - 2026-09-21

Primera versión estable.

### Dibujar

- Capa transparente sobre cada monitor con dos modos: **dibujo** (el ratón dibuja) e
  **interacción** (los dibujos siguen visibles y los clics llegan a las aplicaciones de debajo).
- Herramientas: lápiz, rotulador translúcido, flecha libre, línea, flecha, **flecha con texto**,
  rectángulo, elipse, **texto**, borrador y **mover**.
- 6 colores configurables más un selector libre, 4 grosores, deshacer / rehacer ilimitado y
  borrar todo.
- **Pizarra**: página en blanco en una pantalla o en todas, sin perder las anotaciones del
  escritorio.
- **Reproducir**: vuelve a dibujar toda la explicación desde cero, en el orden en que se hizo, a
  velocidad lenta, normal o rápida.
- **Volver al ratón** automáticamente tras colocar un elemento, configurable por herramienta.
- **Foco** y **halo** para resaltar el puntero.

### Capturar y grabar

- **Capturas** del monitor bajo el ratón, de todos los monitores o de un recuadro / ventana; se
  guardan y se copian al portapapeles.
- **Vídeo** MP4 del monitor, siguiendo al cursor entre pantallas, de todos los monitores (en uno
  o varios vídeos) o de un recuadro / ventana, con calidad *Máxima*, *Alta* o *Compacta* y
  **micrófono** opcional.
- Indicador de grabación con tiempo, **pausa** y parada.
- Cursor del ratón opcional en capturas y vídeos. En Windows, la toolbar y el indicador no salen
  en las capturas ni en los vídeos.
- Carpetas de destino configurables y acceso directo para abrirlas.

### Toolbar y configuración

- **Toolbar a tu medida**: qué elementos muestra, en qué orden, tamaño de los botones, vertical u
  horizontal y de 1 a 3 columnas o filas.
- Recuerda su posición, el grosor y la herramienta entre sesiones.
- **Minimizar**: Recrayon sigue en segundo plano y vuelve desde la barra de tareas o el icono de
  la bandeja.
- Atajos globales configurables (Windows).
- Fuente y tamaño del texto, paleta, color de la pizarra, velocidad de reproducción y más.
- Interfaz en **español e inglés**.

### Instalación

- Instaladores para **Windows** (`.exe`), **Debian / Ubuntu** (`.deb`), **cualquier Linux**
  (AppImage) y **macOS** (`.dmg`, Apple Silicon e Intel).
- Al instalar una versión nueva, el instalador de Windows detecta la anterior y ofrece
  actualizarla conservando los ajustes.
- Licencia CC BY-NC-ND 4.0; los avisos de terceros (Qt, FFmpeg) se incluyen en todos los
  paquetes.
