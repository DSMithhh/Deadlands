# Gameplay Pack 06 — Five parallel foundations

All code on `feature/interaction-light-switch`. Build not verified remotely.

1. **Stamina:** `UDeadlandsVitalsComponent` automatically attached to `ADeadlandsCharacter`. Hold Left Shift to sprint. Server drains 18 stamina/s, restores 12/s otherwise. Sprint stops at zero; values replicate to owner. Note: sprint prediction is prototype-level and may need multiplayer smoothing.
2. **Hunger:** same vitals component, independent hunger counter 0–100. Server drains 0.5/minute; no penalties or food consumption yet.
3. **Thirst:** independent counter 0–100; server drains 1/minute. No penalties or water consumption yet.
4. **Day/night:** place an actor of class `DeadlandsDayNightClock` in `L_TestLab`, assign a **Directional Light** to its **Sun** field in Details. It starts at 12:00, one game day takes 1200 real seconds by default; `Hour` is replicated. You must place the actor and assign Sun to see the light rotate. For quick testing set Real Seconds Per Day to 60 in actor Details. It does not yet adjust sky atmosphere, skylight, exposure, or persist time.
5. **Interaction prompt:** created automatically for local player; shows **[F] Interact** near center when looking at an object implementing DeadlandsInteractable that returns CanInteract. Hides while inventory is open.

## Test matrix
- First pull, close Unreal, build DeadlandsEditor and open L_TestLab.
- Shift: compare walk/sprint; hold for ~6 seconds to see stamina limit (18/s from 100).
- Hunger/Thirst: select player pawn during PIE and inspect SurvivalVitals values (or use GetVitals Blueprint getters); wait or temporarily raise rates in class defaults.
- Day/night: manually place clock actor and assign sun; speed up day to 60s.
- Prompt: look at a container, door, light switch; verify it disappears when looking away.
- Regression: Tab and F inventory, flashlight L, H/J health, two-player PIE.

This is a prototype. Health, hunger and thirst have no linked damage effects. No survival HUD bars yet. The world clock requires level placement. These systems are intentionally isolated so each can be debugged separately.
