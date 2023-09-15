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

#include <ers/Issue.h>

namespace daqling {
ERS_DECLARE_ISSUE(connection, ZMQIssue, "", ERS_EMPTY)
ERS_DECLARE_ISSUE_BASE(connection, ReceiveFailed, connection::ZMQIssue,
                       "ZMQ recv() failed cause: " << what, ERS_EMPTY, ((const char *)what))

ERS_DECLARE_ISSUE_BASE(connection, SendFailed, connection::ZMQIssue,
                       "ZMQ send() failed cause: " << what, ERS_EMPTY, ((const char *)what))

ERS_DECLARE_ISSUE_BASE(connection, InvalidTransportType, connection::ZMQIssue,
                       "Unrecognized transport type: " << what, ERS_EMPTY, ((const char *)what))
ERS_DECLARE_ISSUE_BASE(connection, CannotAddChannel, connection::ZMQIssue,
                       "Failed to add channel! ZMQ returned: " << eWhat, ERS_EMPTY,
                       ((const char *)eWhat))
} // namespace daqling