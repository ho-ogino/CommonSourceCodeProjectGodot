#!python
import os
import sys

opts = Variables([], ARGUMENTS)

# Define the relative path to the Godot headers.
godot_headers_path = "godot-cpp/godot-headers"
godot_bindings_path = "godot-cpp"

# Try to detect the host platform automatically.
# This is used if no `platform` argument is passed
if sys.platform.startswith("linux"):
    host_platform = "linux"
elif sys.platform.startswith("freebsd"):
    host_platform = "freebsd"
elif sys.platform == "darwin":
    host_platform = "osx"
elif sys.platform == "win32" or sys.platform == "msys":
    host_platform = "windows"
else:
    raise ValueError("Could not detect platform automatically, please specify with platform=<platform>")


# Gets the standard flags CC, CCX, etc.
env = DefaultEnvironment()

is64 = sys.maxsize > 2 ** 32

# Define our options. Use future-proofed names for platforms.
platform_array = ["", "windows", "linuxbsd", "macos", "x11", "linux", "osx", "android", "ios"]
opts.Add(EnumVariable("target", "Compilation target", "debug", ["d", "debug", "r", "release"]))
opts.Add(EnumVariable("platform", "Compilation platform", "", platform_array))
opts.Add(EnumVariable("p", "Alias for 'platform'", "", platform_array))
opts.Add(BoolVariable("use_llvm", "Use the LLVM / Clang compiler", "no"))
opts.Add(PathVariable("target_path", "The path where the lib is installed.", "godotemu/bin/"))
opts.Add(PathVariable("target_name", "The library name.", "libgdemu", PathVariable.PathAccept))
opts.Add(BoolVariable("use_mingw", "Use the MinGW compiler instead of MSVC - only effective on Windows", False))
opts.Add(BoolVariable("godot", "for GDNative", True))
opts.Add(EnumVariable("bits", "Target platform bits", "64" if is64 else "32", ("32", "64")))

opts.Add(
    EnumVariable(
        "android_arch",
        "Target Android architecture",
        "armv7",
        ["armv7", "arm64v8", "x86", "x86_64"],
    )
)
opts.Add(
    "android_api_level",
    "Target Android API level",
    "18" if ARGUMENTS.get("android_arch", "armv7") in ["armv7", "x86"] else "21",
)
opts.Add(
    "ANDROID_NDK_ROOT",
    "Path to your Android NDK installation. By default, uses ANDROID_NDK_ROOT from your defined environment variables.",
    os.environ.get("ANDROID_NDK_ROOT", None),
)

emu_array = ["", "x1", "x1t", "msx", "pc88ma"]
opts.Add(EnumVariable("emu", "Target emulator", "", emu_array))

# Updates the environment with the option variables.
opts.Update(env)

# Process platform arguments. Here we use the same names as GDNative.
if env["p"] != "":
    env["platform"] = env["p"]

