import os

import local_tools.path_tools as ptt


SHADER_STAGE_SUFFIX_MAP = {
    "vert": "v",
    "frag": "f",
}


def main():
    glsl_dir_path = os.path.join(ptt.find_repo_root_path(), "asset", "glsl")
    spv_dir_path = os.path.join(ptt.find_repo_root_path(), "asset", "spv")

    for shader_src_name_ext in os.listdir(glsl_dir_path):
        src_path = os.path.join(glsl_dir_path, shader_src_name_ext)
        file_name, extension = os.path.splitext(shader_src_name_ext)
        extension = extension.strip(".")

        if extension not in SHADER_STAGE_SUFFIX_MAP.keys():
            print("- skipped {}".format(src_path))
            continue

        output_file_name_ext = file_name + "_" + SHADER_STAGE_SUFFIX_MAP[extension] + ".spv"
        dst_path = os.path.join(spv_dir_path, output_file_name_ext)

        command_text = "glslc.exe {} -o {}".format(src_path, dst_path)
        if 0 != os.system(command_text):
            print("- failed {}".format(shader_src_name_ext))
        else:
            print("- done {} -> {}".format(src_path, dst_path))


if "__main__" == __name__:
    main()
