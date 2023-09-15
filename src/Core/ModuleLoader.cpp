/**
 * Copyright (C) 2019-2021 CERN
 *
 * DAQling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DAQling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DAQling. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ModuleLoader.hpp"
#include "Utils/Ers.hpp"

using namespace daqling::core;

ModuleLoader::ModuleLoader() : m_create{}, m_delete{}, m_loaded{false} {}

ModuleLoader::~ModuleLoader() {
  if (m_loaded) {
    unload();
  }
}

bool ModuleLoader::load(const std::string &name, const std::string &type) {
  // Load the shared object
  std::string pluginName = "libDaqlingModule" + type + ".so";
  m_handle = dlopen(pluginName.c_str(), RTLD_NOW);
  if (*m_handle == nullptr) {
    throw CannotOpenModule(ERS_HERE, name.c_str(), dlerror());
    m_handle.reset();
    return false;
  }
  // Resolve functions for module creation/destruction
  try {
    m_create = resolve<CreateFunc>("daqling_module_create");
    m_delete = resolve<DeleteFunc>("daqling_module_delete");
  } catch (ers::Issue &issue) {
    throw MissingCreateOrDelete(ERS_HERE, issue);
    // return false;
  }

  // create with correct number
  m_dp = m_create(name);
  m_loaded = true;
  return true;
}

bool ModuleLoader::unload() {
  if (m_loaded) {
    m_delete(*m_dp);
    dlclose(*m_handle);
    m_loaded = false;
    return true;
  }
  return false;
}
