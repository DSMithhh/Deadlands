"""
Deadlands Pack 16 - one-time Unreal Editor map builder.

Run from the Unreal Python console:
exec(open(r"C:/GitHub/Deadlands/Content/Python/build_deadlands_town.py", encoding="utf-8").read())

Requires the built Deadlands C++ module and the Python Editor Script Plugin.
Creates /Game/Maps/L_DeadlandsTown.umap. Does NOT modify L_TestLab.
"""
import unreal

MAP_PATH = "/Game/Maps/L_DeadlandsTown"

# Do not silently overwrite a map someone may have edited.
if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    raise RuntimeError(
        MAP_PATH + " already exists. Open it instead of rerunning the builder. "
        "Delete or rename it manually only if you intentionally want to regenerate it."
    )

if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
    raise RuntimeError("Could not create the saved town level at " + MAP_PATH)

def spawn(actor_class, location, label, rotation=None):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class, location, rotation or unreal.Rotator(0, 0, 0)
    )
    if actor is None:
        raise RuntimeError("Could not spawn " + label)
    actor.set_actor_label(label)
    return actor

# A preplaced town means ZombieBootstrap will NOT create the runtime duplicate.
town_class = unreal.load_class(None, "/Script/Deadlands.DeadlandsTownPrototype")
if town_class is None:
    raise RuntimeError("DeadlandsTownPrototype C++ class missing. Build DeadlandsEditor first.")

spawn(town_class, unreal.Vector(0, 0, 0), "Town_Architecture_Greybox")
spawn(unreal.PlayerStart, unreal.Vector(-2450, 0, 150), "PlayerStart_WestRoad",
      unreal.Rotator(0, 0, 0))

# Navigation coverage: includes roads, interiors and nearby surrounding ground.
# NavMeshBoundsVolume has a brush, and actor scaling expands its coverage.
nav = spawn(unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 150),
            "NavMesh_EntireTown")
nav.set_actor_scale3d(unreal.Vector(75, 65, 12))

sun = spawn(unreal.DirectionalLight, unreal.Vector(0, 0, 1500),
            "Lighting_Sun", unreal.Rotator(-42, -25, 0))
sun_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
if sun_component:
    sun_component.set_editor_property("intensity", 4.5)

sky = spawn(unreal.SkyLight, unreal.Vector(0, 0, 1100), "Lighting_Sky")
sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
if sky_component:
    sky_component.set_editor_property("intensity", 0.8)

fog = spawn(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0),
            "Atmosphere_DistanceFog")
fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
if fog_component:
    fog_component.set_editor_property("fog_density", 0.008)

if not unreal.EditorLevelLibrary.save_current_level():
    raise RuntimeError("Town was created but saving failed. Save the level manually.")

unreal.log("DEADLANDS PACK 16: Saved " + MAP_PATH +
           " | Town, PlayerStart, NavMesh bounds, sun, skylight, fog.")
unreal.log("Open the level, press P to inspect navigation, and test with two players.")
