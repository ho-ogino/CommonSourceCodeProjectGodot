#include "gdlibrary.h"

#include <gdextension_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "gdemu.h"

using namespace godot;

void initialize_gdemu_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ClassDB::register_class<GDEmu>();
}

void uninitialize_gdemu_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

// extern "C" void GDE_EXPORT godot_gdnative_init(godot_gdnative_init_options *o) {
//     godot::Godot::gdnative_init(o);
// }
// 
// extern "C" void GDE_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *o) {
//     godot::Godot::gdnative_terminate(o);
// }
// 
// extern "C" void GDE_EXPORT godot_nativescript_init(void *handle) {
//     godot::Godot::nativescript_init(handle);
// 
//     godot::register_class<godot::GDEmu>();
// }

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT gdemu_library_init(GDExtensionInterfaceGetProcAddress p_interface, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);

	init_obj.register_initializer(initialize_gdemu_module);
	init_obj.register_terminator(uninitialize_gdemu_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
