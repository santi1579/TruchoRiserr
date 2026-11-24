# TruchoRiser

Vocal riser (reverse + reverb + filtro ascendente) con interfaz azul. Formatos: AU, VST3 y Standalone.

## Compilar

Necesitas CMake (>= 3.22) y git.
- Mac: Xcode Command Line Tools (`xcode-select --install`) y `brew install cmake`
- Windows: Visual Studio 2022 (Desktop C++) y CMake

```
cd TruchoRiser
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

La primera vez descarga JUCE (internet). Al terminar, el plugin se copia solo a:
- Mac: ~/Library/Audio/Plug-Ins/Components (AU, para Logic) y ~/Library/Audio/Plug-Ins/VST3
- Windows: C:\Program Files\Common Files\VST3

En Logic: Ajustes > Plug-ins > "Reiniciar y reanalizar" si no aparece. Busca "Trucho > TruchoRiser".

## Uso
- Knob grande: intensidad del riser (volumen, reverb y barrido del filtro).
- < > : cambia el tipo (1/4, 1/2, 1, 2, 4 compases), sincronizado al tempo del DAW.
- RAW / FX: RAW = voz sin efecto, FX = voz + riser.
- LISTEN / LOOP: reproduce un riser de prueba (ruido) para escuchar el ajuste sin voz.
- PHONE / CHORUS / TREMOLO: efectos sobre el riser.
