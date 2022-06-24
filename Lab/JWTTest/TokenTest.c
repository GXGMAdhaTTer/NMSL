#include "gxgfunc.h"
#include "l8w8jwt/encode.h"
#include "l8w8jwt/decode.h"

#define BUFFERSIZE 1024
static const char KEY[] = "Ni ma le ge bi de";

int encodeToken(char* userName, char* Token) {
    char* jwt;
    size_t jwt_length;

    struct l8w8jwt_encoding_params params;
    l8w8jwt_encoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;

    params.sub = NULL;
    params.iss = userName;

    params.aud = NULL;

    params.iat = time(NULL);
    params.exp = time(NULL) + 600; /* Set to expire after 10 minutes (600 seconds). */

    params.secret_key = (unsigned char*)KEY;
    params.secret_key_length = strlen(params.secret_key);

    params.out = &jwt;
    params.out_length = &jwt_length;

    int r = l8w8jwt_encode(&params);

    // printf("l8w8jwt example HS512 token: %s \n", r == L8W8JWT_SUCCESS ? jwt : " (encoding failure) ");
    strcpy(Token, jwt);
    // puts(Token);

    /* Always free the output jwt string! */
    l8w8jwt_free(jwt);

    return 0;
}

int decodeToken(char* userName, const char* JWT){
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
    params.validate_iss = userName; 
    // strcpy(params.validate_iss, userName);
    params.validate_sub =NULL;

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
        return 0;
    }
    else
    {
        printf("Example HS512 token validation failed! \n");
        return -1;
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

int main(void) {
    char Token[BUFFERSIZE] = {0};
    encodeToken("gxg", Token);
    puts(Token);

    int ret = decodeToken("gxg", Token);
    if(!ret){
        printf("是gxg\n");
    } else {
        printf("验证失败\n");
    }

}