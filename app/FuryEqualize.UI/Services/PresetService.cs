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
    float HighShelfDb,
    float[] ChannelGains = null
);

public static class PresetService
{
public static readonly IReadOnlyList<Preset> Presets = new List<Preset>
        {
            new("Stereo / Flat", "Sem processamento — pass-through", -60, 1, 2, 80, 0, 0, 0, 0, new float[] { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }),
            new("Sniper Max Suppression", "Sniper cirúrgico: 0.51ms 8:1 thr -41, duck 24dB, step +5.98dB, master +6dB", -41, 8f, 0.51f, 120, 0, 0, 5.98f, 0, new float[] { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }),
            new("Competitive (Rara)", "Agressivo: -14dB <150Hz, +8dB @3.5kHz, limiter 8:1 thr -32", -32, 8f, 1.0f, 50, 10, -14, 8, 0, new float[] { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }),
            new("Quiet Gunshots (Rara+)", "Extremo: -18dB grave, +10dB passos, limiter 12:1 thr -28 makeup +12", -28, 12f, 0.8f, 40, 12, -18, 10, 2, new float[] { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }),
            new("Footsteps MAX", "MAX: -18dB low, +12dB @3.5kHz, +4dB high, 12:1 thr -36", -36, 12f, 0.5f, 35, 14, -18, 12, 4, new float[] { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }),
            new("FuryFootep", "Footstep focus: tampa graves, passos reais, duck -22dB, boost passos", -32, 8f, 0.8f, 35, 14, -18, 10, 2, new float[] { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }),
        };

    public static Preset GetByName(string name) =>
        Presets.FirstOrDefault(p => p.Name == name) ?? Presets[0];

    public static void Apply(Preset p)
    {
        if (!AudioEngineInterop.IsAvailable) return;
        // Sniper — usa struct completa via JSON (System.Text.Json -> DllImport bytes)
        if (p.Name.Contains("Sniper"))
        {
            try{
                var sniper = SniperPresetService.LoadDefault();
                SniperPresetService.Apply(sniper);
                AudioEngineInterop.AudioEngine_SetMasterVolume(sniper.masterVolumeDb);
            } catch{}
            // Também aplica fallback simples para compatibilidade
            AudioEngineInterop.AudioEngine_SetPreset(p.ThresholdDb, p.Ratio, p.AttackMs, p.ReleaseMs, p.MakeupDb);
            AudioEngineInterop.AudioEngine_SetEq(p.LowShelfDb, p.PeakDb, p.HighShelfDb);
            return;
        }
        AudioEngineInterop.AudioEngine_SetPreset(p.ThresholdDb, p.Ratio, p.AttackMs, p.ReleaseMs, p.MakeupDb);
        AudioEngineInterop.AudioEngine_SetEq(p.LowShelfDb, p.PeakDb, p.HighShelfDb);
    }
}
