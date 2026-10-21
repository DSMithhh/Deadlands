"""Pack 22: replace selected modular placeholder trees with layered conifers.

Uses engine built-in cone/cylinder meshes, avoiding third-party asset licensing
or downloads. Re-runnable, non-destructive to existing town geometry.
"""
import unreal

MAP = "/Game/Maps/L_DeadlandsTown"
PREFIX = "DL22_"
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.EditorLevelLibrary.load_level(MAP)
    cone = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cone.Cone")
    cylinder = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    mats = {
        "foliage": unreal.EditorAssetLibrary.load_asset("/Game/Deadlands/Materials/M_DeadLeaves"),
        "wood": unreal.EditorAssetLibrary.load_asset("/Game/Deadlands/Materials/M_OldWood"),
        "metal": unreal.EditorAssetLibrary.load_asset("/Game/Deadlands/Materials/M_OxidizedMetal"),
        "glass": unreal.EditorAssetLibrary.load_asset("/Game/Deadlands/Materials/M_DirtyGlass"),
        "rust": unreal.EditorAssetLibrary.load_asset("/Game/Deadlands/Materials/M_RustPatches"),
    }
    if not all([cone, cylinder, cube]):
        unreal.log_warning("Pack 22: missing engine primitive meshes, skipped")
    else:
        existing = {a.get_actor_label(): a for a in unreal.EditorLevelLibrary.get_all_level_actors()}
        made = 0

        def part(label, mesh, location, scale, material=None, yaw=0):
            global made
            label = PREFIX + label
            if label in existing:
                return
            actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
                unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(0, yaw, 0))
            if not actor:
                raise RuntimeError("Pack 22 spawn failed: " + label)
            actor.set_actor_label(label)
            actor.set_actor_scale3d(unreal.Vector(*scale))
            comp = actor.get_component_by_class(unreal.StaticMeshComponent)
            comp.set_static_mesh(mesh)
            if material:
                comp.set_material(0, material)
            comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            existing[label] = actor
            made += 1

        # Layered conifers: each segment is separately replaceable in Outliner.
        # New trees are placed in the perimeter strip away from roads/doorways.
        for i in range(16):
            x = -2900 + (i % 8) * 800
            y = 2580 if i < 8 else -2570
            height = 420 + (i % 5) * 55
            part("Pine%02d_Trunk" % i, cylinder,
                 (x, y, height * .42), (.33, .33, height * .0084), mats["wood"])
            for j in range(4):
                radius = 170 - j * 27 + (i % 3) * 8
                z = height * .42 + 35 + j * 100
                part("Pine%02d_Bough%d" % (i, j), cone,
                     (x, y, z), (radius / 50, radius / 50, 2.25),
                     mats["foliage"], (i * 29 + j * 17) % 360)

        # Roadside wreck silhouettes, with individual chassis, cabin, windows
        # and wheels. Visual-only: no unexpected navigation blockers.
        for i, (x, y) in enumerate([(-1800, 1050), (1300, -1100)]):
            yaw = 12 if i == 0 else -17
            part("Wreck%d_Chassis" % i, cube, (x, y, 65),
                 (3.8, 1.75, .65), mats["rust"], yaw)
            part("Wreck%d_Cabin" % i, cube, (x - 25, y, 128),
                 (1.8, 1.50, .65), mats["metal"], yaw)
            part("Wreck%d_Windshield" % i, cube, (x + 72, y, 137),
                 (.07, 1.2, .42), mats["glass"], yaw)
            for j, (dx, dy) in enumerate([(-125, -88), (-125, 88), (125, -88), (125, 88)]):
                part("Wreck%d_Wheel%d" % (i, j), cylinder,
                     (x + dx, y + dy, 36), (.53, .53, .23),
                     mats["metal"], yaw + 90)
        if made:
            if not unreal.EditorLevelLibrary.save_current_level():
                raise RuntimeError("Pack 22 could not save town map")
        unreal.log("Pack 22: added %d independent conifer/wreck components" % made)
