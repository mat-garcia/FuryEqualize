# Backend FuryEqualize

Contrato em `openapi.yaml`. Implementação sugerida: Laravel (PHP) ou Spring Boot (Java).

## Fluxo
1. `GET /auth/discord/redirect` -> Discord OAuth2
2. Discord callback -> `GET /auth/discord/callback?code=...` -> retorna JWT
3. App WPF chama `GET /v1/license/status` com `Authorization: Bearer <JWT>`
4. App busca presets via `GET /v1/presets` e coeficientes assinados via `GET /v1/presets/{id}/coefficients`
5. App injeta coeficientes direto na DLL via `AudioEngine_SetPreset` / `AudioEngine_SetEq` — **não salvar em disco em texto plano**.

## Segurança dos presets
- Servidor calcula coeficientes Biquad (b0,b1,b2,a1,a2) e assina com HMAC-SHA256 (`signature`).
- Client valida assinatura antes de injetar na DLL (evita tampering).
- Coeficientes têm `expiresAt` curto (ex: 24h) — renovação exige JWT válido.

## Mock para dev local
```bash
# Node mock simples (se quiser testar sem Laravel/Spring)
npx prism mock openapi.yaml --port 4010
# ou
docker run -p 4010:4010 stoplight/prism:5 mock -h 0.0.0.0 openapi.yaml
```
Configure `LicenseService.ApiBaseUrl = "http://localhost:4010"` no WPF.
