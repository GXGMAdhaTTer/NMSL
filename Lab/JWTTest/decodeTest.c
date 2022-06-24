#include "gxgfunc.h"

#include "l8w8jwt/decode.h"

static const char KEY[] = "Ni ma le ge bi de";
static const char JWT[] = "eyJhbGciOiJIUzUxMiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2NTE3MTk4NDIsImV4cCI6MTY1MTcyMDQ0Miwic3ViIjoiZ3hnIn0.THIUL7AOpK5hA7lXYieOne2W3QuNFYA4ZecQwhoCIPURHsL5BfKsmlJdiJniHpTrhd8yKQnRnQz-rg6BbYwReQ";

int main(void)
{
    struct l8w8jwt_decoding_params params;
    l8w8jwt_decoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;

    params.jwt = (char*)JWT;
    params.jwt_length = strlen(JWT);

    params.verification_key = (unsigned char*)KEY;
    params.verification_key_length = strlen(KEY);

    /* 
     * Not providing params.validate_iss_length makes it use strlen()
     * Only do this when using properly NUL-terminated C-strings! 
     */
    params.validate_iss = NULL; 
    params.validate_sub = "gxg";

    /* Expiration validation set to false here only because the above example token is already expired! */
    params.validate_exp = 0; 
    params.exp_tolerance_seconds = 60;

    params.validate_iat = 1;
    params.iat_tolerance_seconds = 60;

    enum l8w8jwt_validation_result validation_result;

    int decode_result = l8w8jwt_decode(&params, &validation_result, NULL, NULL);

    if (decode_result == L8W8JWT_SUCCESS && validation_result == L8W8JWT_VALID) 
    {
        printf("Example HS512 token validation successful! \n");
    }
    else
    {
        printf("Example HS512 token validation failed! \n");
    }
    
    /*
     * decode_result describes whether decoding/parsing the token succeeded or failed;
     * the output l8w8jwt_validation_result variable contains actual information about
     * JWT signature verification status and claims validation (e.g. expiration check).
     * 
     * If you need the claims, pass an (ideally stack pre-allocated) array of struct l8w8jwt_claim
     * instead of NULL,NULL into the corresponding l8w8jwt_decode() function parameters.
     * If that array is heap-allocated, remember to free it yourself!
     */

    return 0;
}