# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Google LLC
#

"""Workflow types and operations for patman series management"""

from datetime import datetime, timedelta
import enum


class Wtype(str, enum.Enum):
    """Types of workflow entry"""
    TODO = 'todo'


def todo(cser, series, days):
    """Mark a series as a todo item after a number of days

    Args:
        cser (CseriesHelper): Series helper with open database
        series (str): Name of series to use, or None for current branch
        days (int): Number of days from now to mark as due
    """
    ser = cser._parse_series(series)
    cser.db.workflow_archive(Wtype.TODO, ser.idnum)
    when = cser.get_now() + timedelta(days=days)
    ts = when.strftime('%Y-%m-%d %H:%M:%S')
    cser.db.workflow_add(Wtype.TODO, ser.idnum, ts)
    cser.commit()
    print(f"Series '{ser.name}' marked for todo on {ts}")


def todo_clear(cser, series):
    """Clear the todo marker for a series

    Args:
        cser (CseriesHelper): Series helper with open database
        series (str): Name of series to use, or None for current branch
    """
    ser = cser._parse_series(series)
    cser.db.workflow_archive(Wtype.TODO, ser.idnum)
    cser.commit()
    print(f"Todo cleared for series '{ser.name}'")


def todo_list(cser, show_all):
    """List series that are due (or scheduled) for attention

    Args:
        cser (CseriesHelper): Series helper with open database
        show_all (bool): True to show all scheduled todos, not just
            those that are due
    """
    now = cser.get_now().strftime('%Y-%m-%d %H:%M:%S')
    before = None if show_all else now
    entries = cser.db.workflow_get_by_type(Wtype.TODO, before=before)
    if not entries:
        if show_all:
            print('No todos scheduled')
        else:
            print('No todos due')
        return
    print(f"{'Series':17}  {'Due':>14}  Description")
    print(f"{'-' * 17}  {'-' * 14}  {'-' * 30}")
    for _sid, name, desc, ts in entries:
        when = datetime.strptime(ts, '%Y-%m-%d %H:%M:%S')
        delta = when - cser.get_now()
        days = delta.days
        if days < 0:
            due = f'{-days}d overdue'
        elif days == 0:
            due = 'today'
        else:
            due = f'in {days}d'
        print(f"{name:17}  {due:>14}  {desc}")
