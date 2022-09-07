#!python
import os

opts = Variables([], ARGUMENTS)

# Define the relative path to the Godot headers.
godot_headers_path = "godot-cpp/godot-headers"
godot_bindings_path = "godot-cpp"

# Gets the standard flags CC, CCX, etc.
env = DefaultEnvironment()

# Define our options. Use future-proofed names for platforms.
platform_array = ["", "windows", "linuxbsd", "macos", "x11", "linux", "osx"]
opts.Add(EnumVariable("target", "Compilation target", "debug", ["d", "debug", "r", "release"]))
opts.Add(EnumVariable("platform", "Compilation platform", "", platform_array))
opts.Add(EnumVariable("p", "Alias for 'platform'", "", platform_array))
opts.Add(BoolVariable("use_llvm", "Use the LLVM / Clang compiler", "no"))
opts.Add(PathVariable("target_path", "The path where the lib is installed.", "godotemu/bin/"))
opts.Add(PathVariable("target_name", "The library name.", "libgdemu", PathVariable.PathAccept))
opts.Add(BoolVariable("use_mingw", "Use the MinGW compiler instead of MSVC - only effective on Windows", False))
opts.Add(BoolVariable("godot", "for GDNative", True))

emu_array = ["", "x1", "msx"]
opts.Add(EnumVariable("emu", "Target emulator", "", emu_array))

# Updates the environment with the option variables.
opts.Update(env)

# Process platform arguments. Here we use the same names as GDNative.
if env["p"] != "":
    env["platform"] = env["p"]


if env["platform"] == "macos":
    env["platform"] = "osx"
elif env["platform"] in ("x11", "linuxbsd"):
    env["platform"] = "linux"
elif env["platform"] == "bsd":
    env["platform"] = "freebsd"

if env["platform"] == "":
    print("No valid target platform selected.")
    quit()

platform = env["platform"]

# Check our platform specifics.
if platform == "osx":
    if not env["use_llvm"]:
        env["use_llvm"] = "yes"

    if env["target"] in ("debug", "d"):
        env.Append(CCFLAGS=["-g", "-O2", "-arch", "x86_64", "-arch", "arm64", "-std=c++14"])
        env.Append(LINKFLAGS=["-arch", "x86_64", "-arch", "arm64"])
    else:
        env.Append(CCFLAGS=["-g", "-O3", "-arch", "x86_64", "-arch", "arm64",  "-std=c++14"])
        env.Append(LINKFLAGS=["-arch", "x86_64", "-arch", "arm64"])
    env.Append(LIBS=["z"])
elif platform == "linux":
    if env["target"] in ("debug", "d"):
        env.Append(CCFLAGS=["-fPIC", "-g3", "-Og"])
    else:
        env.Append(CCFLAGS=["-fPIC", "-g", "-O3"])
elif platform == "windows" and not env["use_mingw"]:
    # This makes sure to keep the session environment variables
    # on Windows, so that you can run scons in a VS 2017 prompt
    # and it will find all the required tools.
    env = Environment(ENV=os.environ)
    opts.Update(env)

    env.Append(CCFLAGS=["-DWIN32", "-D_WIN32", "-D_WINDOWS", "-W3", "-GR", "-D_CRT_SECURE_NO_WARNINGS"])
    if env["target"] in ("debug", "d"):
        env.Append(CCFLAGS=["-EHsc", "-D_DEBUG", "-MDd"])
    else:
        env.Append(CCFLAGS=["-O2", "-EHsc", "-DNDEBUG", "-MD"])
elif platform == "windows" and env["use_mingw"]:
    # Don't Clone the environment. Because otherwise, SCons will pick up msvc stuff.
    env = Environment(ENV=os.environ, tools=["mingw"])
    opts.Update(env)

    env.Append(CCFLAGS=["-DWIN32", "-D_WIN32", "-D_WINDOWS", "-D_CRT_SECURE_NO_WARNINGS"])
    # Still need to use C++14.
    env.Append(CCFLAGS=["-std=c++14"])
    # Don't want lib prefixes
    env["IMPLIBPREFIX"] = ""
    env["SHLIBPREFIX"] = ""

    env.Append(LIBS=["z"])
    # env["SPAWN"] = mySpawn
    env.Replace(ARFLAGS=["q"])

if env["godot"]:
    env.Append(CCFLAGS=["-D_GDNATIVE_"])

if env["use_llvm"] == "yes":
    env["CC"] = "clang"
    env["CXX"] = "clang++"

