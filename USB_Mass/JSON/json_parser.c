/**
 * json_parser.c - Lightweight JSON parser for embedded systems
 *
 * Design for Cortex-M0:
 *  - Bump-allocator (arena): no malloc/free, O(1) per allocation
 *  - In-place string unescape: modifies the input buffer
 *  - No libc dependency beyond string.h (memcpy, strlen, strcmp)
 *  - Explicit recursion with depth limit (default 32)
 *  - All internal state lives on the C stack
 */

/* Anonymous union compatibility (same reason as in json_parser.h) */
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#pragma anon_unions
#endif

#include "json_parser.h"
#include <stdio.h>    /* snprintf */
#include <string.h>   /* memcpy, strlen, strcmp */

/* ======================================================================== */
/*  Constants                                                               */
/* ======================================================================== */

#define JSON_DEFAULT_MAX_DEPTH  32
#define JSON_INIT_CAPACITY       4   /* initial capacity for arrays/objects   */

/* ======================================================================== */
/*  Arena (bump allocator)                                                  */
/* ======================================================================== */

typedef struct {
    char*   start;
    char*   end;
    char*   cur;
} arena_t;

static void arena_init(arena_t* a, void* pool, uint32_t sz) {
    a->start = (char*)pool;
    a->end   = a->start + sz;
    a->cur   = a->start;
}

/** Allocate zeroed, 4-byte-aligned memory from the arena. */
static void* arena_alloc(arena_t* a, uint32_t size) {
    /* align to 4 bytes */
    uint32_t aligned = (size + 3u) & ~3u;
    if ((uint32_t)(a->end - a->cur) < aligned) return NULL;
    void* p = a->cur;
    a->cur += aligned;
    /* zero-initialize */
    memset(p, 0, size);
    return p;
}

/* ======================================================================== */
/*  Parser state                                                            */
/* ======================================================================== */

typedef struct {
    const char*  input_start;   /* for error reporting */
    char*        p;             /* current read position */
    char*        end;           /* end of input */
    arena_t*     arena;
    uint32_t     depth;         /* current nesting depth */
    uint32_t     max_depth;
    int          error;
    char         error_msg[64];
} parser_t;

/* ---- Error helpers ----------------------------------------------------- */

static void set_error(parser_t* ps, const char* msg) {
    if (ps->error) return;
    ps->error = 1;
    /* compute line/column for context */
    uint32_t line = 1, col = 1;
    for (const char* c = ps->input_start; c < ps->p && c < ps->end; c++) {
        if (*c == '\n') { line++; col = 1; }
        else { col++; }
    }
    int n = snprintf(ps->error_msg, sizeof(ps->error_msg),
                     "[%u:%u] %s", line, col, msg);
    if (n < 0) {
        ps->error_msg[0] = '\0'; /* snprintf failed, shouldn't happen */
    }
}

/* ---- Low-level helpers ------------------------------------------------- */

static int is_ws(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void skip_ws(parser_t* ps) {
    while (ps->p < ps->end && is_ws(*ps->p)) ps->p++;
}


/* ======================================================================== */
/*  Unicode helpers                                                         */
/* ======================================================================== */

static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static uint16_t parse_hex4(const char* s) {
    uint16_t v = 0;
    for (int i = 0; i < 4; i++) {
        int d = hex_val(s[i]);
        if (d < 0) return 0xFFFF;
        v = (v << 4) | (uint16_t)d;
    }
    return v;
}

/** Encode a Unicode codepoint into UTF-8. Returns bytes written. */
static int put_utf8(uint32_t cp, char* out) {
    if (cp <= 0x7F) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp <= 0x7FF) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp <= 0xFFFF) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    if (cp <= 0x10FFFF) {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
    return 0; /* invalid codepoint */
}

/* ======================================================================== */
/*  String parsing (in-place unescape)                                      */
/* ======================================================================== */

