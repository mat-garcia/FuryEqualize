using System.Net.Http;
using System.Net.Http.Headers;
using System.Net.Http.Json;

namespace FuryEqualize.UI.Services;

// DEPRECATED: modo local sem comercializacao — mantido só como referência
// Presets são locais em PresetService.cs, sem JWT/OAuth
[Obsolete("Modo local: nao usado. Ver PresetService para presets offline.")]
public record LicenseStatus(bool IsActive, string Plan, DateTime? ExpiresAt, string Message);

[Obsolete("Modo local: nao usado.")]
public class LicenseService
{
    private readonly HttpClient _http = new() { Timeout = TimeSpan.FromSeconds(8) };
    private string? _jwt;

    public string ApiBaseUrl { get; set; } = "https://api.furyequalize.local";

    public void SetToken(string jwt)
    {
        _jwt = jwt;
        _http.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", jwt);
    }

    // Mock local para dev sem backend
    public async Task<LicenseStatus> CheckAsync()
    {
        if (string.IsNullOrEmpty(_jwt))
            return new(false, "free", null, "Não autenticado — modo offline (presets locais)");

        try
        {
            var resp = await _http.GetFromJsonAsync<LicenseStatus>($"{ApiBaseUrl}/v1/license/status");
            return resp ?? new(false, "free", null, "Resposta vazia");
        }
        catch (Exception ex)
        {
            return new(false, "free", null, $"Offline: {ex.Message}");
        }
    }

    // Fluxo Discord OAuth2: abre browser -> callback com JWT -> SetToken
    public string GetDiscordOAuthUrl() => $"{ApiBaseUrl}/auth/discord/redirect";
}
