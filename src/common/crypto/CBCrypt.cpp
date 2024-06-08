/*
 * bcrypt wrapper library
 *
 * Written in 2011, 2013, 2014, 2015 by Ricardo Garcia <r@rg3.name>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */


#include "CBCrypt.h"

#include <bcrypt/ow-crypt.h>
#include <random>

#define BCRYPT_HASHSIZE	(64)

 /*
 * This is a best effort implementation. Nothing prevents a compiler from
 * optimizing this function and making it vulnerable to timing attacks, but
 * this method is commonly used in crypto libraries like NaCl.
 *
 * Return value is zero if both strings are equal and nonzero otherwise.
*/
static int timing_safe_strcmp(const char *str1, const char *str2)
{
	const unsigned char *u1;
	const unsigned char *u2;

	const size_t len1 = strlen(str1);
    const size_t len2 = strlen(str2);

	/* In our context both strings should always have the same length
	 * because they will be hashed passwords. */
	if (len1 != len2)
		return 1;

	/* Force unsigned for bitwise operations. */
	u1 = (const unsigned char *)str1;
	u2 = (const unsigned char *)str2;

	int ret = 0;
	for (size_t i = 0; i < len1; ++i)
		ret |= (u1[i] ^ u2[i]);

	return ret;
}

/*
 * This function expects a work factor between 4 and 31 and a char array to
 * store the resulting generated salt. The char array should typically have
 * BCRYPT_HASHSIZE bytes at least. If the provided work factor is not in the
 * previous range, it will default to 12.
 *
 * The return value is zero if the salt could be correctly generated and
 * nonzero otherwise.
 *
 */
int bcrypt_gensalt(const char* prefix, int factor, char salt[BCRYPT_HASHSIZE])
{
#define RANDBYTES (16)
	//char input[RANDBYTES];
    uint16_t input;
	int workf;
	char *aux;

    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<uint16_t> dist;
    input = dist(rng);

	/* Generate salt. */
	workf = (factor < 4 || factor > 31)?12:factor;
	aux = crypt_gensalt_rn(prefix, workf, reinterpret_cast<const char*>(&input), RANDBYTES, salt, BCRYPT_HASHSIZE);
	return (aux == nullptr)?5:0;
#undef RANDBYTES
}

/*
 * This function expects a password to be hashed, a salt to hash the password
 * with and a char array to leave the result. Both the salt and the hash
 * parameters should have room for BCRYPT_HASHSIZE characters at least.
 *
 * It can also be used to verify a hashed password. In that case, provide the
 * expected hash in the salt parameter and verify the output hash is the same
 * as the input hash. However, to avoid timing attacks, it's better to use
 * bcrypt_checkpw when verifying a password.
 *
 * The return value is zero if the password could be hashed and nonzero
 * otherwise.
 */
int bcrypt_hashpw(const char *passwd, const char salt[BCRYPT_HASHSIZE], char hash[BCRYPT_HASHSIZE])
{
	char *aux;
	aux = crypt_rn(passwd, salt, hash, BCRYPT_HASHSIZE);
	return (aux == nullptr)?1:0;
}

/*
 * This function expects a password and a hash to verify the password against.
 * The internal implementation is tuned to avoid timing attacks.
 *
 * The return value will be -1 in case of errors, zero if the provided password
 * matches the given hash and greater than zero if no errors are found and the
 * passwords don't match.
 *
 */
int bcrypt_checkpw(const char *passwd, const char hash[BCRYPT_HASHSIZE])
{
	int ret;
	char outhash[BCRYPT_HASHSIZE];

	ret = bcrypt_hashpw(passwd, hash, outhash);
	if (ret != 0)
		return -1;

	return timing_safe_strcmp(hash, outhash);
}


/* Sphere Methods*/

CSString CBCrypt::HashBCrypt(const char* password, int iPrefixCode, int iCost)  // static
{
    char salt[BCRYPT_HASHSIZE];
    char hash[BCRYPT_HASHSIZE];

    const char *pcPrefix;
    switch (iPrefixCode)
    {
        default:
        case 0:     pcPrefix = "$2a$";  break;
        case 1:     pcPrefix = "$2b$";  break;
        case 2:     pcPrefix = "$2y$";  break;
        case 3:     pcPrefix = "$1$";   break;
        case 4:     pcPrefix = "_";     break;
    }

    if (bcrypt_gensalt(pcPrefix, iCost, salt) != 0)
        return CSString();
    if (bcrypt_hashpw(password, salt, hash) != 0)
        return CSString();
    return CSString(hash);
}

bool CBCrypt::ValidateBCrypt(const char* password, const char* hash) // static
{
    return (bcrypt_checkpw(password, hash) == 0);
}
