"""Deadlands Pack 16: create town once, then repair missing daylight automatically."""
import os
import traceback
import unreal

TARGET = "/Game/Maps/L_DeadlandsTown"
FOLDER = os.path.dirname(__file__)

def run_file(name):
    path = os.path.join(FOLDER, name)
    with open(path, "r", encoding="utf-8") as source:
        exec(compile(source.read(), path, "exec"), {"__name__": "__main__"})

try:
    if not unreal.EditorAssetLibrary.does_asset_exist(TARGET):
        run_file("build_deadlands_town.py")
    # Safe to rerun: only adds missing atmosphere actors and tunes existing lighting.
    run_file("repair_deadlands_lighting.py")
    run_file("apply_deadlands_materials.py")
    run_file("apply_deadlands_weathering.py")
except Exception:
    unreal.log_error("Deadlands Pack 16 startup failed:\n" + traceback.format_exc())
