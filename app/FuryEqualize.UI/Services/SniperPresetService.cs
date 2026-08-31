using System.IO;
using System.Text.Json;

namespace FuryEqualize.UI.Services;

public static class SniperPresetService
{
    public static string PresetFolder => Path.Combine(AppContext.BaseDirectory, "presets");
    public static string AppDataFolder => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "FuryEqualize", "presets");

    public static SniperPresetParams LoadFromJson(string path)
    {
        var json = File.ReadAllText(path);
        using var doc = JsonDocument.Parse(json);
        var root = doc.RootElement;

        SniperPresetParams p = new();
        // Weapon Focus
        if(root.TryGetProperty("self_suppression", out var ss)){
            if(ss.TryGetProperty("weapon_focus", out var wf)){
                p.lfeGate = GetFloat(wf, "lfe_gate", -50);
                p.frontLockEng = GetFloat(wf, "front_lock_eng", 4);
                p.frontLockRel = GetFloat(wf, "front_lock_rel", 2);
                p.fcCrack = GetFloat(wf, "fc_crack", -9.44f);
                p.tier2Centered = GetFloat(wf, "tier2_centered", -3.6f);
                if(wf.TryGetProperty("depth_per_band", out var dpb)){
                    p.frontLoDuck = GetFloat(dpb, "front_lo_duck", 15);
                    p.frontMidDuck = GetFloat(dpb, "front_mid_duck", 24);
                    p.frontHiDuck = GetFloat(dpb, "front_hi_duck", 24);
                    p.fcLoDuck = GetFloat(dpb, "fc_lo_duck", 15);
                    p.fcMidDuck = GetFloat(dpb, "fc_mid_duck", 24);
                    p.fcHiDuck = GetFloat(dpb, "fc_hi_duck", 24);
                }
            }
            if(ss.TryGetProperty("level_tracking", out var lt)){
                p.propThresh = GetFloat(lt, "prop_thresh", -41);
                p.propRatio = GetFloat(lt, "prop_ratio", 8);
                p.propHold = GetFloat(lt, "prop_hold", 200);
                p.frontThLo = GetFloat(lt, "front_th_lo", -8.82f);
                p.frontThMid = GetFloat(lt, "front_th_mid", -7.22f);
                p.frontThHi = GetFloat(lt, "front_th_hi", -5.96f);
                p.fcThLo = GetFloat(lt, "fc_th_lo", -8.82f);
                p.fcThMid = GetFloat(lt, "fc_th_mid", -6.72f);
                p.fcThHi = GetFloat(lt, "fc_th_hi", -5.49f);
                if(lt.TryGetProperty("timing", out var tm)){
                    p.attackMs = GetFloat(tm, "attack", 0.51f);
                    p.releaseMs = GetFloat(tm, "release", 120);
                    p.holdMs = GetFloat(tm, "hold", 190);
                }
            }
            if(ss.TryGetProperty("dialog_guard", out var dg)){
                p.fcTrRatio = GetFloat(dg, "fc_tr_ratio", 6);
                p.fcTrFast = GetFloat(dg, "fc_tr_fast", 3.71f);
                p.fcTrSlow = GetFloat(dg, "fc_tr_slow", 40);
                p.fcTrHold = GetFloat(dg, "fc_tr_hold", 20);
            }
        }
        if(root.TryGetProperty("footstep_focus", out var ff)){
            p.coherenceEng = GetFloat(ff, "coherence_eng", 0.3f);
            p.coherenceRel = GetFloat(ff, "coherence_rel", 0.5f);
            p.subWeight = GetFloat(ff, "sub_weight", 0.29f);
            p.panConfirm = GetFloat(ff, "pan_confirm", 8.03f);
            p.centerConfirm = GetFloat(ff, "center_confirm", -23.3f);
            p.stepDuck = GetFloat(ff, "step_duck", 22);
            p.stepHold = GetFloat(ff, "step_hold", 122);
        }
        if(root.TryGetProperty("footstep_boost", out var fb)){
            p.coherenceMax = GetFloat(fb, "coherence_max", 0.25f);
            p.panMin = GetFloat(fb, "pan_min", 8.03f);
            p.subMax = GetFloat(fb, "sub_max", 0.24f);
            p.onsetRise = GetFloat(fb, "onset_rise", 6);
            p.stepLift = GetFloat(fb, "step_lift", 5.98f);
            p.liftHold = GetFloat(fb, "lift_hold", 64);
        }
        if(root.TryGetProperty("reflection_control", out var rc)){
            p.reflHold = GetFloat(rc, "hold", 0);
            p.tiltThresh = GetFloat(rc, "tilt_thresh", -14.8f);
            p.reflDepth = GetFloat(rc, "depth", 12);
            p.reflAttack = GetFloat(rc, "attack", 0);
            p.reflRelease = GetFloat(rc, "release", 38);
            p.boomThr = GetFloat(rc, "boom_thr", -55);
            p.boomDepth = GetFloat(rc, "boom_depth", -34.8f);
            p.boomRel = GetFloat(rc, "boom_rel", 23);
        }
        p.masterVolumeDb = GetFloat(root, "master_volume_db", 6);
        return p;
    }

    public static void Apply(SniperPresetParams p){
        if(!AudioEngineInterop.IsAvailable) return;
        AudioEngineInterop.AudioEngine_SetSniperPreset(ref p);
    }

    public static SniperPresetParams LoadDefault(){
        // Tenta AppData primeiro, depois presets ao lado do exe, depois fallback embutido
        string[] candidates = {
            Path.Combine(AppDataFolder, "Sniper_Max_Suppression.json"),
            Path.Combine(PresetFolder, "Sniper_Max_Suppression.json"),
            Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "presets", "Sniper_Max_Suppression.json"),
            "presets/Sniper_Max_Suppression.json"
        };
        foreach(var c in candidates){
            var full = Path.GetFullPath(c);
            if(File.Exists(full)){
                try{ return LoadFromJson(full); } catch{}
            }
        }
        // Fallback: valores do JSON embutido
        return new SniperPresetParams{
            lfeGate=-50, frontLockEng=4, frontLockRel=2, fcCrack=-9.44f, tier2Centered=-3.6f,
            frontLoDuck=15, frontMidDuck=24, frontHiDuck=24, fcLoDuck=15, fcMidDuck=24, fcHiDuck=24,
            propThresh=-41, propRatio=8, propHold=200, frontThLo=-8.82f, frontThMid=-7.22f, frontThHi=-5.96f,
            fcThLo=-8.82f, fcThMid=-6.72f, fcThHi=-5.49f, attackMs=0.51f, releaseMs=120, holdMs=190,
            fcTrRatio=6, fcTrFast=3.71f, fcTrSlow=40, fcTrHold=20,
            coherenceEng=0.3f, coherenceRel=0.5f, subWeight=0.29f, panConfirm=8.03f, centerConfirm=-23.3f,
            stepDuck=22, stepHold=122, coherenceMax=0.25f, panMin=8.03f, subMax=0.24f, onsetRise=6, stepLift=5.98f, liftHold=64,
            reflHold=0, tiltThresh=-14.8f, reflDepth=12, reflAttack=0, reflRelease=38, boomThr=-55, boomDepth=-34.8f, boomRel=23,
            masterVolumeDb=6
        };
    }

    private static float GetFloat(JsonElement el, string name, float def){
        if(el.TryGetProperty(name, out var v) && v.TryGetSingle(out var f)) return f;
        if(el.TryGetProperty(name, out var v2) && v2.TryGetDouble(out var d)) return (float)d;
        return def;
    }
}
