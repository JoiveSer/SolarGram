#!/usr/bin/env python3
"""
Idempotent pre-configure patcher for external submodules in SolarGram.
Updates submodule symbol references (lib_ui, lib_tl, lib_icu, codegen)
from AyuGram/AyuUiSettings names to SolarGram/SolarUiSettings without requiring
separate upstream forks in .gitmodules.
"""
import os
import shutil

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
TELEGRAM_DIR = os.path.dirname(SCRIPT_DIR)


def patch_file(path: str, replacements: list[tuple[str, str]]) -> bool:
    if not os.path.isfile(path):
        return False
    with open(path, "r", encoding="utf-8", errors="ignore") as fh:
        content = fh.read()
    updated = content
    for old, new in replacements:
        if old in updated:
            updated = updated.replace(old, new)
    if updated != content:
        with open(path, "w", encoding="utf-8", newline="") as fh:
            fh.write(updated)
        return True
    return False


def main() -> None:
    changed = 0

    # 1. lib_ui: rename ayu/ -> solar/ and AyuUiSettings -> SolarUiSettings
    lib_ui_dir = os.path.join(TELEGRAM_DIR, "lib_ui")
    if os.path.isdir(lib_ui_dir):
        ayu_sub = os.path.join(lib_ui_dir, "ayu")
        solar_sub = os.path.join(lib_ui_dir, "solar")
        if os.path.isdir(ayu_sub) and not os.path.isdir(solar_sub):
            shutil.move(ayu_sub, solar_sub)
            changed += 1
        if os.path.isdir(solar_sub):
            for old_name, new_name in [
                ("ayu_ui_settings.h", "solar_ui_settings.h"),
                ("ayu_ui_settings.cpp", "solar_ui_settings.cpp"),
            ]:
                old_p = os.path.join(solar_sub, old_name)
                new_p = os.path.join(solar_sub, new_name)
                if os.path.isfile(old_p) and not os.path.isfile(new_p):
                    shutil.move(old_p, new_p)
                    changed += 1

        ui_replacements = [
            ("ayu/ayu_ui_settings.cpp", "solar/solar_ui_settings.cpp"),
            ("ayu/ayu_ui_settings.h", "solar/solar_ui_settings.h"),
            ('"ayu_ui_settings.h"', '"solar_ui_settings.h"'),
            ("AyuUiSettings", "SolarUiSettings"),
            ("AyuUserpic", "SolarUserpic"),
            ("ayuToggleAnim", "solarToggleAnim"),
            ("// AyuGram includes", "// SolarGram includes"),
            ("AyuGram", "SolarGram"),
            ("@Radolyn", "SolarGram"),
        ]
        for rel in [
            "CMakeLists.txt",
            "solar/solar_ui_settings.h",
            "solar/solar_ui_settings.cpp",
            "ui/image/image_prepare.cpp",
            "ui/image/image_prepare.h",
            "ui/style/style_core_font.cpp",
            "ui/widgets/checkbox.cpp",
        ]:
            if patch_file(os.path.join(lib_ui_dir, rel), ui_replacements):
                changed += 1

    # 2. lib_tl: generate_tl.py -> is_solarNoforwards / is_solarRestricted
    lib_tl_gen = os.path.join(TELEGRAM_DIR, "lib_tl", "tl", "generate_tl.py")
    if patch_file(
        lib_tl_gen,
        [
            ("# --- AyuGram hook", "# --- SolarGram hook"),
            ("is_ayuNoforwards", "is_solarNoforwards"),
            ("is_ayuRestricted", "is_solarRestricted"),
        ],
    ):
        changed += 1

    # 3. lib_icu: ayugram::lib_icu -> solargram::lib_icu
    lib_icu_cmake = os.path.join(TELEGRAM_DIR, "lib_icu", "CMakeLists.txt")
    if patch_file(
        lib_icu_cmake,
        [
            ("ayugram::lib_icu", "solargram::lib_icu"),
            ("AyuGram", "SolarGram"),
            ("@Radolyn", "SolarGram"),
        ],
    ):
        changed += 1

    # 4. codegen: style/generator.cpp -> SolarUiSettings & solar/solar_ui_settings.h
    codegen_gen = os.path.join(TELEGRAM_DIR, "codegen", "codegen", "style", "generator.cpp")
    if patch_file(
        codegen_gen,
        [
            ("ayu/ayu_ui_settings.h", "solar/solar_ui_settings.h"),
            ("AyuUiSettings", "SolarUiSettings"),
            ("ayuIncluded", "solarIncluded"),
        ],
    ):
        changed += 1

    print(f"[solar_patch_submodules] Patched {changed} submodule item(s).")


if __name__ == "__main__":
    main()
