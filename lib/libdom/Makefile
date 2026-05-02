# lib/libdom/Makefile -- NetSurf libdom (W3C DOM Level 3 implementation)
#
# Upstream: https://github.com/netsurf-browser/libdom @ commit f69781e
# (v0.4.2). Copyright 2007-2020 J-M Bell + contributors, MIT-licensed
# (see COPYING).
#
# 95 .c files / ~36K LOC across:
#   - src/core/    : 18 files -- DOM core (Node, Element, Document, Attr,
#                    Text, NodeList, NamedNodeMap, ...)
#   - src/utils/   :  5 files -- hashtable, namespace, validate,
#                    character_valid, walk
#   - src/events/  : 14 files -- DOM Level 3 Events (Event, EventTarget,
#                    EventListener, MutationEvent, KeyboardEvent, MouseEvent,
#                    ...)
#   - src/html/    : 57 files -- HTML element subclasses (HTMLAnchorElement,
#                    HTMLBodyElement, HTMLInputElement, ...)
#   - src/bindings/hubbub/ : 1 file (parser.c, ~1090 LOC) -- bridges
#                    libhubbub token callbacks into libdom DOM construction
#
# We SKIP src/bindings/xml/ (depends on expat or libxml -- neither shipped
# in amiport).
#
# Source-analyzer audit verdict: CLEAN (2026-05-02). No source patches.
# Pure C99 with libnix Tier 1 stdlib only (assert, ctype, inttypes, stdbool,
# stddef, stdio, stdlib, string, time). No POSIX, no threading, no FP, no
# 64-bit math, no endianness assumptions, no struct-by-value returns >8
# bytes, no large local arrays, no static mutable globals, no non-ASCII bytes.
#
# Required by: ports/netsurf (Phase 1 final consumer) and future
#              lib/libsvgtiny.
#
# CPU TARGET: -m68040 -m68881 (NetSurf-Vampire dep stack convention; see
# lib/libwapcaplet/Makefile header for rationale).
#
# DEPENDS ON: lib/libwapcaplet, lib/libparserutils, lib/libhubbub (already
# shipped this branch). Link order on consumer side: -ldom -lhubbub
# -lparserutils -lwapcaplet.

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR            = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB        = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# -O1 promoted whole-archive 2026-05-02 after perf-optimizer audit. All
# 95 TUs are -O1-safe per crash-patterns #16:
#   - Zero struct-by-value returns >8 bytes (every libdom API returns by
#     pointer or scalar dom_exception)
#   - Zero soft-float pulls (verified via m68k-amigaos-nm libdom.a |
#     grep '__\(div\|mul\)\(sf\|df\)3' returns nothing)
#   - No large local arrays (heap-based design throughout)
#   - No alignment quirks (AMIPORT_ALIGN unused; offsetof not used for
#     pool/arena alignment)
# Estimated speedup: 1.4-1.8x on parse-heavy paths (hubbub binding +
# tree-walk dispatch dominates). bebbo-gcc 13.3 has stable -O1 codegen.
# Same whole-archive pattern as lib/libwapcaplet + lib/libparserutils +
# lib/libhubbub. See lib/libdom/PERF-REPORT.md for the full audit.
# -fno-strict-aliasing defensive (prevents aliasing-based misopts on the
#   pointer-heavy DOM node tree + the void*-cast tree-handler callbacks).
# -m68040 -m68881 matches NetSurf consumer ABI (the entire dep stack
#   matches the ports/netsurf ABI exactly to avoid mixed-CPU complexity).
# -std=c99 mandatory: source uses bool, uint*_t, designated initializers.
# -DNDEBUG strips assert() and verbose-debug fprintf paths.
# -D_DEFAULT_SOURCE -D_BSD_SOURCE match upstream Makefile (the AmigaOS
#   branch sets -U__STRICT_ANSI__ + -D_BSD_SOURCE; our toolchain doesn't
#   default to __STRICT_ANSI__ so the -U is unneeded).
# -I include/ for the public dom/ headers.
# -I src/ for internal cross-subsystem headers (e.g.
#   bindings/hubbub/parser.c includes src/core/document.h).
# -I lib/libwapcaplet/include for libwapcaplet.h
# -I lib/libparserutils/include for parserutils types referenced by string
#   helpers and the hubbub binding.
# -I lib/libhubbub/include for the hubbub callbacks consumed by
#   src/bindings/hubbub/parser.c.
CFLAGS  = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99 -Wall -Wextra
CFLAGS += -DNDEBUG -D_DEFAULT_SOURCE -D_BSD_SOURCE
CFLAGS += -Iinclude -Isrc
CFLAGS += -I../../lib/libwapcaplet/include
CFLAGS += -I../../lib/libparserutils/include
CFLAGS += -I../../lib/libhubbub/include

