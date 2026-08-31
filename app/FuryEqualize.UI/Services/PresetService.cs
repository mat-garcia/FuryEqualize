namespace FuryEqualize.UI.Services;

public record Preset(
    string Name,
    string Description,
    float ThresholdDb,
    float Ratio,
    float AttackMs,
    float ReleaseMs,
    float MakeupDb,
    float LowShelfDb,
    float PeakDb,
    float HighShelfDb
);

public static class PresetService
{
    // Presets locais — agora agressivos estilo Rara (corte grave forte + boost passos + limiter)
    public static readonly IReadOnlyList<Preset> Presets = new List<Preset>
    {
        new("Stereo / Flat", "Sem processamento — pass-through", -60, 1, 2, 80, 0, 0, 0, 0),
        new("Competitive (Rara)", "Agressivo: -14dB <150Hz, +8dB @3.5kHz, limiter 8:1 thr -32", -32, 8f, 1.0f, 50, 10, -14, 8, 0),
        new("Quiet Gunshots (Rara+)", "Extremo: -18dB grave, +10dB passos, limiter 12:1 thr -28 makeup +12", -28, 12f, 0.8f, 40, 12, -18, 10, 2),
        new("Footsteps MAX", "MAX: -18dB low, +12dB @3.5kHz, +4dB high, 12:1 thr -36", -36, 12f, 0.5f, 35, 14, -18, 12, 4),
        new("Competitive (Suave)", "Antes: -6dB/150Hz +4dB/3.5kHz ratio 2.5 (legado)", -30, 2.5f, 2, 80, 3, -6, 4, 0),
    };

    public static Preset GetByName(string name) =>
        Presets.FirstOrDefault(p => p.Name == name) ?? Presets[0];

    public static void Apply(Preset p)
    {
        if (AudioEngineInterop.IsAvailable)
        {
            AudioEngineInterop.AudioEngine_SetPreset(p.ThresholdDb, p.Ratio, p.AttackMs, p.ReleaseMs, p.MakeupDb);
            AudioEngineInterop.AudioEngine_SetEq(p.LowShelfDb, p.PeakDb, p.HighShelfDb);
        }
    }
}
