.. SPDX-License-Identifier: GPL-2.0+:

.. index::
   single: font (command)

font command
============

Synopsis
--------

::

    font list
    font select [<name> [<size>]]
    font size [<size>]
    font info

Description
-----------

The *font* command allows selection of the font to use on the video console.
This is available when the TrueType console is in use.

font list
~~~~~~~~~

This lists the available fonts, using the name of the font file in the build.
Any enabled bitmap fonts are listed as well.

font select
~~~~~~~~~~~

This selects a new font and optionally changes the size. If the name is not
provided, the default font is used.

font size
~~~~~~~~~

This changes the font size only. With no argument it shows the current size.

font info
~~~~~~~~~

This shows glyph rendering statistics, specifically the number of glyphs
rendered since the video console was set up.

This subcommand requires CONFIG_VIDEO_GLYPH_STATS=y.

Examples
--------

::

    => font list
    nimbus_sans_l_regular
    cantoraone_regular
    => font size
    30
    => font size 40
    => font select cantoraone_regular 20
    =>

This shows an example of selecting a bitmap font when Truetype is active::

    => font list
    8x16
    12x22
    nimbus_sans_l_regular
    cantoraone_regular
    => font sel 8x16

This shows glyph rendering statistics::

    => font info
    glyphs rendered: 32705


Configuration
-------------

The command is only available if CONFIG_CONSOLE_TRUETYPE=y.

CONFIG_CONSOLE_TRUETYPE_GLYPH_BUF enables a pre-allocated buffer for glyph
rendering, avoiding malloc/free per character. The buffer starts at 4KB and
grows as needed via realloc().

CONFIG_VIDEO_GLYPH_STATS enables tracking of glyph-rendering statistics.

Return value
------------

The return value $? is 0 (true) if the command completes.
The return value is 1 (false) if the command fails.
