# C-primes

A multi-threaded Unix daemon in C which handles requests for large, random prime numbers of the type used in cryptographic key agreement. Randomness is via Mersenne Twister using a new 128-bit seed from /dev/urandom for each request. Requests and responses are via UDP sockets. Uses the GNU Multi Precision Arithmetic library @ https://gmplib.org/.

(Disclaimer: Mersenne is not a CSPRNG, so even though its state is reseeded from a CSPRNG for each request, please don't use this to secure sensitive information.)
