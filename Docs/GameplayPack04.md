# Gameplay Pack 04 — Runtime Inventory UI (first pass)

## New behavior
- Press **Tab** to open/close the player's inventory. UI is created at runtime in C++; no Widget Blueprint setup needed.
- Aim at a container and press **F** to open it and show both inventories.
- Choose Food, Water, Medicine, Tools, or Materials from the dropdown; click **Take One**, **Put One**, **Take Stack**, or **Put Stack**.
- The server validates container reach, visibility, open state, item type, quantity, and player capacity. Transfers are written to one SaveGame slot containing both updated inventories.
- Typed containers no longer automatically loot on repeated F presses. Legacy generic-supply containers keep their earlier F behavior.

## Test checklist
1. **Close Unreal completely**, pull the branch, build DeadlandsEditor, reopen.
2. Use a container with **Initial Supplies = 0**, **Initial Items** (Food=3, Water=2), and a **new Persistence Id** (e.g. Pack04Box01).
3. Press Tab. Check that the inventory window appears, then close it with Tab.
4. Look at the container and press F. Pick Food; click Take One. Player count should rise by 1, container fall by 1.
5. Click Put One; the item should return. Try Take Stack, then Put Stack.
6. Two-player PIE: confirm the other player sees container depletion and cannot duplicate items.
7. Stop PIE and restart; container stock should remain consistent with transfers.
8. Confirm doors and switches still respond to F.

## Limitations and risks
- **Not compiled or tested by the assistant**; if compilation fails, send the first actual compiler error, not just MSB3073.
- UI is a minimal C++ runtime widget with text lists, a dropdown and buttons, not a final polished visual design.
- Take/Put Stack requests up to that item's stack limit; actual transfer may be smaller due to availability/capacity.
- Player save key still uses transient PlayerState PlayerId. Cross-session player restoration is **not guaranteed** and must be redesigned around stable identities.
- Generic supplies remain a separate legacy system.
- No player-to-player trade, equipment slots, drag-and-drop, or item usage yet.
- Inventory UI does not yet automatically close when walking away from a container; server will reject out-of-range transfers.
- A SaveGame slot write is not a crash-proof database transaction.
