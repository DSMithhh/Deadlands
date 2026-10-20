# Gameplay Pack 10 — First Zombies

Prototype slow-walker zombie, server authoritative, replicated health/death and movement.

## Editor setup (required)
1. Compile DeadlandsEditor and open Unreal.
2. In Content Browser, create a **Blueprint Class** with **All Classes → DeadlandsZombie** as parent; name it **BP_DeadlandsZombie**.
3. Drag one or more BP_DeadlandsZombie actors into L_TestLab on the floor. They use a built-in sphere placeholder.
4. Add **Nav Mesh Bounds Volume** to the level and scale it to cover the test floor and the zombies. Press **P** in the editor to inspect green navigable ground. Zombies require navmesh to chase.
5. Play with two players. Approach a zombie: it should chase and attack. Punch it three times (20 damage x 3, 60 HP) to kill it.
6. Confirm both players see the zombie movement/death and that zombie attacks decrease player HP.

## Defaults
- Detection radius: 1200 Unreal units
- Walk speed: 180 units/sec
- Attack reach: 140 units
- Attack: 10 HP every 1.5 seconds
- Health: 60 HP
- Zombies do not respawn or drop loot yet.
- Existing friendly-fire setting remains enabled for prototype PvP tests.
- Zombie AI is server-only; replicated actor movement and health/death are sent to clients.

## Troubleshooting
If a zombie stands still, check Nav Mesh Bounds Volume coverage (P shows green) and that it is placed on navigable ground.
If the zombie attacks but looks like a floating sphere, that's expected: models/animations come later.
