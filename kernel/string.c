/********************************************************************************
* This software is licensed under the GNU General Public License:
* http://www.gnu.org/licenses/gpl.html
*
* MAVMM Project Group:
* Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
*
*********************************************************************************/

#include "string.h"

char *
strstr(register char *buf, register char *sub) {
    register char *bp;
    register char *sp;

    if (!*sub)
        return buf;
    while (*buf) {
        bp = buf;
        sp = sub;
        do {
            if (!*sp)
                return buf;
        } while (*bp++ == *sp++);
        buf += 1;
    }
    return 0;
}

int
strlen(const char *s) {
    int n;

    n = 0;
    while (*s++)
        n++;
    return (n);
}

int
strcmp(const char *cs, const char *ct) {
    signed char __res;

    while (1) {
        if ((__res = *cs - *ct++) != 0 || !*cs++)
            break;
    }
    return __res;
}

char *
strcpy(char *dest, const char *src) {
    char *tmp = dest;

    while ((*dest++ = *src++) != '\0')
        /* nothing */;
    return tmp;
}

int
strncmp(const char *cs, const char *ct, size_t count) {
    signed char __res = 0;

    while (count) {
        if ((__res = *cs - *ct++) != 0 || !*cs++)
            break;
        count--;
    }
    return __res;
}

// Return a pointer to the first occurrence of 'c' in 's',
// or a null pointer if the string has no 'c'.

char *
strchr(const char *s, char c) {
    for (; *s; s++)
        if (*s == c)
            return (char *) s;
    return 0;
}

char *
strncpy(char *dst, const char *src, size_t size) {
    size_t i;
    char *ret;

    ret = dst;
    for (i = 0; i < size; i++) {
        *dst++ = *src;
        // If strlen(src) < size, null-pad 'dst' out to 'size' chars
        if (*src != '\0')
            src++;
    }
    return ret;
}

char *
strrchr(const char *p, int ch) {
    char *save;

    for (save = NULL;; ++p) {
        if (*p == ch)
            save = (char *) p;
        if (!*p)
            return (save);
    }
    /* NOTREACHED */
}

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
const char strcase_charmap[] = {
    '\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
    '\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
    '\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
    '\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
    '\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
    '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
    '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
    '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
    '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
    '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
    '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
    '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
    '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
    '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
    '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
    '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
    '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
    '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
    '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
    '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
    '\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
    '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
    '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
    '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
    '\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
    '\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317',
    '\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
    '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
    '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
    '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
    '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
    '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

int strncasecmp(const char* s1, const char* s2, size_t len) {
    register unsigned int x2;
    register unsigned int x1;
//    register const char* end = s1 + len;

    const unsigned char *cm = (const unsigned char *) strcase_charmap;
    const unsigned char *us1 = (const unsigned char *) s1;
    const unsigned char *us2 = (const unsigned char *) s2;

    int n = len;
    while (n != 0 && cm[*us1] == cm[*us2++]) {
        if (*us1++ == '\0')
            return (0);
        n--;
    }
    return (n == 0 ? 0 : cm[*us1] - cm[*(us2 - 1)]);


    return x1 - x2;
}

/* Only used for special circumstances. Stolen from i386/string.h */
static inline void *
__inline_memcpy(void * to, const void * from, size_t n) {
    unsigned long d0, d1, d2;
    __asm__ __volatile__(
            "rep ; movsl\n\t"
            "testb $2,%b4\n\t"
            "je 1f\n\t"
            "movsw\n"
            "1:\ttestb $1,%b4\n\t"
            "je 2f\n\t"
            "movsb\n"
            "2:"
            : "=&c" (d0), "=&D" (d1), "=&S" (d2)
            : "0" (n / 4), "q" (n), "1" ((long) to), "2" ((long) from)
            : "memory");
    return (to);
}

void *
memcpy(void *to, const void * from, size_t n) {
    return __inline_memcpy(to, from, n);
}

void *
memmove(void *dest, const void *src, size_t count) {
    if (dest < src) {
        __inline_memcpy(dest, src, count);
    } else {
        char *p = (char *) dest + count;
        char *s = (char *) src + count;
        while (count--) {
            *--p = *--s;
        }
    }
    return dest;
}

void *
memset(void *s, int c, size_t count) {
    char *xs = s;

    while (count--)
        *xs++ = c;
    return s;
}
