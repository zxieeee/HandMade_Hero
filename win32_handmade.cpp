#include "handmade.cpp"
#include "handmade.h"
#include <cstdint>
#include <dsound.h>
#include <profileapi.h>
#include <windows.h>
#include <xinput.h>

///////////////////////////////////////////////////////////////
///  Loading Libraries
//////////////////////////////////////////////////////////////

global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

//------------------------------XInput-------------------------//
#define X_INPUT_GET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void) {
  HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (XInputLibrary) {
    XInputGetState =
        (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState =
        (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
  }
}

//------------------------------Direct Sound-----------------------//

#define DIRECT_SOUND_CREATE(name)                                              \
  HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS,               \
                      LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond,
                              int32 BufferSize) {
  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
  if (DSoundLibrary) {
    direct_sound_create *DirectSoundCreate =
        (direct_sound_create *)GetProcAddress(DSoundLibrary,
                                              "DirectSoundCreate");
    LPDIRECTSOUND DirectSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {

      WAVEFORMATEX WaveFormat = {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 2;
      WaveFormat.nSamplesPerSec = SamplesPerSecond;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nBlockAlign =
          (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
      WaveFormat.nAvgBytesPerSec =
          WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
      WaveFormat.cbSize = 0;

      if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
        DSBUFFERDESC BufferDescription = {};
        BufferDescription.dwSize = sizeof(BufferDescription);
        BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

        LPDIRECTSOUNDBUFFER PrimaryBuffer;

        if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription,
                                                     &PrimaryBuffer, 0))) {
          HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
          if (SUCCEEDED(Error)) {
            OutputDebugStringA("PrimaryBuffer format was set");
          } else {
          }
        } else {
        }
      } else {
      }
      DSBUFFERDESC BufferDescription = {};
      BufferDescription.dwSize = sizeof(BufferDescription);
      BufferDescription.dwFlags = 0;
      BufferDescription.dwBufferBytes = BufferSize;
      BufferDescription.lpwfxFormat = &WaveFormat;

      if (SUCCEEDED(DirectSound->CreateSoundBuffer(
              &BufferDescription, &GlobalSecondaryBuffer, 0))) {
      } else {
      }
    } else {
    }
  } else {
  }
}

///////////////////////////////////////////////////////////////
/// Structs
//////////////////////////////////////////////////////////////

struct win32_offscreen_buffer {
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel = 4;
};
struct win32_sound_output {
  uint32 RunningSampleIndex;
  int SamplesPerSecond;
  int ToneVolume;
  int ToneHz;
  int BytesPerSample;
  int WavePeriod;
  int SecondaryBufferSize;
};

struct win32_window_dimension {
  int Width;
  int Height;
};

internal win32_window_dimension Win32GetWindowDimension(HWND Window) {
  win32_window_dimension Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return (Result);
}

///////////////////////////////////////////////////////////////
/// Variables
//////////////////////////////////////////////////////////////

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

///////////////////////////////////////////////////////////////
/// Functions
///////////////////////////////////////////////////////////////

internal void Win32ClearBuffer(win32_sound_output *SoundOutput) {

  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;
  if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1,
                                            &Region1Size, &Region2,
                                            &Region2Size, 0))) {

    uint8 *DestSample = (uint8 *)Region1;
    for (DWORD ByteIndex = 0; ByteIndex < Region1SampleCount; ByteIndex++) {
      *DestSample++ = 0;
    }
    DestSample = (uint8 *)Region2;
    for (DWORD ByteIndex = 0; ByteIndex < Region2SampleCount; ByteIndex++) {
      *DestSample++ = 0;
      GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
  }
}
internal void Win32FillSoundBuffer(win32_sound_output *SoundOutput,
                                   DWORD ByteToLock, DWORD BytesToWrite,
                                   game_sound_output_buffer *SourceBuffer) {
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;

  if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1,
                                            &Region1Size, &Region2,
                                            &Region2Size, 0))) {

    int16 *DestSample = (int16 *)Region1;
    DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
    int *SourceSample = SourceBuffer->Samples;

    for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount;
         SampleIndex++) {
      real32 t = 2.0f * Pi32 * (real32)SoundOutput->RunningSampleIndex /
                 (real32)SoundOutput->WavePeriod;
      real32 SineValue = sinf(t);
      int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
      *DestSample++ = *SourceSample++;
      *DestSample++ = *SourceSample++;
      ++SoundOutput->RunningSampleIndex;
    }

    DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
    DestSample = (int16 *)Region2;
    for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount;
         SampleIndex++) {
      real32 t = 2.0f * Pi32 * (real32)SoundOutput->RunningSampleIndex /
                 (real32)SoundOutput->WavePeriod;
      real32 SineValue = sinf(t);
      int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
      *DestSample++ = *SourceSample++;
      *DestSample++ = *SourceSample++;
      ++SoundOutput->RunningSampleIndex;
    }
    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
  }
}
///////////////////////////////////////////////////////////////
/// Win32ResizeDIBSection
///////////////////////////////////////////////////////////////

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width,
                                    int Height) {

  if (Buffer->Memory) {
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }
  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize =
      Buffer->BytesPerPixel * (Buffer->Width * Buffer->Height);
  Buffer->Memory =
      VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
  Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                                         HDC DeviceContext, int WindowWidth,
                                         int WindowHeight) {

  StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0,
                Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info,
                DIB_RGB_COLORS, SRCCOPY);
}

