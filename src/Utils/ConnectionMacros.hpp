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

#ifndef CONNECTION_MACROS_HPP
#define CONNECTION_MACROS_HPP
#include "Common/DataType.hpp"
#include "Core/ConnectionLoader.hpp"
#include "Core/ResourceFactory.hpp"
#include "Utils/Ers.hpp"
#include <boost/preprocessor.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/seq.hpp>

#define REGISTER_SENDER(TYPE)                                                                      \
  namespace {                                                                                      \
  struct BOOST_PP_CAT(SenderRegistrator, __LINE__) {                                               \
    BOOST_PP_CAT(SenderRegistrator, __LINE__)() {                                                  \
      ERS_DEBUG(0, "Hi from register sender, adding: " << (BOOST_PP_STRINGIZE(TYPE)));             \
      daqling::core::ConnectionLoader &instance = daqling::core::ConnectionLoader::instance();     \
      ERS_DEBUG(0, "Instance address:" << &instance);                                              \
      instance.addSender<TYPE>(BOOST_PP_STRINGIZE(TYPE));                                          \
    }                                                                                              \
  } BOOST_PP_CAT(registrator, __LINE__);                                                           \
  }
#define REGISTER_RECEIVER(TYPE)                                                                    \
  namespace {                                                                                      \
  struct BOOST_PP_CAT(ReceiverRegistrator, __LINE__) {                                             \
    BOOST_PP_CAT(ReceiverRegistrator, __LINE__)() {                                                \
      daqling::core::ConnectionLoader::instance().addReceiver<TYPE>(BOOST_PP_STRINGIZE(TYPE));     \
    }                                                                                              \
  } BOOST_PP_CAT(registrator, __LINE__);                                                           \
  }

#define REGISTER_QUEUE(TYPE)                                                                       \
  namespace {                                                                                      \
  struct BOOST_PP_CAT(QueueRegistrator, __LINE__) {                                                \
    BOOST_PP_CAT(QueueRegistrator, __LINE__)() {                                                   \
      daqling::core::ConnectionLoader::instance().addQueue<TYPE>(BOOST_PP_STRINGIZE(TYPE));        \
    }                                                                                              \
  } BOOST_PP_CAT(registrator, __LINE__);                                                           \
  }
#define REGISTER_RESOURCE(TYPE)                                                                    \
  namespace {                                                                                      \
  struct BOOST_PP_CAT(ResourceRegistrator, __LINE__) {                                             \
    BOOST_PP_CAT(ResourceRegistrator, __LINE__)() {                                                \
      daqling::core::ResourceFactory::instance().addResource<TYPE>(BOOST_PP_STRINGIZE(TYPE));      \
    }                                                                                              \
  } BOOST_PP_CAT(registrator, __LINE__);                                                           \
  }

#endif