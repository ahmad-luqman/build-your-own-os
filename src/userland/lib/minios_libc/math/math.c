#include "../math.h"

// IEEE 754 double precision format helpers
typedef union {
    double d;
    struct {
        unsigned long long mantissa : 52;
        unsigned int exponent : 11;
        unsigned int sign : 1;
    } ieee;
    unsigned long long bits;
} ieee_double_t;

// Basic mathematical functions (simplified implementations)

double fabs(double x) {
    ieee_double_t u;
    u.d = x;
    u.ieee.sign = 0;
    return u.d;
}

float fabsf(float x) {
    return (float)fabs((double)x);
}

// Power function using repeated multiplication (simplified)
double pow(double x, double y) {
    if (y == 0.0) return 1.0;
    if (x == 0.0) return 0.0;
    if (y == 1.0) return x;
    if (y == -1.0) return 1.0 / x;
    
    // For integer powers, use repeated multiplication
    if (y == (int)y) {
        int n = (int)y;
        double result = 1.0;
        double base = x;
        
        if (n < 0) {
            n = -n;
            base = 1.0 / x;
        }
        
        while (n > 0) {
            if (n & 1) {
                result *= base;
            }
            base *= base;
            n >>= 1;
        }
        
        return result;
    }
    
    // For non-integer powers, return simplified approximation
    return x;  // Placeholder - would need proper exp/log implementation
}

float powf(float x, float y) {
    return (float)pow((double)x, (double)y);
}

// Square root using Newton's method
double sqrt(double x) {
    if (x < 0.0) return x;  // NaN for negative input
    if (x == 0.0) return 0.0;
    
    double guess = x;
    double prev_guess;
    
    // Newton's method: x_{n+1} = (x_n + a/x_n) / 2
    for (int i = 0; i < 20; i++) {
        prev_guess = guess;
        guess = (guess + x / guess) * 0.5;
        
        // Check for convergence
        if (fabs(guess - prev_guess) < 1e-15) {
            break;
        }
    }
    
    return guess;
}

float sqrtf(float x) {
    return (float)sqrt((double)x);
}

// Trigonometric functions (simplified Taylor series)
double sin(double x) {
    // Normalize x to [-pi, pi]
    while (x > M_PI) x -= 2 * M_PI;
    while (x < -M_PI) x += 2 * M_PI;
    
    // Taylor series: sin(x) = x - x³/3! + x⁵/5! - x⁷/7! + ...
    double result = 0.0;
    double term = x;
    double x_squared = x * x;
    
    for (int i = 1; i <= 15; i += 2) {
        result += term;
        term *= -x_squared / ((i + 1) * (i + 2));
    }
    
    return result;
}

float sinf(float x) {
    return (float)sin((double)x);
}

double cos(double x) {
    // cos(x) = sin(x + π/2)
    return sin(x + M_PI_2);
}

float cosf(float x) {
    return (float)cos((double)x);
}

double tan(double x) {
    double c = cos(x);
    if (fabs(c) < 1e-15) return 1.0 / 0.0;  // Infinity
    return sin(x) / c;
}

float tanf(float x) {
    return (float)tan((double)x);
}

// Exponential function (simplified)
double exp(double x) {
    if (x == 0.0) return 1.0;
    
    // Taylor series: e^x = 1 + x + x²/2! + x³/3! + ...
    double result = 1.0;
    double term = 1.0;
    
    for (int i = 1; i <= 20; i++) {
        term *= x / i;
        result += term;
        
        if (fabs(term) < 1e-15) break;
    }
    
    return result;
}

float expf(float x) {
    return (float)exp((double)x);
}

