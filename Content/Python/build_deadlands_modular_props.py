"""Pack 21: independently editable environment props in the saved town.

Creates ordinary StaticMeshActors with stable names. Existing actors are never
deleted, and repeated runs skip previously created props. All foliage is visual
only so it cannot accidentally block zombie navigation.
"""
import unreal

MAP = "/Game/Maps/L_DeadlandsTown"
PREFIX = "DL21_"
MESHES = {
    "trunk": "/Engine/BasicShapes/Cylinder.Cylinder",
    "leaf": "/Engine/BasicShapes/Sphere.Sphere",
    "stone": "/Engine/BasicShapes/Sphere.Sphere",
    "box": "/Engine/BasicShapes/Cube.Cube",
}
MATERIALS = {
    "trunk": "/Game/Deadlands/Materials/M_OldWood",
    "leaf": "/Game/Deadlands/Materials/M_DeadLeaves",
    "stone": "/Game/Deadlands/Materials/M_CrackedConcrete",
    "box": "/Game/Deadlands/Materials/M_OxidizedMetal",
}

if not unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.log_warning("Pack 21: town map missing; skipping modular props")
else:
    unreal.EditorLevelLibrary.load_level(MAP)
    meshes = {k: unreal.EditorAssetLibrary.load_asset(v) for k, v in MESHES.items()}
    materials = {k: unreal.EditorAssetLibrary.load_asset(v) for k, v in MATERIALS.items()}
    if not all(meshes.values()):
        raise RuntimeError("Pack 21: required built-in Unreal meshes are unavailable")
    existing = {a.get_actor_label() for a in unreal.EditorLevelLibrary.get_all_level_actors()}
    created = 0

    def place(label, kind, xyz, scale, yaw=0.0):
        global created
        label = PREFIX + label
        if label in existing:
            return
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.StaticMeshActor, unreal.Vector(*xyz), unreal.Rotator(0, yaw, 0))
        if not actor:
            raise RuntimeError("Failed to spawn " + label)
        actor.set_actor_label(label)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(meshes[kind])
        if materials[kind]:
            component.set_material(0, materials[kind])
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        existing.add(label)
        created += 1

    # Mature roadside trees: cylinder trunks and several irregular spherical
    # canopy clusters. These are distinct editor actors, not C++ subobjects.
    for i in range(12):
        x = -2700.0 + (i % 6) * 1030.0
        y = 2440.0 if i < 6 else -2420.0
        height = 360.0 + (i % 4) * 60.0
        place("Tree%02d_Trunk" % i, "trunk",
              (x, y, height * 0.5), (0.43, 0.43, height / 100.0))
        for j, (dx, dy, dz, radius) in enumerate([
            (0, 0, 0, 1.0), (-85, 35, -45, .72),
            (90, -45, -35, .77), (20, 65, 75, .60)
        ]):
            place("Tree%02d_Crown%d" % (i, j), "leaf",
                  (x + dx, y + dy, height + 90 + dz),
                  (2.15 * radius, 1.9 * radius, 1.55 * radius))

    # Small shrubs along the back lots, away from entrances.
    for i in range(16):
        x = -2450.0 + (i % 8) * 660.0
        y = 1960.0 if i < 8 else -1980.0
        place("Bush%02d" % i, "leaf", (x, y, 65),
              (1.25, 1.05, .95), (i * 37) % 180)

    # Rocks and weathered utility cabinets are independent replaceable props.
    for i in range(10):
        x = -2350.0 + i * 490.0
        y = 2170.0 if i % 2 == 0 else -2150.0
        place("Rock%02d" % i, "stone", (x, y, 28),
              (.85 + (i % 3) * .15, .65, .45), i * 19)
    for i, x in enumerate([-2100.0, 1850.0]):
        place("UtilityCabinet%d" % i, "box", (x, -750, 85),
              (1.15, .65, 1.7))

    if created:
        if not unreal.EditorLevelLibrary.save_current_level():
            raise RuntimeError("Pack 21: unable to save modular actors")
    unreal.log("Pack 21: %d modular props added; existing props preserved" % created)
