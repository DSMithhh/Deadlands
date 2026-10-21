"""Idempotent lighting and sky repair for the saved Deadlands town map."""
import unreal

MAP = "/Game/Maps/L_DeadlandsTown"
MARKER = "Pack16_SkyAtmosphere_v2"

if not unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.log_warning("Deadlands lighting repair: town map does not exist yet.")
else:
    # Explicitly open the target map so we never edit a different level.
    unreal.EditorLevelLibrary.load_level(MAP)
    actors = unreal.EditorLevelLibrary.get_all_level_actors()

    def find_actor(actor_type):
        return next((a for a in actors if isinstance(a, actor_type)), None)

    def ensure(actor_type, label, position, rotation=None):
        found = find_actor(actor_type)
        if found:
            return found
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            actor_type, position, rotation or unreal.Rotator(0, 0, 0)
        )
        if not actor:
            raise RuntimeError("Unable to spawn " + label)
        actor.set_actor_label(label)
        actors.append(actor)
        return actor

    atmosphere = ensure(unreal.SkyAtmosphere, "Atmosphere_ProceduralSky",
                        unreal.Vector(0, 0, 0))
    sun = ensure(unreal.DirectionalLight, "Lighting_Sun",
                 unreal.Vector(0, 0, 1500), unreal.Rotator(-42, -25, 0))
    sun.set_actor_rotation(unreal.Rotator(-42, -25, 0), False)
    sun_light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if sun_light:
        sun_light.set_editor_property("intensity", 8.0)
        sun_light.set_editor_property("atmosphere_sun_light", True)
        sun_light.set_editor_property("atmosphere_sun_light_index", 0)

    skylight = ensure(unreal.SkyLight, "Lighting_Sky",
                      unreal.Vector(0, 0, 1100))
    sky_light = skylight.get_component_by_class(unreal.SkyLightComponent)
    if sky_light:
        sky_light.set_editor_property("real_time_capture", True)
        sky_light.set_editor_property("intensity", 1.0)

    fog = ensure(unreal.ExponentialHeightFog, "Atmosphere_DistanceFog",
                 unreal.Vector(0, 0, 0))
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if fog_component:
        fog_component.set_editor_property("fog_density", 0.004)

    # Clouds are optional: the core daylight setup works without them.
    if hasattr(unreal, "VolumetricCloud"):
        ensure(unreal.VolumetricCloud, "Atmosphere_Clouds",
               unreal.Vector(0, 0, 0))

    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Lighting repaired but town level could not be saved")
    unreal.log("Deadlands Pack 16: sky atmosphere, sun and skylight saved to " + MAP)