// Natural logarithm (simplified)
double log(double x) {
    if (x <= 0.0) return -1.0 / 0.0;  // -Infinity or NaN
    if (x == 1.0) return 0.0;
    
    // Use the identity: ln(x) = 2 * atanh((x-1)/(x+1)) for x > 0
    // Simplified implementation
    if (x < 1.0) {
        return -log(1.0 / x);
    }
    
    // For x close to 1, use Taylor series of ln(1+u) where u = x-1
    double u = x - 1.0;
    if (fabs(u) < 0.5) {
        double result = 0.0;
        double term = u;
        
        for (int i = 1; i <= 20; i++) {
            result += term / i;
            term *= -u;
        }
        
        return result;
    }
    
    // For larger x, use a different approximation
    return 0.693147 * (x - 1.0);  // Very rough approximation
}

float logf(float x) {
    return (float)log((double)x);
}

double log10(double x) {
    return log(x) / M_LN10;
}

float log10f(float x) {
    return (float)log10((double)x);
}

// Rounding functions
double floor(double x) {
    if (x >= 0) {
        return (double)(long long)x;
    } else {
        long long i = (long long)x;
        return (x == i) ? x : i - 1;
    }
}

float floorf(float x) {
    return (float)floor((double)x);
}

double ceil(double x) {
    if (x >= 0) {
        long long i = (long long)x;
        return (x == i) ? x : i + 1;
    } else {
        return (double)(long long)x;
    }
}

float ceilf(float x) {
    return (float)ceil((double)x);
}

double round(double x) {
    return floor(x + 0.5);
}

float roundf(float x) {
    return (float)round((double)x);
}

double trunc(double x) {
    return (double)(long long)x;
}

float truncf(float x) {
    return (float)trunc((double)x);
}

// Modulo function
double fmod(double x, double y) {
    if (y == 0.0) return x;  // NaN
    
    double quotient = trunc(x / y);
    return x - quotient * y;
}

float fmodf(float x, float y) {
    return (float)fmod((double)x, (double)y);
}

// Floating point manipulation
double frexp(double value, int *exp) {
    if (!exp) return value;
    
    ieee_double_t u;
    u.d = value;
    
    if (value == 0.0) {
        *exp = 0;
        return value;
    }
    
    *exp = (int)u.ieee.exponent - 1022;  // IEEE 754 bias is 1023
    u.ieee.exponent = 1022;  // Set exponent to create number in [0.5, 1)
    
    return u.d;
}

float frexpf(float value, int *exp) {
    double d = (double)value;
    double result = frexp(d, exp);
    return (float)result;
}

double ldexp(double x, int exp) {
    ieee_double_t u;
    u.d = x;
    
    if (x == 0.0) return x;
    
    int new_exp = (int)u.ieee.exponent + exp;
    if (new_exp <= 0) return 0.0;        // Underflow
    if (new_exp >= 2047) return 1.0/0.0; // Overflow to infinity
    
    u.ieee.exponent = (unsigned int)new_exp;
    return u.d;
}

float ldexpf(float x, int exp) {
    return (float)ldexp((double)x, exp);
}

double modf(double value, double *iptr) {
    if (!iptr) return value;
    
    *iptr = trunc(value);
    return value - *iptr;
}

float modff(float value, float *iptr) {
    if (!iptr) return value;
    
    double d_iptr;
    double result = modf((double)value, &d_iptr);
    *iptr = (float)d_iptr;
    return (float)result;
}

// Classification functions
int fpclassify(double x) {
    ieee_double_t u;
    u.d = x;
    
    if (u.ieee.exponent == 0) {
        return (u.ieee.mantissa == 0) ? FP_ZERO : FP_SUBNORMAL;
    } else if (u.ieee.exponent == 2047) {
        return (u.ieee.mantissa == 0) ? FP_INFINITE : FP_NAN;
    } else {
        return FP_NORMAL;
    }
}

int isfinite(double x) {
    int class = fpclassify(x);
    return class != FP_INFINITE && class != FP_NAN;
}

int isinf(double x) {
    return fpclassify(x) == FP_INFINITE;
}

int isnan(double x) {
    return fpclassify(x) == FP_NAN;
}

int isnormal(double x) {
    return fpclassify(x) == FP_NORMAL;
}

int signbit(double x) {
    ieee_double_t u;
    u.d = x;
    return u.ieee.sign;
}