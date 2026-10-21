"""Pack 16 one-time map generation on Unreal Editor startup.

Unreal's Python Editor Script Plugin automatically loads Content/Python/init_unreal.py.
Never overwrite a previously generated or manually edited level.
"""
import unreal

TARGET = "/Game/Maps/L_DeadlandsTown"

if unreal.EditorAssetLibrary.does_asset_exist(TARGET):
    unreal.log("Deadlands Pack 16: saved town already exists; startup builder skipped.")
else:
    try:
        import os
        import traceback
        script_path = os.path.join(os.path.dirname(__file__), "build_deadlands_town.py")
        with open(script_path, "r", encoding="utf-8") as source:
            exec(compile(source.read(), script_path, "exec"), {"__name__": "__main__"})
        unreal.log("Deadlands Pack 16: town generated. Open /Game/Maps/L_DeadlandsTown.")
    except Exception:
        unreal.log_error("Deadlands Pack 16 map builder failed:\n" + traceback.format_exc())
