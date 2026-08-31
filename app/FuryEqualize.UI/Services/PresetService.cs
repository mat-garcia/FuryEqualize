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
    // Presets locais (fallback). Em produção, coeficientes vêm do Backend assinado.
    public static readonly IReadOnlyList<Preset> Presets = new List<Preset>
    {
        new("Stereo / Flat", "Sem processamento, somente pass-through", -60, 1, 2, 80, 0, 0, 0, 0),
        new("Competitive", "Corta graves <150Hz, boost passos 2-6kHz, compressão leve", -30, 2.5f, 2, 80, 3, -6, 4, 0),
        new("Quiet Gunshots", "Compressor forte para tiros distantes + EQ footsteps", -24, 6, 1.5f, 60, 8, -8, 6, 2),
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