if host_platform == "windows" and env["platform"] != "android":
    if env["bits"] == "64":
        env = Environment(TARGET_ARCH="amd64")
    elif env["bits"] == "32":
        env = Environment(TARGET_ARCH="x86")

    opts.Update(env)

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
elif env["platform"] == "android":
    if host_platform == "windows":
        # Don't Clone the environment. Because otherwise, SCons will pick up msvc stuff.
        env = Environment(ENV=os.environ, tools=["mingw"])
        opts.Update(env)

        # Long line hack. Use custom spawn, quick AR append (to avoid files with the same names to override each other).
        #env["SPAWN"] = mySpawn
        env.Replace(ARFLAGS=["q"])

    # Verify NDK root
    if not "ANDROID_NDK_ROOT" in env:
        raise ValueError(
            "To build for Android, ANDROID_NDK_ROOT must be defined. Please set ANDROID_NDK_ROOT to the root folder of your Android NDK installation."
        )

    # Validate API level
    api_level = int(env["android_api_level"])
    if env["android_arch"] in ["x86_64", "arm64v8"] and api_level < 21:
        print("WARN: 64-bit Android architectures require an API level of at least 21; setting android_api_level=21")
        env["android_api_level"] = "21"
        api_level = 21

    # Setup toolchain
    toolchain = env["ANDROID_NDK_ROOT"] + "/toolchains/llvm/prebuilt/"
    if host_platform == "windows":
        toolchain += "windows"
        import platform as pltfm

        if pltfm.machine().endswith("64"):
            toolchain += "-x86_64"
    elif host_platform == "linux":
        toolchain += "linux-x86_64"
    elif host_platform == "osx":
        toolchain += "darwin-x86_64"
    env.PrependENVPath("PATH", toolchain + "/bin")  # This does nothing half of the time, but we'll put it here anyways

    # Get architecture info
    arch_info_table = {
        "armv7": {
            "march": "armv7-a",
            "target": "armv7a-linux-androideabi",
            "tool_path": "arm-linux-androideabi",
            "compiler_path": "armv7a-linux-androideabi",
            "ccflags": ["-mfpu=neon"],
        },
        "arm64v8": {
            "march": "armv8-a",
            "target": "aarch64-linux-android",
            "tool_path": "aarch64-linux-android",
            "compiler_path": "aarch64-linux-android",
            "ccflags": [],
        },
        "x86": {
            "march": "i686",
            "target": "i686-linux-android",
            "tool_path": "i686-linux-android",
            "compiler_path": "i686-linux-android",
            "ccflags": ["-mstackrealign"],
        },
        "x86_64": {
            "march": "x86-64",
            "target": "x86_64-linux-android",
            "tool_path": "x86_64-linux-android",
            "compiler_path": "x86_64-linux-android",
            "ccflags": [],
        },
    }
    arch_info = arch_info_table[env["android_arch"]]

    # Setup tools
    env["CC"] = toolchain + "/bin/clang"
    env["CXX"] = toolchain + "/bin/clang++"
    env["AR"] = toolchain + "/bin/llvm-ar"
    env["AS"] = toolchain + "/bin/llvm-as"
    env["LD"] = toolchain + "/bin/llvm-ld"
    env["STRIP"] = toolchain + "/bin/llvm-strip"
    env["RANLIB"] = toolchain + "/bin/llvm-ranlib"
    env["SHLIBSUFFIX"] = ".so"

    env.Append(
        CCFLAGS=[
            "--target=" + arch_info["target"] + env["android_api_level"],
            "-march=" + arch_info["march"],
            "-fPIC",
        ]
    )
    env.Append(CCFLAGS=arch_info["ccflags"])
    env.Append(LINKFLAGS=["--target=" + arch_info["target"] + env["android_api_level"], "-march=" + arch_info["march"]])

    if env["target"] == "debug":
        env.Append(CCFLAGS=["-Og", "-g"])
    elif env["target"] == "release":
        env.Append(CCFLAGS=["-O3"])


if env["godot"]:
    env.Append(CCFLAGS=["-D_GDNATIVE_"])

if env["use_llvm"] == "yes":
    env["CC"] = "clang"
    env["CXX"] = "clang++"

if env["emu"] == "msx":
    env.Append(CPPDEFINES=['_MSX1'])
elif env["emu"] == "pc88ma":
    env.Append(CPPDEFINES=['_PC8801MA'])
elif env["emu"] == "x1t":
    env.Append(CPPDEFINES=['_X1','_X1TURBO'])
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
elif env["emu"] == "pc88ma":
    sources.append("src/emulator/vm/disk.cpp")
    sources.append("src/emulator/vm/harddisk.cpp")
    sources.append("src/emulator/vm/event.cpp")
    sources.append("src/emulator/vm/prnfile.cpp")
    sources.append("src/emulator/vm/i8251.cpp")
    sources.append("src/emulator/vm/i8255.cpp")
    sources.append("src/emulator/vm/upd1990a.cpp")
    sources.append("src/emulator/vm/ym2203.cpp")
    sources.append("src/emulator/vm/z80.cpp")
    sources.append("src/emulator/vm/noise.cpp")
    sources.append("src/emulator/vm/pc80s31k.cpp")
    sources.append("src/emulator/vm/upd765a.cpp")
    sources.append("src/emulator/vm/scsi_dev.cpp")
    sources.append("src/emulator/vm/scsi_hdd.cpp")
    sources.append("src/emulator/vm/scsi_host.cpp")
    sources.append("src/emulator/vm/scsi_cdrom.cpp")
    sources.append("src/emulator/vm/ym2151.cpp")
    sources.append("src/emulator/vm/ay_3_891x.cpp")
    sources.append("src/emulator/vm/i8253.cpp")
    sources.append("src/emulator/vm/pcm1bit.cpp")
    sources.append("src/emulator/vm/pcm8bit.cpp")
    sources.append("src/emulator/vm/pc8801/pc88.cpp")
    sources.append("src/emulator/vm/pc8801/pc8801.cpp")
    sources.append("src/emulator/vm/pc8801/diskio.cpp")
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
    if env["emu"] == "x1t":
        sources.append("src/emulator/vm/z80dma.cpp")
    add_sources(sources, "src/emulator/vm/x1")

library = env.SharedLibrary(target=env["target_path"] + "/" + platform + "/" + env["target_name"], source=sources)
Default(library)
