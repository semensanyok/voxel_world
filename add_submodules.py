import subprocess
for path, url in [
    # ("dependencies/assimp", "https://github.com/assimp/assimp.git"),
    # ("dependencies/bullet3", "https://github.com/bulletphysics/bullet3.git"),
    # ("dependencies/glad", "https://github.com/Dav1dde/glad.git"),
    # ("dependencies/assimp", "https://github.com/assimp/assimp.git"),
    # ("dependencies/glm", "https://github.com/g-truc/glm.git"),
    # ("dependencies/stb", "https://github.com/nothings/stb.git"),
    # ("dependencies/zlib", "https://github.com/madler/zlib.git"),
    # ("dependencies/freetype", "https://github.com/freetype/freetype.git"),
    # ("dependencies/libRocket", "https://github.com/libRocket/libRocket.git"),
    ("dependencies/imgui", "https://github.com/ocornut/imgui.git"),
    # ("dependencies/gtk", "https://gitlab.gnome.org/GNOME/gtk.git")
    ]:
    subprocess.run(args=["git", "submodule", "add", url, path])