if env["emu"] == "msx":
    env.Append(CPPDEFINES=['_MSX1'])
else:
    env.Append(CPPDEFINES=['_X1'])


SConscript("godot-cpp/SConstruct")


def add_sources(sources, dir):
    for f in os.listdir(dir):
        if f.endswith(".cpp"):
            sources.append(dir + "/" + f)


env.Append(
    CPPPATH=[
        # "src/emulator/",
        # "src/emulator/vm/",
        # "src/emulator/vm/x1",
        godot_headers_path,
        godot_bindings_path + "/include",
        godot_bindings_path + "/include/gen/",
        godot_bindings_path + "/include/core/",
    ]
)

env.Append(
    LIBS=[
        env.File(os.path.join("godot-cpp/bin", "libgodot-cpp.%s.%s.64%s" % (platform, env["target"], env["LIBSUFFIX"])))
    ]
)

#if env["target"] in ("debug", "d"):
#    env.Append(LIBPATH=[godot_bindings_path + "/bin/", "src/emulator/zlib-1.2.11/debug/x64/"])
#else:
#    env.Append(LIBPATH=[godot_bindings_path + "/bin/", "src/emulator/zlib-1.2.11/release/x64/"])

if env["target"] in ("debug", "d"):
    env.Append(LIBPATH=[godot_bindings_path + "/bin/"])
else:
    env.Append(LIBPATH=[godot_bindings_path + "/bin/"])


sources = []
add_sources(sources, "src")
add_sources(sources, "src/emulator")
add_sources(sources, "src/emulator/godot")
add_sources(sources, "src/emulator/vm/fmgen")

if env["emu"] == "msx":
    sources.append("src/emulator/vm/event.cpp")
    sources.append("src/emulator/vm/disk.cpp")
    sources.append("src/emulator/vm/datarec.cpp")
    sources.append("src/emulator/vm/i8255.cpp")
    sources.append("src/emulator/vm/io.cpp")
    sources.append("src/emulator/vm/noise.cpp")
    sources.append("src/emulator/vm/not.cpp")
    sources.append("src/emulator/vm/ay_3_891x.cpp")
    sources.append("src/emulator/vm/pcm1bit.cpp")
    sources.append("src/emulator/vm/tms9918a.cpp")
    sources.append("src/emulator/vm/z80.cpp")
    sources.append("src/emulator/vm/ym2413.cpp")
    sources.append("src/emulator/vm/msx/msx_ex.cpp")
    sources.append("src/emulator/vm/msx/memory_ex.cpp")
    #sources.append("src/emulator/vm/msx/psg_stereo.cpp")
    sources.append("src/emulator/vm/msx/joystick.cpp")
    sources.append("src/emulator/vm/msx/keyboard.cpp")
    sources.append("src/emulator/vm/msx/rtcif.cpp")
    sources.append("src/emulator/vm/msx/kanjirom.cpp")
    sources.append("src/emulator/vm/msx/scc.cpp")
    sources.append("src/emulator/vm/msx/sound_cart.cpp")
else:
    sources.append("src/emulator/vm/ay_3_891x.cpp")
    sources.append("src/emulator/vm/event.cpp")
    sources.append("src/emulator/vm/datarec.cpp")
    sources.append("src/emulator/vm/hd46505.cpp")
    sources.append("src/emulator/vm/io.cpp")
    sources.append("src/emulator/vm/mb8877.cpp")
    sources.append("src/emulator/vm/noise.cpp")
    sources.append("src/emulator/vm/scsi_dev.cpp")
    sources.append("src/emulator/vm/scsi_hdd.cpp")
    sources.append("src/emulator/vm/scsi_host.cpp")
    sources.append("src/emulator/vm/ym2151.cpp")
    sources.append("src/emulator/vm/z80.cpp")
    sources.append("src/emulator/vm/z80ctc.cpp")
    sources.append("src/emulator/vm/z80sio.cpp")
    sources.append("src/emulator/vm/mcs48.cpp")
    sources.append("src/emulator/vm/upd1990a.cpp")
    sources.append("src/emulator/vm/disk.cpp")
    sources.append("src/emulator/vm/harddisk.cpp")
    sources.append("src/emulator/vm/i8255.cpp")
    add_sources(sources, "src/emulator/vm/x1")

library = env.SharedLibrary(target=env["target_path"] + "/" + platform + "/" + env["target_name"], source=sources)
Default(library)