/**
 * Parse a JSON string. The input must be positioned at the opening quote.
 * The string is unescaped in-place (writing over the source) and
 * null-terminated. Both the pointer and the length are returned.
 *
 * Returns 0 on success, -1 on error.
 */
static int parse_string(parser_t* ps, char** out_str, uint32_t* out_len) {
    if (ps->p >= ps->end || *ps->p != '"') {
        set_error(ps, "expected string");
        return -1;
    }
    ps->p++; /* skip '"' */

    char*       src = ps->p;
    char*       dst = ps->p;  /* in-place: dst never ahead of src */
    char* const end = ps->end;
    uint32_t    len = 0;

    while (src < end) {
        char c = *src;
        if (c == '"') {
            *dst = '\0';
            *out_str = ps->p;
            *out_len = len;
            ps->p = src + 1;  /* skip closing quote */
            return 0;
        }

        if (c == '\\') {
            src++;                     /* skip backslash */
            if (src >= end) { set_error(ps, "unterminated string escape"); return -1; }
            switch (*src) {
                case '"':  *dst++ = '"';  len++; break;
                case '\\': *dst++ = '\\'; len++; break;
                case '/':  *dst++ = '/';  len++; break;
                case 'b':  *dst++ = '\b'; len++; break;
                case 'f':  *dst++ = '\f'; len++; break;
                case 'n':  *dst++ = '\n'; len++; break;
                case 'r':  *dst++ = '\r'; len++; break;
                case 't':  *dst++ = '\t'; len++; break;
                case 'u': {
                    if (src + 4 >= end) { set_error(ps, "unterminated \\u"); return -1; }
                    uint32_t cp = parse_hex4(src + 1);
                    if (cp == 0xFFFF && (hex_val(src[1]) < 0 ||
                                         hex_val(src[2]) < 0 ||
                                         hex_val(src[3]) < 0 ||
                                         hex_val(src[4]) < 0)) {
                        set_error(ps, "invalid \\u escape");
                        return -1;
                    }
                    src += 4; /* src now at last hex digit */

                    /* Surrogate pair */
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        /* Expect \uXXXX low surrogate */
                        if (src + 2 < end && src[1] == '\\' && src[2] == 'u') {
                            uint16_t low = parse_hex4(src + 3);
                            if (low >= 0xDC00 && low <= 0xDFFF) {
                                cp = 0x10000u + (uint32_t)(cp - 0xD800) * 0x400u
                                               + (uint32_t)(low - 0xDC00);
                                src += 6; /* skip \uXXXX */
                            }
                        }
                    } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                        set_error(ps, "lone low surrogate");
                        return -1;
                    }

                    int n = put_utf8(cp, dst);
                    dst += n;
                    len += (uint32_t)n;
                    break;
                }
                default:
                    set_error(ps, "invalid escape character");
                    return -1;
            }
            src++;
        } else if ((uint8_t)c < 0x20) {
            set_error(ps, "control character in string");
            return -1;
        } else {
            *dst++ = c;
            len++;
            src++;
        }
    }

    set_error(ps, "unterminated string");
    return -1;
}

/* ======================================================================== */
/*  Number parsing                                                          */
/* ======================================================================== */

