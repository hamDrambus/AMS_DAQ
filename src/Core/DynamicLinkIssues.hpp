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

#pragma once
#include <ers/Issue.h>

ERS_DECLARE_ISSUE(core, DynamicLinkIssue, "Dynamic library name: " << dlName,
                  ((const char *)dlName))

ERS_DECLARE_ISSUE_BASE(core, CannotResolveModule, core::DynamicLinkIssue,
                       "Failed to resolve module - Reason: " << reason << " - ",
                       ((const char *)dlName), ((const char *)reason))

ERS_DECLARE_ISSUE_BASE(core, CannotOpenModule, core::DynamicLinkIssue,
                       "Failed to dlopen module - Reason: " << reason << " - ",
                       ((const char *)dlName), ((const char *)reason))
ERS_DECLARE_ISSUE(core, MissingCreateOrDelete, "Failed to resolve create and/or delete", ERS_EMPTY)