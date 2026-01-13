# ＜dis-tone＞
This is a decompilation of PxtoneDLL, a legacy playback file used to playback .PTCOP and .PTTUNE files.
Library is originally developed by **Daisuke Amaya** - Pixel [ぴ]: https://pxtone.org/developer/

Based in v0.9.1.5, with the help from public 220910a source code.

## ＜What is this?＞
This is sample source code to play song files created with Pxtone Collage (also known as PisColla[ピスコラ]) on Windows.

## ＜pxtone＞
You can create waveform data right from song data made with Pxtone.
This waveform data then can be saved as a WAV file, or output through an audio buffer to be used as BGM in games and other applications.

Audio files can also be created with ptNoise which can be used as sound effects in games.

## ＜How to get and use ogg/vorbis＞
To play songs using OGG files, download Ogg Vorbis library ([Vorbis.com](http://www.vorbis.com/)), compile them as **lib** files and then include it with the project.
If you don't plan in using OGG, go to "Classes/pxtone/pxStdDef.h", and comment "#define pxINCLUDE_OGGVORBIS 1".