static double parse_number(parser_t* ps) {
    char* p = ps->p;
    char* end = ps->end;

    int sign = 1;
    if (*p == '-')      { sign = -1; p++; }
    else if (*p == '+') { p++; }

    /* Leading-zero check: "0" followed by another digit is invalid in JSON */
    if (p < end && *p == '0' && p + 1 < end) {
        char nx = *(p + 1);
        if (nx >= '0' && nx <= '9') {
            set_error(ps, "leading zeros not allowed in JSON numbers");
            return 0.0;
        }
    }

    /* Integer part (at least one digit required) */
    if (p >= end || *p < '0' || *p > '9') {
        set_error(ps, "invalid number");
        return 0.0;
    }
    double int_part = 0.0;
    while (p < end && *p >= '0' && *p <= '9') {
        int_part = int_part * 10.0 + (double)(*p - '0');
        p++;
    }

    /* Fractional part */
    double frac = 0.0;
    double div  = 1.0;
    if (p < end && *p == '.') {
        p++;
        if (p >= end || *p < '0' || *p > '9') {
            set_error(ps, "invalid number: trailing dot");
            return 0.0;
        }
        while (p < end && *p >= '0' && *p <= '9') {
            frac = frac * 10.0 + (double)(*p - '0');
            div  *= 10.0;
            p++;
        }
    }

    double val = sign * (int_part + frac / div);

    /* Exponent */
    if (p < end && (*p == 'e' || *p == 'E')) {
        p++;
        int esign = 1;
        if      (*p == '-') { esign = -1; p++; }
        else if (*p == '+') { p++; }

        if (p >= end || *p < '0' || *p > '9') {
            set_error(ps, "invalid number: incomplete exponent");
            return 0.0;
        }
        int exp = 0;
        while (p < end && *p >= '0' && *p <= '9') {
            exp = exp * 10 + (*p - '0');
            p++;
        }

        double factor = 1.0;
        int e = esign * exp;
        if (e >= 0) {
            for (int i = 0; i < e; i++) factor *= 10.0;
        } else {
            for (int i = 0; i > e; i--) factor /= 10.0;
        }
        val *= factor;
    }

    ps->p = p;
    return val;
}

/* ======================================================================== */
/*  Value parsing (forward declarations for mutual recursion)               */
/* ======================================================================== */

static json_value_t* parse_value(parser_t* ps);

/* ---- Array ------------------------------------------------------------- */

static json_value_t* parse_array(parser_t* ps) {
    json_value_t* val = (json_value_t*)arena_alloc(ps->arena, sizeof(json_value_t));
    if (!val) { set_error(ps, "out of memory"); return NULL; }
    val->type = JSON_ARRAY;
    val->array.count = 0;
    val->array.items = NULL;

    if (ps->p >= ps->end) { set_error(ps, "unterminated array"); return NULL; }
    ps->p++; /* skip '[' */

    skip_ws(ps);
    if (ps->p < ps->end && *ps->p == ']') {
        ps->p++;
        return val;
    }

    uint32_t cap = JSON_INIT_CAPACITY;
    val->array.items = (json_value_t*)arena_alloc(ps->arena,
                                                   cap * sizeof(json_value_t));
    if (!val->array.items) { set_error(ps, "out of memory"); return NULL; }

    for (;;) {
        /* Grow if needed */
        if (val->array.count >= cap) {
            uint32_t new_cap = cap * 2;
            json_value_t* new_items = (json_value_t*)arena_alloc(ps->arena,
                                           new_cap * sizeof(json_value_t));
            if (!new_items) { set_error(ps, "out of memory"); return NULL; }
            memcpy(new_items, val->array.items, cap * sizeof(json_value_t));
            val->array.items = new_items;
            cap = new_cap;
        }

        json_value_t* parsed = parse_value(ps);
        if (!parsed) return NULL;
        val->array.items[val->array.count] = *parsed;
        val->array.count++;

        skip_ws(ps);
        if (ps->p >= ps->end) { set_error(ps, "unterminated array"); return NULL; }
        if (*ps->p == ',') { ps->p++; skip_ws(ps); continue; }
        if (*ps->p == ']') { ps->p++; return val; }
        set_error(ps, "expected ',' or ']' in array");
        return NULL;
    }
}

/* ---- Object ------------------------------------------------------------ */

