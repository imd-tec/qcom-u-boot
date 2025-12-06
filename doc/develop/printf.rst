.. SPDX-License-Identifier: GPL-2.0+

Printf-style Functions
======================

U-Boot provides a family of printf-style functions for formatted output.

Functions
---------

printf()
        Prints formatted output to the console.

        .. code-block:: c

            int printf(const char *fmt, ...);

vprintf()
        Like printf() but takes a va_list argument.

        .. code-block:: c

            int vprintf(const char *fmt, va_list args);

sprintf()
        Prints formatted output to a string buffer. The buffer must be large
        enough to hold the output.

        .. code-block:: c

            int sprintf(char *buf, const char *fmt, ...);

vsprintf()
        Like sprintf() but takes a va_list argument.

        .. code-block:: c

            int vsprintf(char *buf, const char *fmt, va_list args);

snprintf()
        Prints formatted output to a string buffer with a size limit. At most
        size-1 characters are written, and the buffer is always null-terminated.
        Returns the number of characters that would have been written if the
        buffer were large enough.

        .. code-block:: c

            int snprintf(char *buf, size_t size, const char *fmt, ...);

vsnprintf()
        Like snprintf() but takes a va_list argument.

        .. code-block:: c

            int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);


Format Specification
--------------------

Each conversion specification consists of:

* leading '%' character
* zero or more flags
* an optional minimum field width
* an optional precision field preceded by '.'
* an optional length modifier
* a conversion specifier

Flags
-----

'space'
	fill up with spaces to reach the specified length

\-
	left justify

\+
	add sign field of decimal conversion

#
	convert to alternative form

	* prepend 0 to octal output
	* ignored for decimal output
	* prepend 0X to hexadecimal output

0
	fill up with zeroes to reach the specified length


Integer types
-------------

Length modifiers
''''''''''''''''

The optional length modifier specifies the size of the argument.

no modifier
	bool, enum, short, int are passed as int

%h
	convert to (unsigned) short before printing.
	Only the low 16 bits are printed.

%hh
	**not implemented**

%j
	**not implemented**

%l
	long

%ll, %L
	long long

%t
	ptr_diff_t

%z, %Z
	size_t, ssize_t

Conversion specifiers
'''''''''''''''''''''

Conversion specifiers control the output.

%d
	signed decimal

%u
	unsigned decimal

%o
	unsigned octal

%x
	unsigned lower case hexadecimal

%X
	unsigned upper case hexadecimal

The floating point conversion specifiers are not implemented:

* %a
* %A
* %e
* %E
* %f
* %F
* %g
* %G

The following tables shows the correct combinations of modifiers and specifiers
for the individual integer types.

=================== ==================
Type                Format specifier
=================== ==================
bool                %d, %x
char                %d, %x
unsigned char       %u, %x
short               %d, %x
unsigned short      %u, %x
int                 %d, %x
unsigned int        %u, %x
long                %ld, %lx
unsigned long       %lu, %lx
long long           %lld, %llx
unsigned long long  %llu, %llx
off_t               %llu, %llx
ptr_diff_t          %td, %tx
fdt_addr_t          %pa, see pointers
fdt_size_t          %pa, see pointers
phys_addr_t         %pa, see pointers
phys_size_t         %pa, see pointers
resource_size_t     %pa, see pointers
size_t              %zu, %zx, %zX
ssize_t             %zd, %zx, %zX
=================== ==================

Characters
----------

%%
	a '%' character is written

%c
        prints a single character

%lc
	**not implemented**

Strings
-------

%s
        prints a UTF-8 string (char \*)

%ls
        prints a UTF-16 string (u16 \*)

Pointers
--------

%p
        prints the address the pointer points to hexadecimally

%pa, %pap
        prints the value of a phys_addr_t value that the pointer points to
        preceded with 0x and zero padding according to the size of phys_addr_t.
	The following types should be printed this way:

	* fdt_addr_t
	* fdt_size_t
	* phys_addr_t
	* phys_size_t
	* resource_size_t

%pD
        prints a UEFI device path (requires CONFIG_EFI_DEVICE_PATH_TO_TEXT)

%pi4, %pI4
        prints IPv4 address, e.g. '192.168.0.1'. Lower case (%pi4) omits the
        dot separators.

%pi6, %pI6
        prints IPv6 address (requires CONFIG_IPV6). Lower case (%pi6) omits the
        colon separators.

%pm
        prints MAC address without separators, e.g. '001122334455'

%pM
        print MAC address colon separated, e.g. '00:01:02:03:04:05'

%pUb
        prints GUID big endian, lower case (requires CONFIG_LIB_UUID)
        e.g. '00112233-4455-6677-8899-aabbccddeeff'

%pUB
        prints GUID big endian, upper case (requires CONFIG_LIB_UUID)
        e.g. '00112233-4455-6677-8899-AABBCCDDEEFF'

%pUl
        prints GUID little endian, lower case (requires CONFIG_LIB_UUID)
        e.g. '33221100-5544-7766-8899-aabbccddeeff'

%pUL
        prints GUID little endian, upper case (requires CONFIG_LIB_UUID)
        e.g. '33221100-5544-7766-8899-AABBCCDDEEFF'

%pUs
        prints text description of a GUID or if such is not known little endian,
        lower case (requires CONFIG_LIB_UUID), e.g. 'system' for a GUID
        identifying an EFI system partition.
