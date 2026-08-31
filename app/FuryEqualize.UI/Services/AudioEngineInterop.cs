using System.Runtime.InteropServices;

namespace FuryEqualize.UI.Services;

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct FuryDeviceInfo
{
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)] public string id;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)] public string name;
    public int channels;
    public int sampleRate;
    public int isDefault;
}

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct SniperPresetParams
{
    public float lfeGate;
    public float frontLockEng;
    public float frontLockRel;
    public float fcCrack;
    public float tier2Centered;
    public float frontLoDuck;
    public float frontMidDuck;
    public float frontHiDuck;
    public float fcLoDuck;
    public float fcMidDuck;
    public float fcHiDuck;
    public float propThresh;
    public float propRatio;
    public float propHold;
    public float frontThLo;
    public float frontThMid;
    public float frontThHi;
    public float fcThLo;
    public float fcThMid;
    public float fcThHi;
    public float attackMs;
    public float releaseMs;
    public float holdMs;
    public float fcTrRatio;
    public float fcTrFast;
    public float fcTrSlow;
    public float fcTrHold;
    public float coherenceEng;
    public float coherenceRel;
    public float subWeight;
    public float panConfirm;
    public float centerConfirm;
    public float stepDuck;
    public float stepHold;
    public float coherenceMax;
    public float panMin;
    public float subMax;
    public float onsetRise;
    public float stepLift;
    public float liftHold;
    public float reflHold;
    public float tiltThresh;
    public float reflDepth;
    public float reflAttack;
    public float reflRelease;
    public float boomThr;
    public float boomDepth;
    public float boomRel;
    public float masterVolumeDb;
}

internal static class AudioEngineInterop
{
    private const string DllName = "FuryEqualizeCore.dll";

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    public static extern int AudioEngine_Start(string? deviceId);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_Stop();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_IsRunning();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_SetPreset(float thresholdDb, float ratio, float attackMs, float releaseMs, float makeupDb);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_SetEq(float lowShelfGainDb, float peakGainDb, float highShelfGainDb);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_SetBuffer(int bufferSize);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_GetBuffer();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    public static extern int AudioEngine_SetRenderDevice(string? renderDeviceId);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    public static extern int AudioEngine_GetRenderDevice(byte[] outId, int maxLen);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_SetMasterVolume(float db);
    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern float AudioEngine_GetMasterVolume();
    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_SetSniperPreset(ref SniperPresetParams p);
    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    public static extern int AudioEngine_LoadSniperPresetJson(string path);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_GetDevices([Out] FuryDeviceInfo[] outDevices, int maxCount);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern float AudioEngine_GetInputLevelDb();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern float AudioEngine_GetGainReductionDb();

    public static bool IsAvailable
    {
        get
        {
            try { _ = AudioEngine_GetBuffer(); return true; }
            catch (DllNotFoundException) { return false; }
            catch (BadImageFormatException) { return false; }
        }
    }
}
