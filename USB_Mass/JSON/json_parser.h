/**
 * json_parser.h - Lightweight JSON parser for embedded systems (Cortex-M0)
 *
 * Features:
 *  - No dynamic memory allocation (uses user-provided arena)
 *  - In-place string parsing (modifies input buffer)
 *  - Full JSON support: object, array, string, number, bool, null
 *  - Unicode escape (\uXXXX) with surrogate pair handling
 *  - Configurable recursion depth limit
 *  - Detailed error reporting
 *
 * Memory model:
 *  - All parser allocations come from a fixed-size arena passed to json_parse()
 *  - The input string is modified in-place during parsing
 *  - After use, simply discard the arena (no individual free needed)
 *
 * Usage:
 *   char buf[4096];
 *   json_value_t* root = json_parse(json_str, buf, sizeof(buf));
 *   if (!root) { printf("Error: %s\n", json_get_error()); return; }
 *   // access values...
 */

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <stdint.h>

/*
 * Anonymous unions are enabled for Keil MDK and other compilers that need
 * an explicit pragma. GCC/Clang support them by default; the pragma does
 * no harm there.
 */
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#pragma anon_unions
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---- JSON value types -------------------------------------------------- */

typedef enum {
    JSON_NULL   = 0,
    JSON_BOOL   = 1,
    JSON_NUMBER = 2,
    JSON_STRING = 3,
    JSON_ARRAY  = 4,
    JSON_OBJECT = 5
} json_type_t;

typedef struct json_value json_value_t;
typedef struct json_pair  json_pair_t;

/* ---- JSON value node --------------------------------------------------- */
/* A json_value_t occupies 16 bytes on ARM (with double alignment).       */

struct json_value {
    json_type_t type;         /* value type                                 */
    uint32_t    _pad;         /* explicit padding for alignment             */
    union {
        int      bool_val;    /* JSON_BOOL                                  */
        double   num_val;     /* JSON_NUMBER                                */
        struct {              /* JSON_STRING                                */
            char*      str;   /* null-terminated, unescaped in-place        */
            uint32_t   len;   /* byte length (excluding null terminator)    */
        } string;
        struct {              /* JSON_ARRAY                                 */
            json_value_t* items;
            uint32_t       count;
        } array;
        struct {              /* JSON_OBJECT                                */
            json_pair_t* pairs;
            uint32_t      count;
        } object;
    };
};

/* ---- Key-value pair (used in JSON_OBJECT) ------------------------------ */

struct json_pair {
    char*         key;        /* null-terminated key string                 */
    uint32_t      key_len;    /* key byte length                            */
    json_value_t  value;      /* value by-copy (no extra indirection)       */
};

/* ---- Options ----------------------------------------------------------- */

typedef struct {
    uint32_t max_depth;       /* max nesting depth (default 32, 0=use default) */
} json_opts_t;

/* ---- Main parse API ---------------------------------------------------- */

/**
 * Parse a JSON string.
 *
 * @param json_str   Null-terminated JSON string (WILL BE MODIFIED in-place).
 * @param pool       Memory arena for allocations.
 * @param pool_size  Size of the memory arena in bytes.
 * @param opts       Parser options, or NULL for defaults.
 * @return           Root value on success, NULL on error.
 *                   Call json_get_error() to retrieve the error message.
 */
json_value_t* json_parse(char* json_str, void* pool, uint32_t pool_size,
                         const json_opts_t* opts);

/* ---- Value accessors --------------------------------------------------- */

json_type_t json_get_type(const json_value_t* v);

/** JSON_BOOL: returns 1 for true, 0 for false (also 0 if type mismatch). */
int         json_get_bool(const json_value_t* v);

/** JSON_NUMBER: returns the number (0.0 if type mismatch). */
double      json_get_number(const json_value_t* v);

/**
 * JSON_STRING: returns pointer to null-terminated string.
 * @param len  [out] receives string byte length, may be NULL.
 * @return     string pointer, NULL if type mismatch.
 */
const char* json_get_string(const json_value_t* v, uint32_t* len);

/** JSON_ARRAY: number of elements. */
uint32_t    json_array_size(const json_value_t* v);

/** JSON_ARRAY: get element by index. Returns NULL if out of range. */
json_value_t* json_array_get(const json_value_t* v, uint32_t index);

/** JSON_OBJECT: number of keys. */
uint32_t    json_object_size(const json_value_t* v);

/**
 * JSON_OBJECT: get value by key (linear scan, O(n)).
 * Returns NULL if key not found or type mismatch.
 */
json_value_t* json_object_get(const json_value_t* v, const char* key);

/* ---- Error reporting --------------------------------------------------- */

/** Returns a human-readable error message from the last json_parse() call. */
const char* json_get_error(void);

#ifdef __cplusplus
}
#endif

#endif /* JSON_PARSER_H */
