#pragma once
#include <stdint.h>

// Preset Sniper_Max_Suppression — extraído do vídeo, cirúrgico para sniper sense máxima
// Salvo como JSON em presets/Sniper_Max_Suppression.json e carregado via C-API em memória (sem alocação no audio thread)
#pragma pack(push, 1)
struct SniperPresetParams {
    // --- Weapon Focus ---
    float lfeGate = -50.0f;
    float frontLockEng = 4.0f;
    float frontLockRel = 2.0f;
    float fcCrack = -9.44f;
    float tier2Centered = -3.60f;
    float frontLoDuck = 15.0f;
    float frontMidDuck = 24.0f;
    float frontHiDuck = 24.0f;
    float fcLoDuck = 15.0f;
    float fcMidDuck = 24.0f;
    float fcHiDuck = 24.0f;
    // --- Level Tracking ---
    float propThresh = -41.0f;
    float propRatio = 8.0f;
    float propHold = 200.0f;
    float frontThLo = -8.82f;
    float frontThMid = -7.22f;
    float frontThHi = -5.96f;
    float fcThLo = -8.82f;
    float fcThMid = -6.72f;
    float fcThHi = -5.49f;
    float attackMs = 0.51f;
    float releaseMs = 120.0f;
    float holdMs = 190.0f;
    // --- Dialog Guard ---
    float fcTrRatio = 6.0f;
    float fcTrFast = 3.71f;
    float fcTrSlow = 40.0f;
    float fcTrHold = 20.0f;
    // --- Footstep Focus ---
    float coherenceEng = 0.30f;
    float coherenceRel = 0.50f;
    float subWeight = 0.29f;
    float panConfirm = 8.03f;
    float centerConfirm = -23.3f;
    float stepDuck = 22.0f;
    float stepHold = 122.0f;
    float coherenceMax = 0.25f;
    float panMin = 8.03f;
    float subMax = 0.24f;
    float onsetRise = 6.0f;
    float stepLift = 5.98f;
    float liftHold = 64.0f;
    // --- Reflection Control ---
    float reflHold = 0.0f;
    float tiltThresh = -14.8f;
    float reflDepth = 12.0f;
    float reflAttack = 0.0f;
    float reflRelease = 38.0f;
    float boomThr = -55.0f;
    float boomDepth = -34.8f;
    float boomRel = 23.0f;
    // --- Master ---
    float masterVolumeDb = 6.0f; // extra: para aumentar som geral
};
#pragma pack(pop)

// Instância ativa (lida pela thread de áudio sem lock — atomic via memcpy)
extern SniperPresetParams g_activeSniperPreset;
