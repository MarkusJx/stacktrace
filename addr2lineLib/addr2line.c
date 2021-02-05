/* addr2line.c -- convert addresses to line number and function name
   Copyright (C) 1997-2020 Free Software Foundation, Inc.
   Contributed by Ulrich Lauther <Ulrich.Lauther@mchp.siemens.de>
   This file is part of GNU Binutils.
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA. */

/* Derived from objdump.c and nm.c by Ulrich.Lauther@mchp.siemens.de
   Usage:
   addr2line [options] addr addr ...
   or
   addr2line [options]
   both forms write results to stdout, the second form reads addresses
   to be converted from stdin.  */

/*
 * A library to get addr2line functionality without actually executing
 * the program itself. The main function is process_file, see addr2line.h
 * for further information about the params & how to use it.
 *
 * In order to compile this binutils-dev and libiberty-dev packages must be installed.
 *
 * Derived from addr2line.c by MarkusJx.
 *
 * Changes made:
 *  Removed all printf calls
 *  Added functions to make this a library
 *  Adapted imports and function calls to avoid building the whole thing
 *  Removed main
 *
 * Return codes:
 *      0: ok
 *      1: general error
 *      2: out of memory
 */

#define PACKAGE "stacktrace-lib"
#define PACKAGE_VERSION "1"

#include <stdlib.h>
#include <bfd.h>
#include <stdio.h>
#include <memory.h>

#ifndef __APPLE__
#   include <libiberty/demangle.h>
#endif

#include "addr2line.h"

#define TARGET "x86_64-pc-linux-gnu"

#define OK 0
// A general error
#define ERR_GENERAL 1
// A allocation error (out of memory etc.)
#define ERR_ALLOCATION 2

#define bfd_section_flags(section) section->flags

#ifndef __APPLE__
#   define bfd_section_vma(abfd, section) bfd_section_vma(section)
#   define bfd_section_size(abfd, section) bfd_section_size(section)
#   define DMGL_PARAMS (1 << 0)
#   define DMGL_ANSI (1 << 1)
#   define DMGL_NO_RECURSE_LIMIT (1 << 18)
#endif

static bfd_boolean unwind_inlines;    /* -i, unwind inlined functions. */
static bfd_boolean do_demangle;        /* -C, demangle names.  */

/* Flags passed to the name demangler.  */
static int demangle_flags = DMGL_PARAMS | DMGL_ANSI;

static int naddr;        /* Number of addresses to process.  */
static const char **addr;        /* Hex addresses to process.  */

static asymbol **syms;        /* Symbol table.  */

static int slurp_symtab(bfd *);

static void find_address_in_section(bfd *, asection *, void *);

static void find_offset_in_section(bfd *, asection *);

static void translate_addresses(bfd *, asection *, address_info *);

/* Read in the symbol table.  */

static int slurp_symtab(bfd *abfd) {
    long storage;
    long symcount;
    bfd_boolean dynamic = FALSE;

    if ((bfd_get_file_flags(abfd) & (unsigned) HAS_SYMS) == 0)
        return ERR_GENERAL;

    storage = bfd_get_symtab_upper_bound(abfd);
    if (storage == 0) {
        storage = bfd_get_dynamic_symtab_upper_bound(abfd);
        dynamic = TRUE;
    }

    if (storage < 0) return ERR_GENERAL;

    syms = (asymbol **) malloc(storage);
    if (!syms) return ERR_ALLOCATION;

    if (dynamic)
        symcount = bfd_canonicalize_dynamic_symtab(abfd, syms);
    else
        symcount = bfd_canonicalize_symtab(abfd, syms);
    if (symcount < 0) return ERR_GENERAL;

    /* If there are no symbols left after canonicalization and
       we have not tried the dynamic symbols then give them a go.  */
    if (symcount == 0 && !dynamic && (storage = bfd_get_dynamic_symtab_upper_bound(abfd)) > 0) {
        free(syms);
        syms = malloc(storage);
        if (!syms) return ERR_ALLOCATION;
        symcount = bfd_canonicalize_dynamic_symtab(abfd, syms);
    }

    /* PR 17512: file: 2a1d3b5b.
       Do not pretend that we have some symbols when we don't.  */
    if (symcount <= 0) {
        free(syms);
        syms = NULL;
    }

    return OK;
}

/* These global variables are used to pass information between
   translate_addresses and find_address_in_section.  */

static bfd_vma pc;
static const char *filename;
static const char *functionname;
static unsigned int line;
static unsigned int discriminator;
static bfd_boolean found;

/* Look for an address in a section.  This is called via
   bfd_map_over_sections.  */

static void find_address_in_section(bfd *abfd, asection *section, void *data ATTRIBUTE_UNUSED) {
    bfd_vma vma;
    bfd_size_type size;

    if (found)
        return;

    if ((bfd_section_flags(section) & (unsigned) SEC_ALLOC) == 0)
        return;

    vma = bfd_section_vma(abfd, section);
    if (pc < vma)
        return;

    size = bfd_section_size(abfd, section);
    if (pc >= vma + size)
        return;

    found = bfd_find_nearest_line_discriminator(abfd, section, syms, pc - vma, &filename, &functionname, &line,
                                                &discriminator);
}

/* Look for an offset in a section.  This is directly called.  */

static void find_offset_in_section(bfd *abfd, asection *section) {
    bfd_size_type size;

    if (found)
        return;

    if ((bfd_section_flags(section) & (unsigned) SEC_ALLOC) == 0)
        return;

    size = bfd_section_size(abfd, section);
    if (pc >= size)
        return;

    found = bfd_find_nearest_line_discriminator(abfd, section, syms, pc, &filename, &functionname, &line,
                                                &discriminator);
}

