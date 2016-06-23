#!/usr/bin/env python
# Copyright (C) 2016 Hewlett Packard Enterprise Development LP
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

from opsvalidator.base import BaseValidator
from opsvalidator import error
from opsvalidator.error import ValidationError

# This validator doesn't allow the user to disable ECMP as the support
# would be added in near future

class EcmpValidator(BaseValidator):
    resource = "system"

    def validate_modification(self, validation_args):
        system_row = validation_args.resource_row
        if hasattr(system_row, "ecmp_config"):
            ecmp_config = system_row.__getattr__("ecmp_config")
            enabled_value = ecmp_config.get("enabled", None)
            if enabled_value == "false":
                details = "ECMP cannot be disabled"
                raise ValidationError(error.VERIFICATION_FAILED, details)
