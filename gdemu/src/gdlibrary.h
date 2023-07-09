/* godot-cpp integration testing project.
 *
 * This is free and unencumbered software released into the public domain.
 */

#ifndef GDEMU_REGISTER_TYPES_H
#define GDEMU_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_gdemu_module(ModuleInitializationLevel p_level);
void uninitialize_gdemu_module(ModuleInitializationLevel p_level);

#endif // GDEMU_REGISTER_TYPES_H
