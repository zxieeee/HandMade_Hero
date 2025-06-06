#pragma once
#include <cmath>
#include <stdint.h>
#define internal static
#define local_persist static
#define global_variable static
#define Pi32 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;

struct game_offscreen_buffer {
  // NOTE: Pixels are always 32-bits wide, Memory Order BB GG RR XX

  // BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel = 4;
};

struct game_sound_output_buffer {
  int SamplesPerSecond;
  int SampleCount;
  int16 *Samples;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset,
                                  int YOffset,
                                  game_sound_output_buffer *SoundBuffer);
