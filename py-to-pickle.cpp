
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
            perror("invalid double format");
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

static FILE *in, *out;

static int parse();
static int parse_list();
static int parse_dict();

static int parse_list() {
    int c;
    fputc(EMPTY_LIST, out);
    fputc(MARK, out);
    while(true) {
        c = parse();
        if(c == ',') continue;
        if(c == ']') break;
    }
    fputc(APPENDS, out);
}

static int parse_dict() {
    int c;
    fputc(EMPTY_DICT, out);

}

static int parse_str(char quote) {

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
        fwrite(pdata, 9, 1, out);
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

static int parse() {
    int state = 0;
    while(true) {
        int c = fgetc(in);
        if(c < 0) return c;

        if(c == ' ' || c == '\t' || c == '\n')
            continue;
        else if(c == '[')
            parse_list();
        else if(c == '{')
            parse_dict();
        else if(c == '\'' || c == '"')
            parse_str(c);
        else if((c >= '0' && c <= '9') || c == '+' || c == '-')
            parse_num(c);
        else
            return c;
    }

}

int main(int argc, char** argv) {
    if(argc <= 2) {
        printf("usage: %s <in-py-file> <out-pickle-file>\n", argv[0]);
        return -1;
    }

    in = fopen(argv[1], "r");
    out = fopen(argv[2], "w");

    fputc(PROTO, out);
    fputc(protocol, out);

    int c = parse();
    if(c >= 0) {
        fprintf(stderr, "invalid char %c\n", c);
        perror("invalid char");
        abort();
    }
    // fputc(NONE, out);

    fputc(STOP, out);

    fclose(out);
    fclose(in);
}
