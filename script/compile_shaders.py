import os

import local_tools.path_tools as ptt


SHADER_STAGE_INITIALS = {
    "vert": "v",
    "frag": "f",
}


def main():
    shader_dir_path = os.path.join(ptt.find_repo_root_path(), "asset", "shader")
    for shader_src_name_ext in os.listdir(shader_dir_path):
        if shader_src_name_ext.endswith("spv"):
            continue

        file_name, extension = os.path.splitext(shader_src_name_ext)
        extension = extension.strip(".")
        if extension not in SHADER_STAGE_INITIALS.keys():
            continue

        output_file_name_ext = file_name + "_" + SHADER_STAGE_INITIALS[extension] + ".spv"
        src_path = os.path.join(shader_dir_path, shader_src_name_ext)
        dst_path = os.path.join(shader_dir_path, output_file_name_ext)

        command_text = "glslc.exe {} -o {}".format(src_path, dst_path)
        if 0 != os.system(command_text):
            print("- failed {}".format(shader_src_name_ext))
        else:
            print("- done {} -> {}".format(shader_src_name_ext, output_file_name_ext))


if "__main__" == __name__:
    main()