///////////////////////////////////////////////////////////////
/// Win32MainWindowCallback
/////////////////////////////////////////////////////////////

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,

                                         WPARAM WParam, LPARAM LParam) {
  LRESULT Result = 0;

  switch (Message) {
  case WM_SIZE: {
    OutputDebugStringA("WM_SIZE\n");
  } break;

  case WM_DESTROY: {
    Running = false;
    OutputDebugStringA("WM_DESTROY\n");
  } break;

  case WM_CLOSE: {
    Running = false;
    OutputDebugStringA("WM_CLOSE n");
  } break;

  case WM_ACTIVATEAPP: {
    OutputDebugStringA("WM_ACTIVATEAPP\n");
  } break;

  case WM_SYSKEYDOWN:

  case WM_SYSKEYUP:

  case WM_KEYDOWN:

  case WM_KEYUP: {
    uint32 VKCode = WParam;
    bool WasDown = ((LParam & (1 << 30)) != 0);
    bool IsDown = ((LParam & (1 << 31)) == 0);
    if (WasDown != IsDown) {
      if (VKCode == 'W') {

      } else if (VKCode == 'A') {

      } else if (VKCode == 'S') {

      } else if (VKCode == 'D') {

      } else if (VKCode == 'Q') {

      } else if (VKCode == 'E') {

      } else if (VKCode == VK_UP) {

      } else if (VKCode == VK_LEFT) {

      } else if (VKCode == VK_DOWN) {

      } else if (VKCode == VK_RIGHT) {

      } else if (VKCode == VK_ESCAPE) {

      } else if (VKCode == VK_SPACE) {
      }
      bool AltKeyWasDown = ((LParam & (1 << 29)) != 0);
      if ((VKCode == VK_F4) && AltKeyWasDown) {
        Running = false;
      }
    }
  }
  case WM_PAINT: {
    PAINTSTRUCT Paint;
    HDC DeviceContext = BeginPaint(Window, &Paint);
    int X = Paint.rcPaint.left;
    int Y = Paint.rcPaint.top;
    int Width = Paint.rcPaint.right - Paint.rcPaint.left;
    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
    Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                               Dimension.Width, Dimension.Height);
    EndPaint(Window, &Paint);
  } break;
  default: {
    Result = DefWindowProc(Window, Message, WParam, LParam);
  } break;
  }
  return (Result);
}

