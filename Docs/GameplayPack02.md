# Deadlands Gameplay Pack 02 — finite supply prototype

## What is implemented
- Containers start with an instance-configurable **InitialSupplies** count (default 5).
- The authoritative server saves each container's **RemainingSupplies** under its stable PersistenceId.
- An empty container stays empty across restarts. A saved value of zero is not treated as missing.
- First F interaction opens a closed container; subsequent F presses take one supply each. F on an open, empty container closes it.
- Supplies transfer into the interacting DeadlandsCharacter's replicated **Supplies** counter, capped at **SupplyCapacity** (default 20).
- Only the owning player receives their inventory replication. Other clients see the container's remaining amount.
- Blueprint presentation events: **OnSuppliesChanged** on character, **OnRemainingSuppliesChanged** on container.

## Test in L_TestLab
1. Close Unreal, pull, build DeadlandsEditor, then reopen the editor.
2. Select the placed BP_TestContainer. Keep its unique PersistenceId and set InitialSupplies to 3. **Use a new PersistenceId** if this container was already looted in an earlier test.
3. Play with 2 players as a listen server.
4. Player 1: press F once to open, then three more times to take supplies. Inspect the character's Supplies and the container's RemainingSupplies in the Blueprint debugger/Details or watch Output Log for "Looted" and "now carries".
5. Player 2 should not be able to loot any more. Stop PIE and start again; the container must still be empty.
6. Check door and light switch still work.

## Important limitations
- This is **generic supply units**, not item definitions, weight, equipment, or a finished inventory UI.
- Player inventory is currently held on the character pawn and **does not persist across PIE restart, logout, or pawn replacement**. Only container depletion persists.
- A server crash between saving container depletion and granting inventory can lose a supply; the production implementation will need atomic transactions and persistent player inventories.
- Existing container open/close behavior changes: an open container now attempts to loot before it can close, and can only close with F when empty.
- No randomized loot, resource respawns, or item duplication are intended.
- Saving remains synchronous and local to the listen server; not yet production-ready.
- No on-screen prompts or animations are included in this pack.
