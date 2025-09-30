#ifndef MINIOS_STDLIB_H
#define MINIOS_STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Type definitions
typedef struct {
    int quot;                   // Quotient
    int rem;                    // Remainder
} div_t;

typedef struct {
    long quot;                  // Quotient
    long rem;                   // Remainder
} ldiv_t;

// Constants
#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1
#define RAND_MAX        32767

// Memory allocation
void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

// Process control
void exit(int status) __attribute__((noreturn));
void abort(void) __attribute__((noreturn));
int atexit(void (*func)(void));

// Environment
char *getenv(const char *name);
int system(const char *command);

// String conversion
double atof(const char *str);
int atoi(const char *str);
long atol(const char *str);
long long atoll(const char *str);

double strtod(const char *str, char **endptr);
float strtof(const char *str, char **endptr);
long double strtold(const char *str, char **endptr);

long strtol(const char *str, char **endptr, int base);
long long strtoll(const char *str, char **endptr, int base);
unsigned long strtoul(const char *str, char **endptr, int base);
unsigned long long strtoull(const char *str, char **endptr, int base);

// Pseudo-random number generation
int rand(void);
void srand(unsigned int seed);

// Arithmetic
int abs(int x);
long labs(long x);
long long llabs(long long x);

div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);

// Searching and sorting
void *bsearch(const void *key, const void *base, size_t num, size_t size,
              int (*compar)(const void *, const void *));
void qsort(void *base, size_t num, size_t size,
           int (*compar)(const void *, const void *));

// Multibyte characters (minimal support)
int mblen(const char *str, size_t max);
int mbtowc(wchar_t *pwc, const char *str, size_t max);
int wctomb(char *str, wchar_t wchar);

size_t mbstowcs(wchar_t *dest, const char *src, size_t max);
size_t wcstombs(char *dest, const wchar_t *src, size_t max);

#ifdef __cplusplus
}
#endif

#endif /* MINIOS_STDLIB_H */