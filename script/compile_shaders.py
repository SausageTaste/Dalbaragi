from ntpath import join
import os
import multiprocessing
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

    os.system(cmd_str)


class WorkDef:
    def __init__(self, output_file_name_ext, shader_src_name_ext, macro_definitions: Iterable[str]) -> None:
        self.m_output_file_name_ext = str(output_file_name_ext)
        self.m_shader_src_name_ext = str(shader_src_name_ext)
        self.m_macro_definitions = list(macro_definitions)


def _work_for_one_def(work: WorkDef):
    _work_for_one(work.m_output_file_name_ext, work.m_shader_src_name_ext, work.m_macro_definitions)


def main():
    jobs = [
        WorkDef("gbuf_v.spv", "gbuf.vert", []),

        WorkDef("gbuf_f.spv", "gbuf.frag", []),

        WorkDef("gbuf_animated_v.spv", "gbuf_animated.vert", []),

        WorkDef("fill_screen_v.spv", "fill_screen.vert", []),

        WorkDef("fill_screen_f.spv", "fill_screen.frag", []),

        WorkDef("composition_v.spv", "composition.vert", []),

        WorkDef("composition_f.spv", "composition.frag", [
            "DAL_VOLUMETRIC_ATMOS",
            "DAL_ATMOS_DITHERING",
        ]),

        WorkDef("composition_gamma_f.spv", "composition.frag", [
            "DAL_GAMMA_CORRECT",
            "DAL_VOLUMETRIC_ATMOS",
            "DAL_ATMOS_DITHERING",
        ]),

        WorkDef("alpha_v.spv", "alpha.vert", []),

        WorkDef("alpha_animated_v.spv", "alpha_animated.vert", []),

        WorkDef("alpha_f.spv", "alpha.frag", []),

        WorkDef("alpha_gamma_f.spv", "alpha.frag", ["DAL_GAMMA_CORRECT"]),

        WorkDef("shadow_v.spv", "shadow.vert", []),

        WorkDef("shadow_f.spv", "shadow.frag", []),

        WorkDef("shadow_animated_v.spv", "shadow_animated.vert", []),
    ]

    pool = multiprocessing.Pool()
    pool.map(_work_for_one_def, jobs)
    pool.close()
    pool.join()

    print("\nDone")


if "__main__" == __name__:
    main()