/* Read hexadecimal addresses from stdin, translate into
   file_name:line_number and optionally function name.  */

static void translate_addresses(bfd *abfd, asection *section, address_info *info) {
    while (naddr > 0) {
        --naddr;
        pc = bfd_scan_vma(*addr++, NULL, 16);

        // TODO: Maybe replace this
        if (bfd_get_flavour(abfd) == bfd_target_elf_flavour) {
            //const struct elf_backend_data *bed = get_elf_backend_data(abfd);
            //bfd_vma sign = (bfd_vma) 1 << (unsigned) (bed->s->arch_size - 1);
            bfd_vma sign = (bfd_vma) 1 << (unsigned) (abfd->arch_info->bits_per_address - 1);

            pc &= (sign << (unsigned) 1) - 1;
            //if (bed->sign_extend_vma)
            //    pc = (pc ^ sign) - sign;
        }

        // Set function address
        info[naddr].address = pc;

        found = FALSE;
        if (section)
            find_offset_in_section(abfd, section);
        else
            bfd_map_over_sections(abfd, find_address_in_section, NULL);

        if (found) {
            do {
                // Set function name
                {
                    const char *name;
                    char *alloc = NULL;

                    name = functionname;
                    if (name == NULL || *name == '\0') {
                        name = NULL;
                    } else if (do_demangle) {
                        alloc = bfd_demangle(abfd, name, demangle_flags);
                        if (alloc != NULL)
                            name = alloc;
                    }

                    if (name != NULL) {
                        strcpy(info[naddr].name, name);
                    }

                    free(alloc);
                }

                info[naddr].line = line;
                info[naddr].discriminator = discriminator;

                // Set file names
                if (filename != NULL) {
                    strcpy(info[naddr].filename, filename);

                    char *h;

                    h = strrchr(filename, '/');
                    if (h != NULL) {
                        filename = h + 1;
                        strcpy(info[naddr].basename, filename);
                    }
                }

                if (!unwind_inlines)
                    found = FALSE;
                else
                    found = bfd_find_inliner_info(abfd, &filename, &functionname, &line);
            } while (found);
        }
    }
}

/**
 * Get the size of a file.
 * Source: https://stackoverflow.com/a/6039648/14148409
 *
 * @param file_name the name of the file
 * @return the size of the file
 */
static long getFileSize(const char *file_name) {
    struct stat stat_buf;
    int rc = stat(file_name, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

static int initialized = FALSE;

/* Process a file.  Returns an exit value for main().  */

addr2line_result
process_file(const char *file_name, const char *section_name, const char *target, const char **_addr, int _naddr) {
    bfd *abfd;
    asection *section;
    char **matching;

    addr = _addr;
    naddr = _naddr;

    addr2line_result res;
    res.status = OK;
    res.info = NULL;
    res.err_msg = NULL;

    // Init bfd if not already initialized
    if (!initialized) {
        bfd_init();

        if (!bfd_ok()) {
            res.status = ERR_GENERAL;
            res.err_msg = "bfd init failed";

            return res;
        }

        if (!bfd_set_default_target(TARGET)) {
            res.status = ERR_GENERAL;
            res.err_msg = "bfd_set_default_target failed";

            return res;
        }

        initialized = TRUE;
    }

    if (getFileSize(file_name) < 1) {
        res.status = ERR_GENERAL;
        return res;
    }

    abfd = bfd_openr(file_name, target);
    if (abfd == NULL) {
        res.status = ERR_GENERAL;
        return res;
    }

    /* Decompress sections.  */
    abfd->flags |= (unsigned) BFD_DECOMPRESS;

    if (bfd_check_format(abfd, bfd_archive)) {
        res.status = ERR_GENERAL;
        res.err_msg = "cannot get addresses from archive";
        return res;
    }

    if (!bfd_check_format_matches(abfd, bfd_object, &matching)) {
        if (bfd_get_error() == bfd_error_file_ambiguously_recognized) {
            free(matching);
        }

        res.status = ERR_GENERAL;
        res.err_msg = "bfd format does not match";
        return res;
    }

    if (section_name != NULL) {
        section = bfd_get_section_by_name(abfd, section_name);
        if (section == NULL) fprintf(stderr, "%s: cannot find section %s", file_name, section_name);
    } else
        section = NULL;

    int stat = slurp_symtab(abfd);
    if (stat != OK) {
        res.err_msg = "Unable to read the symbol table";
        res.status = stat;
        return res;
    }

    // Allocate function info, return error if allocation fails
    address_info *info = calloc(naddr, sizeof(address_info));
    if (!info) {
        free(syms);
        syms = NULL;
        bfd_close(abfd);

        res.status = ERR_ALLOCATION;
        return res;
    }

    res.info = info;

    translate_addresses(abfd, section, info);

    free(syms);
    syms = NULL;

    bfd_close(abfd);

    return res;
}

void set_options(int _unwind_inlines, int no_recurse_limit, int demangle, const char *demangling_style) {
    unwind_inlines = _unwind_inlines;
    if (no_recurse_limit) {
        demangle_flags |= DMGL_NO_RECURSE_LIMIT;
    } else {
        demangle_flags &= ~DMGL_NO_RECURSE_LIMIT;
    }
    do_demangle = demangle;

#ifndef __APPLE__
    if (demangling_style != NULL) {
        enum demangling_styles style;

        style = cplus_demangle_name_to_style(demangling_style);
        if (style == unknown_demangling)
            fprintf(stderr, "unknown demangling style `%s'", demangling_style);
        else
            cplus_demangle_set_style(style);
    }
#endif
}

const char *bfd_getError() {
    return bfd_errmsg(bfd_get_error());
}

int bfd_ok() {
    return bfd_get_error() == bfd_error_no_error;
}
