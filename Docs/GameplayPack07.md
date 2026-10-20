# Gameplay Pack 07 — Survival and inventory actions

Five prototype features, on branch `feature/interaction-light-switch`. **Code has not been compiled locally.**

1. **Survival HUD:** top-left four text bars and numeric values for health, stamina, hunger, thirst. Uses the existing runtime-created interaction HUD; no widget Blueprint required.
2. **Eat/drink:** Tab opens inventory. Select Food or Water, then click **EAT / DRINK SELECTED**. Server checks possession and whether hunger/thirst is below 100, persists inventory removal, then restores +30 hunger or +40 thirst. Food/Water cannot be consumed at full vitals. No animation.
3. **Low-vitals stamina penalty:** when either hunger or thirst is below 25, stamina recovery falls to 40% of normal. Sprint drain is unchanged.
4. **Crouch:** Left Ctrl toggles Unreal Character crouch; entering crouch cancels sprint. Default crouch visuals depend on the character Blueprint/animation setup.
5. **Drop/pickup:** Tab, choose any item, click **DROP ONE SELECTED**. Server removes one item, persists player inventory, and spawns a replicated basic sphere pickup in front of player. Aim at sphere and press F to collect. Server checks inventory capacity and persists collection. Pickups are **session-only**: world pickup locations are not persisted after exiting the level. No physics/gravity yet.

## Test order
- Pull changes, close Unreal, build DeadlandsEditor, open L_TestLab.
- Observe four status bars; press H/J to change health and Shift to change stamina.
- Wait for hunger/thirst to fall below 100, then transfer Food/Water from a container to inventory. Tab, select Food or Water, Eat/Drink; inventory count should decrease by one and vital should increase.
- Low-vitals penalty: default decay takes a long time. For testing adjust HungerLossPerMinute and ThirstLossPerMinute on SurvivalVitals component defaults or wait; compare stamina recovery below 25 to normal.
- Press Left Ctrl to crouch/uncrouch; test in two-player PIE.
- Put an item in inventory, Drop One; find the sphere in front, aim and F. Test other player's ability to collect it in two-player PIE.
- Regression: door, light, inventory transfers, day/night, flashlight.

Known limitations: world pickups are not persisted; HUD is text bars rather than polished graphics; survival values are not persisted; consumables do not affect health; no hunger/thirst death damage. Multiplayer replication and PIE need user validation.
