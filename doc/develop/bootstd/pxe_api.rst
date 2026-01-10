.. SPDX-License-Identifier: GPL-2.0+:

PXE Parser API
==============

The PXE parser handles configuration files in the extlinux/syslinux format,
which is widely used for boot menus on both local storage and network-boot
environments. This document describes the internal API for parsing these
configuration files and booting the selected operating system.

Background
----------

The extlinux configuration format provides a simple way to define multiple boot
options, each with its own kernel, initial ramdisk, device tree, and command
line arguments. A typical configuration file looks like this::

    menu title Boot Menu
    timeout 50
    default linux

    label linux
        menu label Ubuntu Linux
        kernel /vmlinuz
        initrd /initrd.img
        fdt /dtb/board.dtb
        fdtoverlays /dtb/overlay1.dtbo /dtb/overlay2.dtbo
        append root=/dev/sda1 quiet

The parser reads this configuration, presents a menu if appropriate, and boots
the selected label by loading the kernel and associated files into memory.

Traditional API
---------------

The traditional approach uses callback functions to load files as they are
needed during parsing and booting. This works well when the caller has direct
access to the storage device and wants the PXE code to handle all file
operations.

The entry point is ``pxe_process()``, which parses the configuration file,
handles the menu interaction, and boots the selected label. Before calling
this function, the caller must set up a context using ``pxe_setup_ctx()``,
providing a callback function that knows how to read files from the
appropriate source.

The callback function receives a filename and a memory address, and is
responsible for loading the file contents to that address. This abstraction
allows the same parsing code to work with local filesystems, network TFTP,
or any other file source.

When the parser encounters an ``include`` directive, it automatically calls
the callback to load the included file, then parses its contents. This
happens recursively for nested includes, up to a maximum depth of 16 levels.
The caller does not need to handle includes explicitly.

For cases where the caller wants to inspect the parsed configuration before
booting, ``pxe_probe()`` provides a way to parse and select a label without
immediately booting. The caller can then examine the selected label's
properties and call ``pxe_boot()`` when ready to proceed.

Callback-free API
-----------------

Some callers prefer to handle file loading themselves rather than providing
callbacks. This is particularly useful in environments where file access
requires special handling, or where the caller wants complete control over
memory allocation and file placement.

The callback-free API separates the boot process into distinct phases, giving
the caller full visibility into what files are needed and where they should
be loaded.

The first phase uses ``pxe_parse()`` to parse the configuration file and
return a context containing a menu structure. The caller must first load the
configuration file into memory at a known address, then pass that address
and size to the parser. The function allocates and initialises the context
internally, so there is no need to call ``pxe_setup_ctx()`` beforehand.

Note that ``pxe_parse()`` does not process ``include`` directives
automatically, since there is no callback to load files. The caller must
handle includes explicitly after parsing, as described below.

During parsing, the code collects information about all files referenced by
each label. These are stored in a files list within each label structure,
with each entry recording the file path and type. The types distinguish
between kernels, initial ramdisks, device trees, and device tree overlays,
allowing the caller to handle each appropriately.

After parsing, the caller can examine the menu structure to see what labels
are available and what files each one requires. For labels that use the
``include`` directive, the caller must load each included file and call
``pxe_parse_include()`` to merge its contents into the menu. The includes
list may grow as included files reference further includes, so the caller
should process includes in a loop until none remain.

The second phase involves loading the files for the selected label. The
caller iterates over the label's files list, loads each file to an
appropriate memory address, and calls ``pxe_load()`` to record where the
file was placed. This function simply stores the address and size in the
file structure for later use.

The final phase boots the selected label using ``pxe_boot()``. The caller
sets ``ctx->label`` to point to the selected label, and the function
automatically retrieves the kernel, initial ramdisk, and device tree
addresses from the files list. It then invokes the boot process, which
does not return on success.

File Types
----------

The files list uses an enumeration to distinguish between different file
types. ``PFT_KERNEL`` indicates the kernel image, which may be a raw binary,
a compressed image, or a FIT image containing multiple components.
``PFT_INITRD`` marks the initial ramdisk, which the kernel uses as a
temporary root filesystem during early boot. ``PFT_FDT`` identifies the
flattened device tree that describes the hardware to the kernel. Finally,
``PFT_FDTOVERLAY`` marks device tree overlay files that modify the base
device tree, typically used to enable optional hardware or adjust
configuration.

The caller can use these types to determine appropriate load addresses for
each file, or to apply special handling such as decompression or
verification.

Include Handling
----------------

Configuration files may use the ``include`` directive to incorporate
additional configuration from other files. When using the traditional API
with callbacks, includes are processed automatically during parsing.

With the callback-free API, includes require explicit handling. After the
initial parse, the menu's includes list contains entries for each include
directive encountered. Each entry records the path to the included file
and the nesting level.

The caller loads each included file, adds a null terminator to the buffer
since the parser expects null-terminated strings, and calls
``pxe_parse_include()`` to parse and merge the contents. This may add more
entries to the includes list if the included file itself contains include
directives. Processing continues until all includes have been handled.

The parser enforces a maximum nesting depth to prevent infinite recursion
from circular includes.

Example Usage
-------------

A typical use of the callback-free API follows this pattern::

    struct pxe_context *ctx;
    struct pxe_menu *menu;
    struct pxe_label *label;
    struct pxe_file *file;
    ulong addr = CONFIG_SYS_LOAD_ADDR;
    ulong file_addr;
    ulong size;

    /* Load and parse the configuration file */
    size = load_config_file("/extlinux/extlinux.conf", addr);
    ctx = pxe_parse(addr, size, "/extlinux/extlinux.conf");
    menu = ctx->cfg;

    /* Process any include directives */
    for (i = 0; i < menu->includes.count; i++) {
        const struct pxe_include *inc;

        inc = alist_get(&menu->includes, i, struct pxe_include);
        size = load_file(inc->path, addr);
        pxe_parse_include(ctx, inc, addr, size);
    }

    /* Select a label (here we just take the first one) */
    label = list_first_entry(&menu->labels, struct pxe_label, list);

    /* Load all files for this label */
    file_addr = KERNEL_LOAD_ADDR;
    alist_for_each(file, &label->files) {
        size = load_file(file->path, file_addr);
        pxe_load(file, file_addr, size);
        file_addr += ALIGN(size, SZ_64K);
    }

    /* Boot - pxe_boot() gets addresses from the files list */
    ctx->label = label;
    pxe_boot(ctx);

    /* Clean up (only reached if boot fails) */
    pxe_cleanup(ctx);

This approach gives the caller complete control over file loading while
still benefiting from the parser's understanding of the configuration
format.
