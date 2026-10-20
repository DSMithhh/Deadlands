# Gameplay Pack 03 — typed inventory foundation (prototype)

## Included
- Typed item stacks with IDs **Food, Water, Medicine, Tools, Materials**.
- Per-stack limits: 5, 5, 3, 1, 10. Player carrying capacity: 20 units.
- Server-owned container inventories with persistent depletion; one F press transfers one item after opening.
- Owner-only replicated character inventory; container inventories replicate to both players.
- A single save operation updates both sides of each transfer, avoiding a save-file state where an item appears in both inventories.
- Blueprint presentation hooks: character **OnItemsChanged** and container **OnContainerItemsChanged**, plus **GetItems** BlueprintPure accessors.
- Legacy generic supply containers remain functional.

## Test
1. Close Unreal; pull and compile. Open BP_TestContainer or create another instance with a **new Persistence Id** (e.g. ItemBox01).
2. Set **Initial Supplies** to 0. Add **Initial Items** entries: Food=3, Water=2, Medicine=1, Tools=1, Materials=4. Item IDs are case-sensitive FNames.
3. Two-player listen server: F once opens; each subsequent F transfers one unit, following initial item order.
4. Watch Output Log for `Transferred Food from ...`, etc. Character **GetItems** and container **GetItems** are exposed for Blueprint display/debugging.
5. Both players should share finite container stock. Stop and restart PIE: the container should stay depleted.
6. Check door, light switch and older generic supply containers for regressions.

## Important incomplete work
- **No finished UMG inventory interface yet**. Blueprint hooks exist, but no visual UI assets or input actions were committed.
- Player inventory save keys currently use Unreal **PlayerState PlayerId**, which is not a stable cross-session identity. This is **not reliable persistent player inventory across restarts**, especially in PIE with multiple players. It must be replaced with an authenticated player identity or dedicated persistent character ID before production.
- No direct player-to-player transfer interface, dropping, equipment, weight, or item use.
- SaveGameToSlot is a synchronous whole-file write, not a crash-proof database transaction.
- Item IDs and stack limits are a fixed C++ prototype catalogue; future packs should use data assets.
- Generic supplies remain a separate legacy inventory and are not migrated.
- This commit has not been compiled or run in Unreal by the assistant.
