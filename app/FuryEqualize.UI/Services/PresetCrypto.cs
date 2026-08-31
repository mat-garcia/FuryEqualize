using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace FuryEqualize.UI.Services;

// Modo local (sem comercialização): presets ficam em PresetService.cs
// Este arquivo mantido apenas como referência futura se quiser voltar ao modelo com backend
public static class PresetCrypto
{
    // Em produção, essa chave NÃO deve estar hardcoded — venha via JWT claims ou obfuscada via DPAPI
    // Aqui é placeholder para dev; backend real usa HMAC com secret rotacionado por usuário
    private static readonly byte[] DevKey = Encoding.UTF8.GetBytes("fury-dev-hmac-key-CHANGE-ME-32B");

    public record BiquadCoeff(string type, float b0, float b1, float b2, float a1, float a2);
    public record PresetCoefficients(
        string presetId,
        BiquadCoeff[] biquads,
        CompressorParams compressor,
        string signature,
        DateTime expiresAt
    );
    public record CompressorParams(float thresholdDb, float ratio, float attackMs, float releaseMs, float makeupDb, float kneeDb);

    public static bool Verify(PresetCoefficients coeff, byte[]? key = null)
    {
        key ??= DevKey;
        // Payload canônico: presetId + biquads + compressor + expiresAt (ordem fixa)
        var payload = JsonSerializer.Serialize(new {
            coeff.presetId,
            biquads = coeff.biquads.Select(b => new { b.type, b.b0, b.b1, b.b2, b.a1, b.a2 }),
            compressor = coeff.compressor,
            expiresAt = coeff.expiresAt.ToString("O")
        });
        using var hmac = new HMACSHA256(key);
        var hash = hmac.ComputeHash(Encoding.UTF8.GetBytes(payload));
        var computed = Convert.ToHexString(hash).ToLowerInvariant();
        var provided = coeff.signature.Trim().ToLowerInvariant();
        // constant-time compare
        return CryptographicOperations.FixedTimeEquals(Encoding.UTF8.GetBytes(computed), Encoding.UTF8.GetBytes(provided));
    }

    public static bool IsExpired(PresetCoefficients coeff) => DateTime.UtcNow > coeff.expiresAt;

    // Aplica direto na DLL em memória, sem tocar disco
    public static bool ApplySecure(PresetCoefficients coeff)
    {
        if (IsExpired(coeff)) return false;
        if (!Verify(coeff)) return false;
        if (!AudioEngineInterop.IsAvailable) return false;

        var c = coeff.compressor;
        var rc1 = AudioEngineInterop.AudioEngine_SetPreset(c.thresholdDb, c.ratio, c.attackMs, c.releaseMs, c.makeupDb);
        if (rc1 != 0) return false;

        // Extrai ganhos das biquads (aproximação): se tivermos 3 biquads, mapeia para low/peak/high
        // Backend já envia coeficientes calculados; aqui ideal seria AudioEngine_SetBiquadCoeffs direto
        // Como C-API atual só tem SetEq (ganhos), fazemos best-effort decode dos ganhos a partir dos tipos
        // Para MVP, aplicamos 0/0/0 e deixa compressor fazer o trabalho; Fase 3 completa adiciona SetBiquadRaw
        var low = coeff.biquads.FirstOrDefault(b => b.type == "lowShelf");
        var peak = coeff.biquads.FirstOrDefault(b => b.type == "peak");
        var high = coeff.biquads.FirstOrDefault(b => b.type == "highShelf");
        // Não temos ganho direto, mas podemos estimar via b0? Para agora, aplica 0
        // TODO: expor AudioEngine_SetBiquadCoeffs(b0,b1,b2,a1,a2) na DLL
        _ = low; _ = peak; _ = high;
        return true;
    }

    // Removido em modo local — não há necessidade de persistência criptografada
    // Se precisar no futuro, use File.WriteAllBytes + ProtectedData.Protect

    // Helper para gerar assinatura no mock/dev (backend faria isso)
    public static string Sign(PresetCoefficients coeff, byte[]? key = null)
    {
        key ??= DevKey;
        var payload = JsonSerializer.Serialize(new {
            coeff.presetId,
            biquads = coeff.biquads.Select(b => new { b.type, b.b0, b.b1, b.b2, b.a1, b.a2 }),
            compressor = coeff.compressor,
            expiresAt = coeff.expiresAt.ToString("O")
        });
        using var hmac = new HMACSHA256(key);
        var hash = hmac.ComputeHash(Encoding.UTF8.GetBytes(payload));
        return Convert.ToHexString(hash).ToLowerInvariant();
    }
}


