/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "health_data.h"

#include "applib/ui/ui.h"

//! Creates a health hr detail window
//! @param HealthData pointer to the health data to be given to this card
//! @return A pointer to a newly allocated health hr detail window
Window *health_hr_detail_card_create(HealthData *health_data);

//! Destroys a health hr detail window
//! @param window Window pointer to health hr detail window
void health_hr_detail_card_destroy(Window *window);
