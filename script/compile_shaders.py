import os
from typing import Iterable

import local_tools.path_tools as ptt


SHADER_STAGE_SUFFIX_MAP = {
    "vert": "v",
    "frag": "f",
}

GLSL_DIR = os.path.join(ptt.find_repo_root_path(), "asset", "glsl")
SPV_DIR = os.path.join(ptt.find_repo_root_path(), "asset", "spv")


def _make_cmd_str(dst_path: str, src_path: str, macro_definitions: Iterable[str]):
    macro_str = ""
    for x in macro_definitions:
        macro_str += "-D" + x + " "
    macro_str.rstrip()

    return "glslc.exe {} {} -o {}".format(src_path, macro_str, dst_path)


def _work_for_one(output_file_name_ext, shader_src_name_ext, macro_definitions: Iterable[str]):
    src_path = os.path.join(GLSL_DIR, shader_src_name_ext)
    dst_path = os.path.join(SPV_DIR, output_file_name_ext)

    cmd_str = _make_cmd_str(
        dst_path,
        src_path,
        macro_definitions,
    )

    if 0 != os.system(cmd_str):
        print("- failed {}".format(shader_src_name_ext))
    else:
        print("- done {} -> {}".format(src_path, dst_path))



def main():
    _work_for_one("gbuf_v.spv", "gbuf.vert", [])

    _work_for_one("gbuf_f.spv", "gbuf.frag", [])

    _work_for_one("gbuf_animated_v.spv", "gbuf_animated.vert", [])

    _work_for_one("fill_screen_v.spv", "fill_screen.vert", [])

    _work_for_one("fill_screen_f.spv", "fill_screen.frag", [])

    _work_for_one("composition_v.spv", "composition.vert", [])

    _work_for_one("composition_f.spv", "composition.frag", [])

    _work_for_one("composition_gamma_f.spv", "composition.frag", ["DAL_GAMMA_CORRECT"])

    _work_for_one("alpha_v.spv", "alpha.vert", [])

    _work_for_one("alpha_animated_v.spv", "alpha_animated.vert", [])

    _work_for_one("alpha_f.spv", "alpha.frag", [])

    _work_for_one("alpha_gamma_f.spv", "alpha.frag", ["DAL_GAMMA_CORRECT"])

    _work_for_one("shadow_v.spv", "shadow.vert", [])

    _work_for_one("shadow_f.spv", "shadow.frag", [])


if "__main__" == __name__:
    main()
