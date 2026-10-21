"""Pack 18: editor-side town material palette."""
import unreal

ROOT = "/Game/Deadlands/Materials"
COLORS = {
    "Asphalt": (0.07, 0.07, 0.08),
    "Concrete": (0.43, 0.42, 0.39),
    "Grass": (0.18, 0.24, 0.13),
    "Paint": (0.8, 0.7, 0.4),
    "Walls": (0.52, 0.49, 0.42),
    "Roof": (0.22, 0.23, 0.24),
    "Wood": (0.32, 0.22, 0.13),
    "Metal": (0.28, 0.31, 0.32),
    "Leaves": (0.12, 0.25, 0.12),
    "Glass": (0.10, 0.18, 0.23),
    "Rust": (0.38, 0.19, 0.12),
}

def create(name, rgb):
    path = ROOT + "/M_" + name
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset:
        return asset
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_" + name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if asset is None:
        raise RuntimeError("Could not create material: " + name)
    node = unreal.MaterialEditingLibrary.create_material_expression(
        asset, unreal.MaterialExpressionConstant3Vector, -300, 0)
    node.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        node, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(asset)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset

def category(n):
    n = n.lower()
    if "stripe" in n: return "Paint"
    if "road" in n: return "Asphalt"
    if "sidewalk" in n or "curb" in n: return "Concrete"
    if n == "ground": return "Grass"
    if "treecrown" in n or "overgrowth" in n: return "Leaves"
    if "window" in n: return "Glass"
    if "roof" in n or "canopy" in n: return "Roof"
    if "fence" in n or "trunk" in n or "table" in n or "shelf" in n: return "Wood"
    if "wheel" in n: return "Rust"
    if "pole" in n or "pump" in n or "car" in n: return "Metal"
    return "Walls"

MAP = "/Game/Maps/L_DeadlandsTown"
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.EditorLevelLibrary.load_level(MAP)
    palette = {n: create(n, rgb) for n, rgb in COLORS.items()}
    updated = 0
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_class().get_name() == "DeadlandsTownPrototype":
            for part in actor.get_components_by_class(unreal.StaticMeshComponent):
                part.set_material(0, palette[category(part.get_name())])
                updated += 1
    if updated:
        unreal.EditorLevelLibrary.save_current_level()
    unreal.log("Pack 18: applied materials to %d town components" % updated)
