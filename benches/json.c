// SPDX-License-Identifier: 0BSD
//
// cJSON parsing and printing a JSON document of 49 KB: an array of
// records shaped like a web API's reply, mostly short keys, strings and
// integers, with booleans, nulls, nested objects, lists of strings, of
// numbers and of mixed types, and numbers with fractions. cJSON is the JSON
// library of most small C programs, and its two halves stress different
// things:
//
//   json-parse  byte-at-a-time scanning and branchy dispatch, a malloc() for
//               every value and key, and a strtod() for every number
//   json-print  escaping strings into a growing buffer, and sprintf() for
//               every number, with sscanf() reading each fraction back
//
// They measure the C runtime about as much as the compiler: cJSON allocates
// with the runtime's malloc(), which FASTMEM does not reach.
//
// The fractions are multiples of 1/8 or 1/64, which a double holds exactly,
// so that every correct C runtime prints and parses the same digits and the
// check values agree. No number has more than 9 significant digits: the
// libnix of 16.2-rc22 and before reads some with 10 as 0.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#include "bench.h"

// The records in the document, and the size cJSON_Print() makes of them.
// setup() fails if the text comes out any other size, which would change
// the rate.
#define RECORDS 128
#define TEXT_SIZE 49930UL

struct json_state {
    cJSON *tree;    // the document as setup() built it
    char *text;     // and as cJSON_Print() wrote it, NUL included
    cJSON *parsed;  // json-parse's latest result
    char *printed;  // json-print's latest result
};

static struct json_state json_state;

static ULONG seed;

// A linear congruential generator, so the document is the same everywhere.
static ULONG next(ULONG n) {
    seed = seed * 1103515245UL + 12345UL;
    return (seed >> 8) % n;
}

// The people who made the Amiga and its ancestors at Atari, Amiga and
// Commodore, and the chips they made.
static const char *const WORDS[32] = {
    "Miner", "Sassenrath", "Luck", "Fish", "Mical", "Needle", "Decuir",
    "Nicholson", "Keller", "Haynie", "Herd", "Peddle", "Finkel", "Robbins",
    "Porter", "Sinz", "Nesbitt", "Jesup", "Katin", "Davis", "Amiga", "Paula",
    "Denise", "Agnus", "Gary", "Buster", "Lisa", "Alice", "Ramsey", "Lorraine",
    "Budgie", "Akiko",
};

// The notes, for the escapes cJSON prints and parses: quotes, backslashes,
// control characters and UTF-8, which it passes through unescaped.
static const char *const NOTES[4] = {
    "Ships in \"3-5\" days",
    "C:\\Tools\\json\tversion 1",
    "line one\nline two\x01",
    "caf\xc3\xa9 cr\xc3\xa8me br\xc3\xbbl\xc3\xa9\x65",
};

static const long CPUS[5] = {68000, 68020, 68030, 68040, 68060};

// A number with a fraction of up to six binary places, like 28.375 or
// 999.984375: up to six decimals, and 9 significant digits below 1000.
static double fraction(ULONG range) {
    return (double)next(range * 64) / 64;
}

// Every draw from next() is a statement of its own: C leaves the order in
// which a call's arguments are evaluated to the compiler, and a different
// order would make a different document.
static cJSON *make_record(int id) {
    cJSON *rec, *addr, *tags, *list;
    const char *w1, *w2, *w3;
    char buf[64];
    int i, n;

    rec = cJSON_CreateObject();
    cJSON_AddNumberToObject(rec, "id", id);
    w1 = WORDS[next(32)];
    w2 = WORDS[next(32)];
    sprintf(buf, "%s %s", w1, w2);
    cJSON_AddStringToObject(rec, "name", buf);
    w1 = WORDS[next(32)];
    w2 = WORDS[next(32)];
    w3 = WORDS[next(32)];
    sprintf(buf, "%s.%s@%s.example", w1, w2, w3);
    cJSON_AddStringToObject(rec, "email", buf);
    cJSON_AddBoolToObject(rec, "active", next(4) != 0);
    cJSON_AddNumberToObject(rec, "age", 18 + (int)next(70));
    cJSON_AddNumberToObject(rec, "balance", ((double)next(800000) - 400000) / 8);
    tags = cJSON_AddArrayToObject(rec, "tags");
    n = (int)next(5);
    for (i = 0; i < n; i++)
        cJSON_AddItemToArray(tags, cJSON_CreateString(WORDS[next(32)]));
    // A list of mixed types, then one of numbers, some of them integers.
    list = cJSON_AddArrayToObject(rec, "machine");
    cJSON_AddItemToArray(list, cJSON_CreateString(WORDS[next(32)]));
    cJSON_AddItemToArray(list, cJSON_CreateNumber(CPUS[next(5)]));
    cJSON_AddItemToArray(list, cJSON_CreateNumber(fraction(50)));
    cJSON_AddItemToArray(list, cJSON_CreateString("MHz"));
    cJSON_AddItemToArray(list, cJSON_CreateBool(next(2) != 0));
    list = cJSON_AddArrayToObject(rec, "readings");
    n = (int)next(7);
    for (i = 0; i < n; i++)
        cJSON_AddItemToArray(list, cJSON_CreateNumber(
            next(2) ? (double)next(1000) : fraction(1000)));
    addr = cJSON_AddObjectToObject(rec, "address");
    n = 1 + (int)next(999);
    w1 = WORDS[next(32)];
    sprintf(buf, "%d %s Street", n, w1);
    cJSON_AddStringToObject(addr, "street", buf);
    cJSON_AddStringToObject(addr, "city", WORDS[next(32)]);
    sprintf(buf, "%05d", (int)next(100000));
    cJSON_AddStringToObject(addr, "zip", buf);
    if (next(3) == 0)
        cJSON_AddStringToObject(rec, "note", NOTES[next(4)]);
    if (id > 1 && next(2))
        cJSON_AddNumberToObject(rec, "manager", 1 + (int)next((ULONG)id - 1));
    else
        cJSON_AddNullToObject(rec, "manager");
    return rec;
}

