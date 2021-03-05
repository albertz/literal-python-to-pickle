
// c++ py-to-pickle.cpp -o py-to-pickle.bin
// ./a.out demo.txt demo.pkl
// python3 -c "import pickle; pickle.load(open('demo.pkl', 'rb'))"

// https://github.com/python/cpython/blob/master/Modules/_pickle.c
// load_dict

#include <string>
#include <stdio.h>

const int protocol = 3;

/* Pickle opcodes. These must be kept updated with pickle.py.
   Extensive docs are in pickletools.py. */
enum opcode {
    MARK            = '(',
    STOP            = '.',
    POP             = '0',
    POP_MARK        = '1',
    DUP             = '2',
    FLOAT           = 'F',
    INT             = 'I',
    BININT          = 'J',
    BININT1         = 'K',
    LONG            = 'L',
    BININT2         = 'M',
    NONE            = 'N',
    PERSID          = 'P',
    BINPERSID       = 'Q',
    REDUCE          = 'R',
    STRING          = 'S',
    BINSTRING       = 'T',
    SHORT_BINSTRING = 'U',
    UNICODE         = 'V',
    BINUNICODE      = 'X',
    APPEND          = 'a',
    BUILD           = 'b',
    GLOBAL          = 'c',
    DICT            = 'd',
    EMPTY_DICT      = '}',
    APPENDS         = 'e',
    GET             = 'g',
    BINGET          = 'h',
    INST            = 'i',
    LONG_BINGET     = 'j',
    LIST            = 'l',
    EMPTY_LIST      = ']',
    OBJ             = 'o',
    PUT             = 'p',
    BINPUT          = 'q',
    LONG_BINPUT     = 'r',
    SETITEM         = 's',
    TUPLE           = 't',
    EMPTY_TUPLE     = ')',
    SETITEMS        = 'u',
    BINFLOAT        = 'G',

    /* Protocol 2. */
    PROTO       = '\x80',
    NEWOBJ      = '\x81',
    EXT1        = '\x82',
    EXT2        = '\x83',
    EXT4        = '\x84',
    TUPLE1      = '\x85',
    TUPLE2      = '\x86',
    TUPLE3      = '\x87',
    NEWTRUE     = '\x88',
    NEWFALSE    = '\x89',
    LONG1       = '\x8a',
    LONG4       = '\x8b',

    /* Protocol 3 (Python 3.x) */
    BINBYTES       = 'B',
    SHORT_BINBYTES = 'C',

    /* Protocol 4 */
    SHORT_BINUNICODE = '\x8c',
    BINUNICODE8      = '\x8d',
    BINBYTES8        = '\x8e',
    EMPTY_SET        = '\x8f',
    ADDITEMS         = '\x90',
    FROZENSET        = '\x91',
    NEWOBJ_EX        = '\x92',
    STACK_GLOBAL     = '\x93',
    MEMOIZE          = '\x94',
    FRAME            = '\x95',

    /* Protocol 5 */
    BYTEARRAY8       = '\x96',
    NEXT_BUFFER      = '\x97',
    READONLY_BUFFER  = '\x98'
};

static void _PyFloat_Pack8(double x, unsigned char *p, int le) {
    typedef enum {
        unset, ieee_big_endian_format, ieee_little_endian_format
    } float_format_type;
    static float_format_type double_format = unset;

#if __SIZEOF_DOUBLE__ == 8
    if(double_format == unset) {
        double x = 9006104071832581.0;
        if (memcmp(&x, "\x43\x3f\xff\x01\x02\x03\x04\x05", 8) == 0)
            double_format = ieee_big_endian_format;
        else if (memcmp(&x, "\x05\x04\x03\x02\x01\xff\x3f\x43", 8) == 0)
            double_format = ieee_little_endian_format;
        else {
            fprintf(stderr, "invalid double format");
            abort();
        }
    }
#else
#error invalid __SIZEOF_DOUBLE__
#endif

    const unsigned char *s = (unsigned char*)&x;
    int i, incr = 1;

    if ((double_format == ieee_little_endian_format && !le)
        || (double_format == ieee_big_endian_format && le)) {
        p += 7;
        incr = -1;
    }

    for (i = 0; i < 8; i++) {
        *p = *s++;
        p += incr;
    }
}