TARGET = libdom.a

OBJS = \
    src/core/attr.o \
    src/core/cdatasection.o \
    src/core/characterdata.o \
    src/core/comment.o \
    src/core/doc_fragment.o \
    src/core/document.o \
    src/core/document_type.o \
    src/core/element.o \
    src/core/entity_ref.o \
    src/core/implementation.o \
    src/core/namednodemap.o \
    src/core/node.o \
    src/core/nodelist.o \
    src/core/pi.o \
    src/core/string.o \
    src/core/text.o \
    src/core/tokenlist.o \
    src/core/typeinfo.o \
    src/events/custom_event.o \
    src/events/dispatch.o \
    src/events/document_event.o \
    src/events/event.o \
    src/events/event_listener.o \
    src/events/event_target.o \
    src/events/keyboard_event.o \
    src/events/mouse_event.o \
    src/events/mouse_multi_wheel_event.o \
    src/events/mouse_wheel_event.o \
    src/events/mutation_event.o \
    src/events/mutation_name_event.o \
    src/events/text_event.o \
    src/events/ui_event.o \
    src/html/html_anchor_element.o \
    src/html/html_applet_element.o \
    src/html/html_area_element.o \
    src/html/html_base_element.o \
    src/html/html_basefont_element.o \
    src/html/html_body_element.o \
    src/html/html_br_element.o \
    src/html/html_button_element.o \
    src/html/html_canvas_element.o \
    src/html/html_collection.o \
    src/html/html_directory_element.o \
    src/html/html_div_element.o \
    src/html/html_dlist_element.o \
    src/html/html_document.o \
    src/html/html_element.o \
    src/html/html_fieldset_element.o \
    src/html/html_font_element.o \
    src/html/html_form_element.o \
    src/html/html_frame_element.o \
    src/html/html_frameset_element.o \
    src/html/html_head_element.o \
    src/html/html_heading_element.o \
    src/html/html_hr_element.o \
    src/html/html_html_element.o \
    src/html/html_iframe_element.o \
    src/html/html_image_element.o \
    src/html/html_input_element.o \
    src/html/html_isindex_element.o \
    src/html/html_label_element.o \
    src/html/html_legend_element.o \
    src/html/html_li_element.o \
    src/html/html_link_element.o \
    src/html/html_map_element.o \
    src/html/html_menu_element.o \
    src/html/html_meta_element.o \
    src/html/html_mod_element.o \
    src/html/html_object_element.o \
    src/html/html_olist_element.o \
    src/html/html_opt_group_element.o \
    src/html/html_option_element.o \
    src/html/html_options_collection.o \
    src/html/html_paragraph_element.o \
    src/html/html_param_element.o \
    src/html/html_pre_element.o \
    src/html/html_quote_element.o \
    src/html/html_script_element.o \
    src/html/html_select_element.o \
    src/html/html_style_element.o \
    src/html/html_table_element.o \
    src/html/html_tablecaption_element.o \
    src/html/html_tablecell_element.o \
    src/html/html_tablecol_element.o \
    src/html/html_tablerow_element.o \
    src/html/html_tablesection_element.o \
    src/html/html_text_area_element.o \
    src/html/html_title_element.o \
    src/html/html_ulist_element.o \
    src/utils/character_valid.o \
    src/utils/hashtable.o \
    src/utils/namespace.o \
    src/utils/validate.o \
    src/utils/walk.o \
    src/bindings/hubbub/parser.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