static void json_teardown(void *state) {
    struct json_state *s = state;

    cJSON_Delete(s->tree);
    cJSON_Delete(s->parsed);
    free(s->printed);
    bench_free(s->text);
    memset(s, 0, sizeof(*s));
}

// Build the document, print it, and parse the text back: the round trip
// must give the same document, or neither benchmark can run.
static void *json_setup(void) {
    struct json_state *s = &json_state;
    cJSON *roundtrip;
    char *text;
    ULONG len;
    int i;

    seed = 1;
    s->tree = cJSON_CreateArray();
    for (i = 1; i <= RECORDS; i++)
        cJSON_AddItemToArray(s->tree, make_record(i));
    text = cJSON_Print(s->tree);
    if (!text) {
        json_teardown(s);
        return NULL;
    }
    len = (ULONG)strlen(text);
    if (len != TEXT_SIZE) {
        printf("json: the document is %lu bytes, not %lu\n",
               (unsigned long)len, (unsigned long)TEXT_SIZE);
        free(text);
        json_teardown(s);
        return NULL;
    }
    // A copy in the harness's memory, which FASTMEM reaches.
    s->text = bench_alloc(len + 1, "json text");
    if (s->text)
        memcpy(s->text, text, len + 1);
    free(text);
    roundtrip = s->text ? cJSON_Parse(s->text) : NULL;
    if (!roundtrip || !cJSON_Compare(roundtrip, s->tree, 1)) {
        cJSON_Delete(roundtrip);
        json_teardown(s);
        return NULL;
    }
    cJSON_Delete(roundtrip);
    return s;
}

// Each loop also frees the tree the loop before it parsed, as a program
// that reads one document after another would.
static const void *parse_run(void *state) {
    struct json_state *s = state;

    cJSON_Delete(s->parsed);
    s->parsed = cJSON_ParseWithLength(s->text, TEXT_SIZE);
    return s->parsed;
}

// Fold a parsed tree into the check value: its shape, keys, strings and
// the bits of every number.
static ULONG hash_tree(ULONG h, const cJSON *item) {
    for (; item; item = item->next) {
        h = checksum(h, &item->type, sizeof(item->type));
        if (item->string)
            h = checksum(h, item->string, (ULONG)strlen(item->string));
        if (item->valuestring)
            h = checksum(h, item->valuestring, (ULONG)strlen(item->valuestring));
        if (cJSON_IsNumber(item))
            h = checksum(h, &item->valuedouble, sizeof(item->valuedouble));
        h = hash_tree(h, item->child);
    }
    return h;
}

static bool parse_verify(void *state, const void *result, ULONG *check) {
    struct json_state *s = state;

    if (!cJSON_Compare(result, s->tree, 1))
        return false;
    *check = hash_tree(0, result);
    return true;
}

// Likewise, each loop frees the text the loop before it printed.
static const void *print_run(void *state) {
    struct json_state *s = state;

    free(s->printed);
    s->printed = cJSON_Print(s->tree);
    return s->printed;
}

static bool print_verify(void *state, const void *result, ULONG *check) {
    struct json_state *s = state;

    if (strcmp(result, s->text))
        return false;
    *check = checksum(0, result, TEXT_SIZE);
    return true;
}

const struct Bench bench_json_parse = {
    /* .name= */ "json-parse",
    /* .desc= */ "cJSON parsing 49 KB of records into a tree",
    /* .unit= */ "KB",
    /* .per_loop= */ TEXT_SIZE / 1024.0,
    /* .setup= */ json_setup,
    /* .run= */ parse_run,
    /* .verify= */ parse_verify,
    /* .teardown= */ json_teardown,
};

const struct Bench bench_json_print = {
    /* .name= */ "json-print",
    /* .desc= */ "cJSON printing the same tree back to indented text",
    /* .unit= */ "KB",
    /* .per_loop= */ TEXT_SIZE / 1024.0,
    /* .setup= */ json_setup,
    /* .run= */ print_run,
    /* .verify= */ print_verify,
    /* .teardown= */ json_teardown,
};