static void _write_size64(char *out, size_t value) {
    size_t i;
    static_assert(sizeof(size_t) <= 8, "sizeof size_t != 8");

    for (i = 0; i < sizeof(size_t); i++) {
        out[i] = (unsigned char)((value >> (8 * i)) & 0xff);
    }
    for (i = sizeof(size_t); i < 8; i++) {
        out[i] = 0;
    }
}

static FILE *in, *out;

static void parse_error(const char* ctx, char c) {
    fprintf(stderr, "parse error: %s: char '%c' in pos %li\n", ctx, c, ftell(in));
    exit(1);
}

typedef std::pair<int,bool> ParseRes;  // next char + parsed one item or not

static ParseRes parse();
static void parse_list();
static void parse_dict();

static void parse_list() {
    fputc(EMPTY_LIST, out);
    fputc(MARK, out);
    while(true) {
        ParseRes res = parse();
        int c = res.first;
        if(c == ',') continue;
        else if(c == ']') break;
        else parse_error("list", c);
    }
    fputc(APPENDS, out);
}

static void parse_dict_or_set() {
    int c;
    size_t count = 0;
    enum {Dict, Set} obj_type = Dict;
    enum {Key, Value} cur = Key;
    long start_out_pos = ftell(out);
    auto make_set = [&]{
        if(count != 1) parse_error("dict after parsing more than one entry", c);
        cur = Value;
        obj_type = Set;
        int cur_pos = ftell(out);
        fseek(out, start_out_pos, SEEK_SET);
        fputc(EMPTY_SET, out);
        fseek(out, cur_pos, SEEK_SET);
    };
    fputc(EMPTY_DICT, out);
    fputc(MARK, out);
    while(true) {
        ParseRes res = parse();
        c = res.first;
        if(res.second) ++count;
        if(c == ',') {
            if(count == 1) make_set();
            if(cur == Key) parse_error("dict after parsing key", c);
            if(obj_type == Dict) cur = Key;
        }
        else if(c == ':') {
            if(cur != Key) parse_error("dict expected key before", c);
            if(obj_type == Set) parse_error("set", c);
            cur = Value;
        }
        else if(c == '}') {
            if(count == 1) make_set();
            break;
        }
        else parse_error("dict|set", c);
    }
    if(obj_type == Dict && count % 2 != 0) parse_error("dict, uneven count", c);
    fputc((obj_type == Dict) ? SETITEMS : ADDITEMS, out);
}

static void parse_str(char quote) {
    // We expect to already have utf8 here (i.e. input file is utf8).
    // This can potentially be sped up, by writing early,
    // and then filling in the size. (We might want to ignore BINUNICODE8.)
    int c;
    std::string buf;
    enum {Direct, EscapeInit, EscapeHex} escape_mode = Direct;
    int escape_hex_pos = 0;
    int escape_hex = 0;
    while(true) {
        c = fgetc(in);
        if(c < 0) parse_error("str, got EOF", c);
        if(escape_mode == Direct) {
            if(c == quote) break;
            if(c == '\\') escape_mode = EscapeInit;
            else buf.push_back((char) c);
        }
        else if(escape_mode == EscapeInit) {
            if(c == 'x') {
                escape_mode = EscapeHex;
                escape_hex_pos = 0;
                escape_hex = 0;
            }
            else {
                char c_;
                if(c == 'r') c_ = '\r';
                else if(c == 't') c_ = '\t';
                else if(c == 'n') c_ = '\n';
                else if(c == '\\' || c == '"' || c == '\'' || c == '\n') c_ = char(c);
                else parse_error("str escaped", c);
                buf.push_back(c_);
                escape_mode = Direct;
            }
        }
        else if(escape_mode == EscapeHex) {
            int h;
            if(c >= '0' && c <= '9') h = c - '0';
            else if(c >= 'a' && c <= 'f') h = c - 'a' + 10;
            else parse_error("str hex escaped", c);
            escape_hex *= 16;
            escape_hex += h;
            escape_hex_pos++;
            if(escape_hex_pos == 2) {
                buf.push_back(char(escape_hex));
                escape_mode = Direct;
            }
        }
    }

    char header[9];
    int len;
    size_t size = buf.length();
    if(size <= 0xff) {
        header[0] = SHORT_BINUNICODE;
        header[1] = (unsigned char)(size & 0xff);
        len = 2;
    }
    else if(size <= 0xffffffffUL) {
        header[0] = BINUNICODE;
        header[1] = (unsigned char)(size & 0xff);
        header[2] = (unsigned char)((size >> 8) & 0xff);
        header[3] = (unsigned char)((size >> 16) & 0xff);
        header[4] = (unsigned char)((size >> 24) & 0xff);
        len = 5;
    }
    else {
        header[0] = BINUNICODE8;
        _write_size64(header + 1, size);
        len = 9;
    }
    fwrite(header, len, 1, out);
    fwrite(buf.data(), size, 1, out);
}