static json_value_t* parse_object(parser_t* ps) {
    json_value_t* val = (json_value_t*)arena_alloc(ps->arena, sizeof(json_value_t));
    if (!val) { set_error(ps, "out of memory"); return NULL; }
    val->type = JSON_OBJECT;
    val->object.count = 0;
    val->object.pairs = NULL;

    if (ps->p >= ps->end) { set_error(ps, "unterminated object"); return NULL; }
    ps->p++; /* skip '{' */

    skip_ws(ps);
    if (ps->p < ps->end && *ps->p == '}') {
        ps->p++;
        return val;
    }

    uint32_t cap = JSON_INIT_CAPACITY;
    val->object.pairs = (json_pair_t*)arena_alloc(ps->arena,
                                                   cap * sizeof(json_pair_t));
    if (!val->object.pairs) { set_error(ps, "out of memory"); return NULL; }

    for (;;) {
        if (val->object.count >= cap) {
            uint32_t new_cap = cap * 2;
            json_pair_t* new_pairs = (json_pair_t*)arena_alloc(ps->arena,
                                          new_cap * sizeof(json_pair_t));
            if (!new_pairs) { set_error(ps, "out of memory"); return NULL; }
            memcpy(new_pairs, val->object.pairs, cap * sizeof(json_pair_t));
            val->object.pairs = new_pairs;
            cap = new_cap;
        }

        json_pair_t* pair = &val->object.pairs[val->object.count];

        skip_ws(ps);
        if (ps->p >= ps->end) { set_error(ps, "unterminated object"); return NULL; }

        /* Key */
        if (*ps->p != '"') { set_error(ps, "expected string key"); return NULL; }
        if (parse_string(ps, &pair->key, &pair->key_len) < 0) return NULL;

        /* Colon */
        skip_ws(ps);
        if (ps->p >= ps->end || *ps->p != ':') {
            set_error(ps, "expected ':' in object");
            return NULL;
        }
        ps->p++;

        /* Value */
        skip_ws(ps);
        {
            json_value_t* parsed = parse_value(ps);
            if (!parsed) return NULL;
            pair->value = *parsed;
        }

        val->object.count++;

        skip_ws(ps);
        if (ps->p >= ps->end) { set_error(ps, "unterminated object"); return NULL; }
        if (*ps->p == ',') { ps->p++; skip_ws(ps); continue; }
        if (*ps->p == '}') { ps->p++; return val; }
        set_error(ps, "expected ',' or '}' in object");
        return NULL;
    }
}

/* ---- Value dispatcher -------------------------------------------------- */

static json_value_t* parse_value(parser_t* ps) {
    skip_ws(ps);
    if (ps->p >= ps->end) { set_error(ps, "unexpected end of input"); return NULL; }

    char c = *ps->p;

    /* Depth check */
    if (ps->depth >= ps->max_depth) {
        set_error(ps, "max depth exceeded");
        return NULL;
    }

    switch (c) {
    case '{': {
        ps->depth++;
        json_value_t* v = parse_object(ps);
        ps->depth--;
        return v;
    }
    case '[': {
        ps->depth++;
        json_value_t* v = parse_array(ps);
        ps->depth--;
        return v;
    }
    case '"': {
        json_value_t* v = (json_value_t*)arena_alloc(ps->arena, sizeof(json_value_t));
        if (!v) { set_error(ps, "out of memory"); return NULL; }
        v->type = JSON_STRING;
        if (parse_string(ps, &v->string.str, &v->string.len) < 0) return NULL;
        return v;
    }
    case 't':
        if (ps->end - ps->p >= 4 && ps->p[1] == 'r' && ps->p[2] == 'u' && ps->p[3] == 'e') {
            json_value_t* v = (json_value_t*)arena_alloc(ps->arena, sizeof(json_value_t));
            if (!v) { set_error(ps, "out of memory"); return NULL; }
            v->type = JSON_BOOL;
            v->bool_val = 1;
            ps->p += 4;
            return v;
        }
        set_error(ps, "invalid value");
        return NULL;

    case 'f':
        if (ps->end - ps->p >= 5 && ps->p[1] == 'a' && ps->p[2] == 'l' &&
            ps->p[3] == 's' && ps->p[4] == 'e') {
            json_value_t* v = (json_value_t*)arena_alloc(ps->arena, sizeof(json_value_t));
            if (!v) { set_error(ps, "out of memory"); return NULL; }
            v->type = JSON_BOOL;
            v->bool_val = 0;
            ps->p += 5;
            return v;
        }
        set_error(ps, "invalid value");
        return NULL;

    case 'n':
        if (ps->end - ps->p >= 4 && ps->p[1] == 'u' && ps->p[2] == 'l' && ps->p[3] == 'l') {
            json_value_t* v = (json_value_t*)arena_alloc(ps->arena, sizeof(json_value_t));
            if (!v) { set_error(ps, "out of memory"); return NULL; }
            v->type = JSON_NULL;
            ps->p += 4;
            return v;
        }
        set_error(ps, "invalid value");
        return NULL;

    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': {
        json_value_t* v = (json_value_t*)arena_alloc(ps->arena, sizeof(json_value_t));
        if (!v) { set_error(ps, "out of memory"); return NULL; }
        v->type = JSON_NUMBER;
        v->num_val = parse_number(ps);
        if (ps->error) return NULL;
        return v;
    }

    default:
        set_error(ps, "unexpected character");
        return NULL;
    }
}

