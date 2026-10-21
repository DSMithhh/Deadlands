"""Pack 19: procedural weathered surfaces, no external art assets required.

Creates new material assets (never edits Pack 18 originals), then assigns them
to the existing saved town. Noise is evaluated in world space so repeated cubes
do not show identical UV patterns. Safe to run again.
"""
import unreal

ROOT = "/Game/Deadlands/Materials"
MAP = "/Game/Maps/L_DeadlandsTown"

# Base and variation colors (linear RGB), noise scale, roughness.
SURFACES = {
    "WeatheredAsphalt": ((0.025, .028, .031), (.115, .115, .110), .018, .96),
    "CrackedConcrete": ((.28, .27, .25), (.55, .53, .48), .025, .91),
    "DryGrass": ((.12, .16, .075), (.31, .30, .12), .035, .98),
    "FadedPaint": ((.43, .38, .23), (.82, .73, .48), .03, .89),
    "AgedSiding": ((.31, .30, .26), (.60, .56, .47), .015, .88),
    "RoofShingles": ((.09, .10, .11), (.30, .30, .28), .024, .96),
    "OldWood": ((.16, .09, .045), (.39, .25, .14), .027, .94),
    "OxidizedMetal": ((.13, .15, .16), (.37, .35, .31), .025, .79),
    "DeadLeaves": ((.055, .10, .04), (.21, .29, .09), .035, .98),
    "DirtyGlass": ((.035, .08, .10), (.13, .20, .21), .012, .35),
    "RustPatches": ((.17, .065, .025), (.44, .19, .08), .038, .94),
}

def constant_color(mat, rgb, x, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant3Vector, x, y)
    node.set_editor_property("constant",
                             unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    return node

def create_weathered(name, spec):
    path = ROOT + "/M_" + name
    existing = unreal.EditorAssetLibrary.load_asset(path)
    if existing:
        return existing
    low, high, scale, roughness = spec
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_" + name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if not mat:
        raise RuntimeError("Material creation failed: " + name)

    a = constant_color(mat, low, -600, -220)
    b = constant_color(mat, high, -600, -40)
    noise = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionNoise, -600, 160)
    noise.set_editor_property("scale", scale)
    blend = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionLinearInterpolate, -260, -80)
    unreal.MaterialEditingLibrary.connect_material_expressions(a, "", blend, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(b, "", blend, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(noise, "", blend, "Alpha")
    unreal.MaterialEditingLibrary.connect_material_property(
        blend, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -260, 180)
    rough.set_editor_property("r", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat

def classify(name):
    n = name.lower()
    if "stripe" in n: return "FadedPaint"
    if "road" in n: return "WeatheredAsphalt"
    if "sidewalk" in n or "curb" in n or "bollard" in n: return "CrackedConcrete"
    if n == "ground": return "DryGrass"
    if "treecrown" in n or "overgrowth" in n: return "DeadLeaves"
    if "window" in n: return "DirtyGlass"
    if "roof" in n or "canopy" in n: return "RoofShingles"
    if "fence" in n or "trunk" in n or "table" in n or "shelf" in n: return "OldWood"
    if "wheel" in n: return "RustPatches"
    if "pole" in n or "pump" in n or "car" in n: return "OxidizedMetal"
    return "AgedSiding"

if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.EditorLevelLibrary.load_level(MAP)
    palette = {name: create_weathered(name, spec)
               for name, spec in SURFACES.items()}
    count = 0
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_class().get_name() != "DeadlandsTownPrototype":
            continue
        for part in actor.get_components_by_class(unreal.StaticMeshComponent):
            part.set_material(0, palette[classify(part.get_name())])
            count += 1
    if count and not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Pack 19 materials applied but map save failed")
    unreal.log("Deadlands Pack 19: %d components have weathered materials" % count)
