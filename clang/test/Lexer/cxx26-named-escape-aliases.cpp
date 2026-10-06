// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// expected-no-diagnostics

// P3733R1: a named universal character escape \N{...} designates a character by its name or by any of its character name
// aliases (no longer only those of type control, correction and alternate); this includes the abbreviations.

static_assert(__cpp_named_character_escapes == 202606L);

static_assert(U"\N{NBSP}"[0] == 0x00A0);      // abbreviation of NO-BREAK SPACE
static_assert(U"\N{LF}"[0] == 0x000A);        // abbreviation (and control alias)
static_assert(U"\N{ZWJ}"[0] == 0x200D);       // abbreviation of ZERO WIDTH JOINER
static_assert(U"\N{ZWNJ}"[0] == 0x200C);
static_assert(U"\N{BOM}"[0] == 0xFEFF);       // abbreviation
static_assert(U"\N{VS16}"[0] == 0xFE0F);      // abbreviation of VARIATION SELECTOR-16
static_assert(U"\N{LATIN SMALL LETTER A}"[0] == 0x61); // the name itself
static_assert(U"\N{LINE FEED}"[0] == 0x0A);   // control alias, as before