/* ======================================================================== */
/*  Public API implementation                                               */
/* ======================================================================== */

/* Thread-local / global error state (safe for single-threaded embedded use) */
static const char* g_last_error = NULL;

json_value_t* json_parse(char* json_str, void* pool, uint32_t pool_size,
                         const json_opts_t* opts)
{
    g_last_error = NULL;

    if (!json_str || !pool || pool_size < 64) {
        g_last_error = "invalid arguments";
        return NULL;
    }

    arena_t arena;
    arena_init(&arena, pool, pool_size);

    parser_t ps;
    ps.input_start = json_str;
    ps.p           = json_str;
    ps.end         = json_str + strlen(json_str);
    ps.arena       = &arena;
    ps.depth       = 0;
    ps.max_depth   = opts && opts->max_depth ? opts->max_depth : JSON_DEFAULT_MAX_DEPTH;
    ps.error       = 0;
    ps.error_msg[0] = '\0';

    json_value_t* root = parse_value(&ps);

    /* Check for trailing non-whitespace characters */
    if (root && !ps.error) {
        skip_ws(&ps);
        if (ps.p < ps.end) {
            set_error(&ps, "trailing garbage after JSON value");
            return NULL;
        }
    }

    if (ps.error || !root) {
        g_last_error = ps.error_msg;
        return NULL;
    }

    return root;
}

/* ---- Accessors --------------------------------------------------------- */

json_type_t json_get_type(const json_value_t* v) {
    return v ? v->type : JSON_NULL;
}

int json_get_bool(const json_value_t* v) {
    if (v && v->type == JSON_BOOL) return v->bool_val;
    return 0;
}

double json_get_number(const json_value_t* v) {
    if (v && v->type == JSON_NUMBER) return v->num_val;
    return 0.0;
}

const char* json_get_string(const json_value_t* v, uint32_t* len) {
    if (v && v->type == JSON_STRING) {
        if (len) *len = v->string.len;
        return v->string.str;
    }
    if (len) *len = 0;
    return NULL;
}

uint32_t json_array_size(const json_value_t* v) {
    if (v && v->type == JSON_ARRAY) return v->array.count;
    return 0;
}

json_value_t* json_array_get(const json_value_t* v, uint32_t index) {
    if (v && v->type == JSON_ARRAY && index < v->array.count)
        return &v->array.items[index];
    return NULL;
}

uint32_t json_object_size(const json_value_t* v) {
    if (v && v->type == JSON_OBJECT) return v->object.count;
    return 0;
}

json_value_t* json_object_get(const json_value_t* v, const char* key) {
    if (!v || v->type != JSON_OBJECT || !key) return NULL;
    for (uint32_t i = 0; i < v->object.count; i++) {
        if (strcmp(v->object.pairs[i].key, key) == 0)
            return &v->object.pairs[i].value;
    }
    return NULL;
}

const char* json_get_error(void) {
    return g_last_error ? g_last_error : "no error";
}