///////////////////////////////////////////////////////////////
/// WinMain
//////////////////////////////////////////////////////////////

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR CommandLine, int ShowCode) {

  LARGE_INTEGER PerfCountFrequencyResult;
  QueryPerformanceCounter(&PerfCountFrequencyResult);
  int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

  WNDCLASSA WindowClass = {};
  Win32LoadXInput();
  Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
  WindowClass.style = CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";
  BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER * lpFrequency);

  if (RegisterClass(&WindowClass)) {
    HWND Window = CreateWindowEx(0, WindowClass.lpszClassName, "Handmade Hero",
                                 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, 0, 0, Instance, 0);
    if (Window) {
      // NOTE:(zxieeee):  declaration

      win32_sound_output SoundOutput = {};
      MSG Message;
      Running = true;
      int XOffset = 0;
      int YOffset = 0;
      HDC DeviceContext = GetDC(Window);
      bool SoundIsPlaying = false;

      // NOTE:(zxieeee): __rdtsc Initialisations
      Win32InitDSound(Window, SoundOutput.SamplesPerSecond,
      LARGE_INTEGER LastCounter;
      QueryPerformanceCounter(&LastCounter);
      uint64 LastCycleCount = __rdtsc();

    //NOTE:(zxieeee): Sound Initialisations
      Win32InitDSound(Window, SoundOutput.SamplesPerSecond,
                      SoundOutput.SecondaryBufferSize);
      Win32ClearBuffer(&SoundOutput);
      GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);


      SoundOutput.RunningSampleIndex = 0;
      SoundOutput.SamplesPerSecond = 48000;
      SoundOutput.ToneVolume = 8000;
      SoundOutput.ToneHz = 256;
      SoundOutput.BytesPerSample = sizeof(int16) * 2;
      SoundOutput.WavePeriod =
          SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
      SoundOutput.SecondaryBufferSize =
          SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

      //
      //NOTE:(zxieeee): MainLoop
      //
      while (Running) {
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
          if (Message.message == WM_QUIT) {
            Running = false;
          }
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }
        // NOTE:(zxieeee): Controller Setup
        DWORD dwResult;
        for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT;
             ControllerIndex++) {

          XINPUT_STATE ControllerState;
          ZeroMemory(&ControllerState, sizeof(XINPUT_STATE));

          dwResult = XInputGetState(ControllerIndex, &ControllerState);

          if (dwResult == ERROR_SUCCESS) {
            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

            bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
            bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
            bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool RightShoulder =
                (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
            bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
            bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
            bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

            int16 StickX = Pad->sThumbLX;
            int16 StickY = Pad->sThumbLY;

            // Controller is connected
          } else {
            // Controller is not connected
          }
        }

        int16 Samples[48000 / 30 * 2];
        game_sound_output_buffer SoundBuffer = {};
        SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
        SoundBuffer.SampleCount = SoundBuffer.SamplesPerSecond / 30;
        SoundBuffer.Samples = Samples;

        game_offscreen_buffer Buffer = {};
        Buffer.Memory = GlobalBackBuffer.Memory;
        Buffer.Width = GlobalBackBuffer.Width;
        Buffer.Height = GlobalBackBuffer.Height;
        Buffer.Pitch = GlobalBackBuffer.Pitch;

        DWORD PlayCursor;
        DWORD WriteCursor;
        DWORD BytesToWrite;
        DWORD ByteToLock;
        DWORD TargetCursor;
        bool32 SoundIsValid = false;

        if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(
                &PlayCursor, &WriteCursor))) {
          SoundIsValid = true;

          ByteToLock = SoundOutput.RunningSampleIndex *
                       SoundOutput.BytesPerSample %
                       SoundOutput.SecondaryBufferSize;
          TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount *
                                         SoundOutput.BytesPerSample)) %
                          SoundOutput.SecondaryBufferSize);
          if (ByteToLock > TargetCursor) {
            BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
            BytesToWrite += TargetCursor;
          } else {
            BytesToWrite = TargetCursor - ByteToLock;
          }
        }

        if (SoundIsValid) {
          Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite,
                               &SoundBuffer);
        }
        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        // RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);
        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                                   Dimension.Width, Dimension.Height);

        GameUpdateAndRender(&Buffer, XOffset, YOffset, &SoundBuffer);
        XOffset++;
        YOffset++;
        LARGE_INTEGER EndCounter;
        QueryPerformanceCounter(&EndCounter);
        // TODO: Display the value here
        uint64 EndCycleCount = __rdtsc();
        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
        int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
        int64 MSPerFrame = ((1000 * CounterElapsed) / PerfCountFrequency);
        int32 FPS = PerfCountFrequency / CounterElapsed;

        // NOTE: last mark
        // char Buffer[256];
        // wsprintf(Buffer, "%dms/f,%df/s", MSPerFrame, FPS);
        // OutputDebugStringA(Buffer);
        LastCounter = EndCounter;
        LastCycleCount = EndCycleCount;
      }
    } else {
    }
  }
  return (0);
}
