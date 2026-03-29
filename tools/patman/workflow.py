# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Google LLC
#

"""Workflow types and operations for patman series management"""

import enum


class Wtype(str, enum.Enum):
    """Types of workflow entry"""
    TODO = 'todo'