static int parse_num(char first) {
    int c;
    std::string buf;
    bool is_float = false;
    buf.push_back(first);

    while(true) {
        c = fgetc(in);
        if(c < 0) break;

        if((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.') {
            if(c == '.')
                is_float = true;
            buf.push_back(c);
        }
        else
            break;
    }

    if(is_float) {
        double val = std::stod(buf);
        char pdata[9];
        pdata[0] = BINFLOAT;
        _PyFloat_Pack8(val, (unsigned char *)&pdata[1], 0);
        fwrite(pdata, sizeof(pdata), 1, out);
    }
    else {
        int len;
        long val = std::stol(buf);
        char pdata[8];
        pdata[1] = (unsigned char)(val & 0xff);
        pdata[2] = (unsigned char)((val >> 8) & 0xff);
        pdata[3] = (unsigned char)((val >> 16) & 0xff);
        pdata[4] = (unsigned char)((val >> 24) & 0xff);

        if ((pdata[4] != 0) || (pdata[3] != 0)) {
            pdata[0] = BININT;
            len = 5;
        }
        else if (pdata[2] != 0) {
            pdata[0] = BININT2;
            len = 3;
        }
        else {
            pdata[0] = BININT1;
            len = 2;
        }
        fwrite(pdata, len, 1, out);
    }
    return c;
}

static ParseRes parse() {
    int c;
    bool had_one_item = false;

    while(true) {
        c = fgetc(in);
        if(c < 0) break;

        if(isspace(c))
            continue;

        // (continued strings not yet supported...)
        if(had_one_item)
            break;
        
        if(c == '\'' || c == '"') {
            parse_str(c);
            had_one_item = true;
        }
        else if(c == '[') {
            parse_list();
            had_one_item = true;
        }
        else if(c == '{') {
            parse_dict_or_set();
            had_one_item = true;
        }
        else if((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.') {
            c = parse_num(c);
            had_one_item = true;
            if(c < 0 || !isspace(c))
                break;
        }
        else
            // some unexpected char
            break;
    }

    return ParseRes(c, had_one_item);
}

int main(int argc, char** argv) {
    if(argc <= 2) {
        printf("usage: %s <in-py-file> <out-pickle-file>\n", argv[0]);
        return -1;
    }

    in = fopen(argv[1], "r");
    out = fopen(argv[2], "w");
    setvbuf(out, NULL, _IOFBF, 0); // fully buffered out stream

    fputc(PROTO, out);
    fputc(protocol, out);

    ParseRes res = parse();
    if(!res.second) {
        fprintf(stderr, "invalid char %c\n", res.first);
        abort();
    }
    // fputc(NONE, out);

    fputc(STOP, out);

    fclose(out);
    fclose(in);
}
