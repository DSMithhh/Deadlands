# Deadlands Gameplay Pack 01 — interaction foundations

## Included
- Persistent, replicated hinged door (`ADeadlandsDoor`).
- Server-side door lock state and `SetLocked` API. Locked doors cannot be operated with F.
- Persistent, replicated container open/closed state (`ADeadlandsContainer`).
- Generic saved actor boolean flags (`SavedActorFlags`) in the existing SaveGame.
- Reuse of existing server-validated F-key interaction.

## Important limitations
- This is a C++ foundation, **not** a finished art or UI pack.
- Assign a collision-enabled static mesh in Blueprint subclasses of the door and container. Without a mesh, the camera trace cannot hit the actor.
- The door rotates its DoorMesh around DoorRoot immediately (no animation). Use a mesh whose hinge alignment fits the component offset.
- Container open/close is state-only. `OnContainerStateChanged` is a Blueprint event for visual presentation; inventory and finite loot are NOT implemented.
- `SetLocked` is a server-only gameplay API; there is no key or lockpicking system yet.
- For level-placed actors, assign a **unique, stable PersistenceId** on each instance before shipping. Actor-name fallback is only for prototyping.
- Saved flags use the same synchronous local SaveGame slot as the light switch. Production server saves need batching, versioning, failure recovery, and atomic writes.
- No item spawning, loot replenishment, offscreen activity, or player inventory has been introduced.

## Test sequence
1. Close Unreal, pull branch, build DeadlandsEditor.
2. Create Blueprint subclasses of DeadlandsDoor and DeadlandsContainer; give their meshes collision-enabled cube meshes.
3. Place one of each in L_TestLab and give each a unique PersistenceId.
4. Run 2 players in Play As Listen Server. Both should see door state; container state requires Blueprint visual feedback or inspection.
5. Toggle, stop PIE, restart and check saved states.
6. Verify existing BP_LightSwitch still works and remembers its state.

## Interaction feedback
A visible interaction prompt and client-facing `CanInteract` feedback are deferred to the next iteration. The existing interface can be extended without changing the F-key binding.
