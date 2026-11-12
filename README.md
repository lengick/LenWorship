# Len-Worship

Sintetizador **VST3** de piano oscuro estilo worship con capa de pads atmosféricos
(reverb enorme tipo "supermassive"), interfaz negra con luces neón.

## Perillas

| Panel   | Perilla | Qué hace |
|---------|---------|----------|
| LAYERS  | PIANO   | Nivel de la capa de piano oscuro |
|         | PAD     | Nivel de la capa de pads (ataque y release lentos) |
| FILTER  | CUTOFF  | Filtro pasa-bajos general (40 Hz – 20 kHz) |
| DELAY   | DELAY   | Cantidad de delay ping-pong (cada eco suena más oscuro) |
|         | TIME    | Tiempo del delay (50 – 1200 ms) |
| REVERB  | REVERB  | Cantidad de reverb |
|         | SIZE    | Tamaño: de ~1 s hasta colas de ~15 s |
| MASTER  | VOLUME  | Volumen de salida |

Responde a velocidad (más fuerte = más brillo), pedal de sustain y 24 voces de polifonía.

## Cómo obtener el .vst3 para Windows

### Opción A — sin instalar nada (GitHub Actions)

1. Crea una cuenta gratis en https://github.com y un repositorio nuevo.
2. Sube **todo el contenido de esta carpeta** al repositorio (incluida la carpeta oculta `.github`).
3. Entra a la pestaña **Actions** del repositorio. La compilación empieza sola (tarda ~10 minutos).
4. Cuando termine, abre la ejecución y descarga **Len-Worship-VST3-Windows** (sección *Artifacts*).

### Opción B — compilar en tu PC

Necesitas Visual Studio 2022 (carga de trabajo "Desarrollo para el escritorio con C++"),
CMake y Git. Luego haz doble clic en `build_windows.bat`.

## Instalar en FL Studio

1. Copia la carpeta **`Len-Worship.vst3`** a `C:\Program Files\Common Files\VST3`.
2. En FL Studio: *Options → Manage plugins → Find installed plugins* (o *Start scan*).
3. Abre un canal nuevo: *Add → Len-Worship* (aparece como instrumento).

Funciona igual en Ableton Live, Reaper, Cubase, Studio One, Bitwig, etc.

> **Sobre Kontakt:** Kontakt es un sampler que carga sus propias librerías (NKI);
> no puede cargar plugins VST de terceros dentro de él. Len-Worship se usa
> directamente en tu DAW, y Kontakt puede convivir en otra pista.

## Estructura del código

```
Source/
  PluginProcessor.*   parámetros, cadena de audio (filtro → delay → reverb → volumen)
  Voices.h            voz de piano oscuro + voz de pad
  FxChain.h           delay ping-pong y reverb FDN (sin dependencias de JUCE)
  PluginEditor.*      interfaz
  NeonLookAndFeel.h   perillas con anillo de neón
```

Hecho con [JUCE](https://juce.com) 8. Ojo con la licencia: JUCE es gratis para uso
personal/open source, pero si vas a **vender o distribuir** el plugin necesitas
revisar sus condiciones de licencia en juce.com/legal.
