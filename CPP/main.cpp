/*
C++ Improvement Suggestions:
1. Replace C-style strings with std::string for better string handling and memory safety
   - Replace char* with std::string in function parameters
   - Use string::length() instead of strlen()
   - Use string concatenation instead of manual ASCII manipulation

2. Use std::vector instead of fixed-size arrays
   - Replace uint64_t coefficients[k] with std::vector<uint64_t>
   - This provides dynamic sizing and bounds checking

3. Use std::random instead of C's rand()
   - std::random_device for true randomness
   - std::mt19937 for better random number generation
   - std::uniform_int_distribution for proper distribution

4. Use std::iostream instead of printf/scanf
   - Replace printf with std::cout
   - Replace scanf with std::cin
   - Use string streams for formatting

5. Use std::array for fixed-size arrays when stack allocation is preferred
   - Good for small arrays with known size at compile time

6. Use constructor initialization and RAII principles
   - Create a ShareGenerator class to handle share generation
   - Create a SecretReconstructor class for reconstruction

7. Use std::numeric_limits instead of magic numbers
   - Replace PRIME with constexpr from limits

8. Use references instead of pointers
   - Change pointer parameters to const references

9. Use std::optional for functions that might fail
   - Return std::optional<std::string> for reconstruction

10. Use enum class for modes instead of strings
    - Replace string comparison with type-safe enum
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <CommonCrypto/CommonRandom.h>

/*
C++ Include Improvements:
- Add <string>, <vector>, <random>, <iostream>, <array>, <optional>
- Replace C-style headers with C++ versions: 
  - <cstdio> instead of <stdio.h>
  - <cstdlib> instead of <stdlib.h>
  - <cstring> instead of <string.h>
  - <ctime> instead of <time.h>
  - <cstdint> instead of <stdint.h>
*/

/*
C++ Improvement:
- Replace #define with constexpr for compile-time constants
- Consider using std::numeric_limits for bounds
*/
#define PRIME 2147483647  // A large prime number for mod operations

/*
C++ Improvements for mod_exp:
- Consider making this a static member function of a utility class
- Use references instead of value parameters for better performance
- Add const correctness
*/
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

/*
C++ Improvements for mod_inverse:
- Make this part of the same utility class as mod_exp
- Add const correctness
*/
// Function to calculate modular inverse using Fermat's Little Theorem
uint64_t mod_inverse(uint64_t a, uint64_t mod) {
    return mod_exp(a, mod - 2, mod);
}

/*
C++ Improvements for evaluate_polynomial:
- Replace raw pointer with std::vector<uint64_t> or std::array
- Use const references for parameters
- Consider using std::span (C++20) for array view
*/
// Function to evaluate polynomial at x
uint64_t evaluate_polynomial(uint64_t *coefficients, int degree, uint64_t x) {
    uint64_t result = 0;
    for (int i = degree; i >= 0; i--) {
        result = (result * x + coefficients[i]) % PRIME;
    }
    return result;
}

/*
C++ Improvements for interpolate_secret:
- Replace raw pointers with std::vector or std::array
- Use const references for parameters
- Consider returning std::optional<uint64_t> for error cases
- Add bounds checking for array access
*/
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

/*
C++ Improvements for generate_shares:
- Replace char* with std::string for secret parameter
- Use std::vector for coefficients array
- Use std::random_device and std::mt19937 for better randomization
- Use std::cout instead of printf
- Consider making this a member function of ShareGenerator class
- Add error handling with exceptions or std::optional
*/
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

/*
C++ Improvements for reconstruct_secret:
- Use std::vector for x and y arrays
- Use std::string for secret storage
- Use std::cin instead of scanf
- Use std::cout instead of printf
- Consider making this a member function of SecretReconstructor class
- Add proper error handling with exceptions or std::optional
- Use std::string::push_back instead of manual char array manipulation
*/
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

/*
C++ Improvements for main:
- Use std::string for string handling
- Use enum class for modes instead of string comparison
- Use std::cout/cerr instead of fprintf
- Use std::stoi instead of atoi for better error handling
- Consider using a command line parsing library
- Add try-catch blocks for error handling
- Use std::string_view for string arguments (C++17)
*/
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