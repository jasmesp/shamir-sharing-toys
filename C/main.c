#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <CommonCrypto/CommonRandom.h>



#define PRIME 2147483647  // A large prime number for mod operations

// Function to compute (base^exp) % mod
uint64_t mod_exp(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t result = 1;
    base = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1)
            result = (result * base) % mod;
        exp = exp >> 1;
        base = (base * base) % mod;
    }
    return result;
}

// Function to calculate modular inverse using Fermat's Little Theorem
uint64_t mod_inverse(uint64_t a, uint64_t mod) {
    return mod_exp(a, mod - 2, mod);
}

// Function to evaluate polynomial at x
uint64_t evaluate_polynomial(uint64_t *coefficients, int degree, uint64_t x) {
    uint64_t result = 0;
    for (int i = degree; i >= 0; i--) {
        result = (result * x + coefficients[i]) % PRIME;
    }
    return result;
}

// Function to interpolate and find the secret
uint64_t interpolate_secret(uint64_t *x, uint64_t *y, int k) {
    uint64_t secret = 0;
    for (int i = 0; i < k; i++) {
        uint64_t num = 1, den = 1;
        for (int j = 0; j < k; j++) {
            if (i != j) {
                num = (num * (0 - x[j] + PRIME)) % PRIME;
                den = (den * (x[i] - x[j] + PRIME)) % PRIME;
            }
        }
        uint64_t term = (y[i] * num % PRIME) * mod_inverse(den, PRIME) % PRIME;
        secret = (secret + term) % PRIME;
    }
    return secret;
}

void generate_shares(char *secret, int n, int k) {
    int secret_len = strlen(secret);
    uint64_t coefficients[k];
    
    // Convert the secret into a numeric value by summing ASCII values
    coefficients[0] = 0;
    for (int i = 0; i < secret_len; i++) {
        coefficients[0] = (coefficients[0] * 256 + (unsigned char)secret[i]) % PRIME;
    }

    // Generate random coefficients for the polynomial
    srand(time(NULL));
    for (int i = 1; i < k; i++) {
        coefficients[i] = rand() % PRIME;
    }

    // Generate and print shares
    for (int i = 1; i <= n; i++) {
        uint64_t share = evaluate_polynomial(coefficients, k - 1, i);
        printf("%d %llu\n", i, share);
    }
}

void reconstruct_secret(int k) {
    uint64_t x[k], y[k];

    // Read k shares
    for (int i = 0; i < k; i++) {
        scanf("%llu %llu", &x[i], &y[i]);
    }

    uint64_t secret_numeric = interpolate_secret(x, y, k);

    // Convert the numeric secret back to an alphanumeric string
    char secret[256];
    int idx = 0;
    while (secret_numeric > 0) {
        secret[idx++] = (char)(secret_numeric % 256);
        secret_numeric /= 256;
    }
    secret[idx] = '\0';

    // Reverse the string to restore the original order
    for (int i = 0; i < idx / 2; i++) {
        char temp = secret[i];
        secret[i] = secret[idx - i - 1];
        secret[idx - i - 1] = temp;
    }

    printf("Reconstructed secret: %s\n", secret);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <mode> [args]\n", argv[0]);
        fprintf(stderr, "Modes:\n");
        fprintf(stderr, "  generate <secret> <n> <k>  - Generate n shares with threshold k\n");
        fprintf(stderr, "  reconstruct <k>           - Reconstruct secret from k shares (stdin)\n");
        return 1;
    }

    if (strcmp(argv[1], "generate") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Usage: %s generate <secret> <n> <k>\n", argv[0]);
            return 1;
        }

        char *secret = argv[2];
        int n = atoi(argv[3]);
        int k = atoi(argv[4]);

        if (k > n) {
            fprintf(stderr, "Threshold k cannot be greater than the total number of shares n.\n");
            return 1;
        }

        generate_shares(secret, n, k);
    } else if (strcmp(argv[1], "reconstruct") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s reconstruct <k>\n", argv[0]);
            return 1;
        }

        int k = atoi(argv[2]);
        reconstruct_secret(k);
    } else {
        fprintf(stderr, "Invalid mode. Use 'generate' or 'reconstruct'.\n");
        return 1;
    }

    return 0;
}