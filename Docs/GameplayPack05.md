# Gameplay Pack 05 — independent mechanics

Branch: `feature/interaction-light-switch`

Three independent prototype features were added to `ADeadlandsCharacter`. No Blueprint setup or new assets are required if the existing test character inherits from this C++ class.

## A. Sprint
- Hold **Left Shift** to sprint; release to walk.
- Walk speed defaults to 500; sprint speed defaults to 800 (editable in character Blueprint defaults).
- Sprint state is replicated; client updates speed immediately and requests authoritative server state.
- Test in two-player PIE: both pawns should move faster while sprinting.
- Prototype limitation: no stamina, encumbrance or movement-mode restrictions yet.

## B. Flashlight
- Press **L** to toggle a spotlight attached to the first-person camera.
- Starts off. No battery drain.
- **Local-only prototype:** other players are not guaranteed to see the light; remote flashlight replication is deliberately deferred.
- Test in a dark room or dimly lit section; if the level is very bright, the beam may be difficult to see.

## C. Health debug
- Press **H** to apply 25 debug damage; **J** to heal 25.
- Health starts at 100 and is clamped to 0–100.
- Changes are processed on the server and replicated to clients.
- Health values are logged to Unreal Output Log (filter for `LogDeadlands`); the Blueprint event `OnHealthChanged` is available for later HUD hookup.
- **No health HUD or death behavior yet**; this is the replicated data foundation only.
- These keys are for testing and must be removed or secured before shipping.

## Testing
1. Pull, close Unreal, build `DeadlandsEditor`, reopen `L_TestLab`.
2. In PIE verify Tab/F inventory still work and close correctly after clicking buttons.
3. Hold Left Shift and release while moving.
4. Press L in a dim area to toggle the beam.
5. Press H four times, J twice; Output Log should show 0 then 50 health.
6. Repeat in two-player PIE, using both clients independently.

These changes were committed remotely but **not compiled or run by the assistant**. Report the first compiler error or unexpected behavior for targeted fixes.
