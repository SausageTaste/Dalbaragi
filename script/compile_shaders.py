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
    _work_for_one("simple_v.spv", "simple.vert", [])

    _work_for_one("simple_f.spv", "simple.frag", [])

    _work_for_one("simple_gamma_f.spv", "simple.frag", ["DAL_GAMMA_CORRECT"])


if "__main__" == __name__:
    main()
