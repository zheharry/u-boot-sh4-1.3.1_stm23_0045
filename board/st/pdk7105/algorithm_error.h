/*---------------------------------------------------------------------------
//
//	Copyright(C) SMIT Corporation, 2000-2010.
//
//  File	:	algorithm_error.h
//	Purpose	:	
//	History :
//				2008-03-06 created by X.J.MENG.
//
---------------------------------------------------------------------------*/
#ifndef XYSSL_ERROR_H
#define XYSSL_ERROR_H

#ifdef __cplusplus
extern "C"{
#endif

//the issue rang is from 0x00a01000  to 0x00a01033
#define ISSUE_ERROR_BASE 0x00a01000
enum{
//asn1 issue list
 XYSSL_ERR_ASN1_OUT_OF_DATA = ISSUE_ERROR_BASE,
 XYSSL_ERR_ASN1_UNEXPECTED_TAG,  
 XYSSL_ERR_ASN1_NO_CRITICAL_TAG,  
 XYSSL_ERR_ASN1_INVALID_LENGTH,                  
 XYSSL_ERR_ASN1_LENGTH_MISMATCH,                 
 XYSSL_ERR_ASN1_INVALID_DATA,                    
//x509 issue list
 XYSSL_ERR_X509_FEATURE_UNAVAILABLE,             
 XYSSL_ERR_X509_CERT_INVALID_PEM,    
 XYSSL_ERR_X509_CERT_INVALID_CERT,
 XYSSL_ERR_X509_CERT_INVALID_FORMAT,             
 XYSSL_ERR_X509_CERT_INVALID_VERSION,            
 XYSSL_ERR_X509_CERT_INVALID_SERIAL,             
 XYSSL_ERR_X509_CERT_INVALID_ALG,                
 XYSSL_ERR_X509_CERT_INVALID_NAME,               
 XYSSL_ERR_X509_CERT_INVALID_DATE,               
 XYSSL_ERR_X509_CERT_INVALID_PUBKEY,             
 XYSSL_ERR_X509_CERT_INVALID_SIGNATURE,          
 XYSSL_ERR_X509_CERT_INVALID_EXTENSIONS,         
 XYSSL_ERR_X509_CERT_UNKNOWN_VERSION,            
 XYSSL_ERR_X509_CERT_UNKNOWN_SIG_ALG,           
 XYSSL_ERR_X509_CERT_UNKNOWN_PK_ALG,             
 XYSSL_ERR_X509_CERT_SIG_MISMATCH,               
 XYSSL_ERR_X509_CERT_VERIFY_FAILED,              
 XYSSL_ERR_X509_KEY_INVALID_PEM,                 
 XYSSL_ERR_X509_KEY_INVALID_VERSION,             
 XYSSL_ERR_X509_KEY_INVALID_FORMAT,              
 XYSSL_ERR_X509_KEY_INVALID_ENC_IV,              
 XYSSL_ERR_X509_KEY_UNKNOWN_ENC_ALG,             
 XYSSL_ERR_X509_KEY_PASSWORD_REQUIRED,           
 XYSSL_ERR_X509_KEY_PASSWORD_MISMATCH,           
 XYSSL_ERR_X509_POINT_ERROR,                     
 XYSSL_ERR_X509_VALUE_TO_LENGTH,   
 XYSSL_ERR_X509_ISSUER_AND_SUBJECT_VERIFY,
 XYSSL_ERR_X509_EXT_BASIC_NO_VALID,
 XYSSL_ERR_X509_ISSUE_SUBJECT,
 XYSSL_ERR_X509_EXT_NO_VALID,
//rsa issue list
 XYSSL_ERR_RSA_BAD_INPUT_DATA, 
 XYSSL_ERR_RSA_INVALID_PUBLIC_KEY,
 XYSSL_ERR_RSA_INVALID_PADDING,                 
 XYSSL_ERR_RSA_KEY_GEN_FAILED,                  
 XYSSL_ERR_RSA_KEY_CHECK_FAILED,               
 XYSSL_ERR_RSA_PUBLIC_FAILED,                   
 XYSSL_ERR_RSA_PRIVATE_FAILED,                  
 XYSSL_ERR_RSA_VERIFY_FAILED,                   
//dh issue list
 XYSSL_ERR_DHM_BAD_INPUT_DATA,                  
 XYSSL_ERR_DHM_READ_PARAMS_FAILED,              
 XYSSL_ERR_DHM_MAKE_PARAMS_FAILED,              
 XYSSL_ERR_DHM_READ_PUBLIC_FAILED,              
 XYSSL_ERR_DHM_MAKE_PUBLIC_FAILED,             
 XYSSL_ERR_DHM_CALC_SECRET_FAILED,            
//mpi issue list
 XYSSL_ERR_MPI_FILE_IO_ERROR,                   
 XYSSL_ERR_MPI_BAD_INPUT_DATA,                  
 XYSSL_ERR_MPI_INVALID_CHARACTER,               
 XYSSL_ERR_MPI_BUFFER_TOO_SMALL,                
 XYSSL_ERR_MPI_NEGATIVE_VALUE,                  
 XYSSL_ERR_MPI_DIVISION_BY_ZERO,                
 XYSSL_ERR_MPI_NOT_ACCEPTABLE,                  
//base64 issue list
 XYSSL_ERR_BASE64_BUFFER_TOO_SMALL,             
 XYSSL_ERR_BASE64_INVALID_CHARACTER,    
//SHA-1 and SHA-256 issue list
XYSSL_ERR_SHA1_SIGN_LEN_INPUT_CHECK_FAILED ,
XYSSL_ERR_SHA1_SIGN_LEN_OUT_CHECK_FAILED,
XYSSL_ERR_SHA256_SIGN_LEN_INPUT_CHECK_FAILED,  
XYSSL_ERR_SHA256_SIGN_LEN_OUT_CHECK_FAILED,    
//DES issue list
XYSSL_ERR_DES_KEY_LEN_INPUT_CHECK_FAILED,
XYSSL_ERR_DES_DATE_LEN_OUT_CHECK_FAILED,
//AES issue list
XYSSL_ERR_AES_KEY_LEN_INPUT_CHECK_FAILED,
XYSSL_ERR_AES_VECTOR_LEN_INPUT_CHECK_FAILED,
XYSSL_ERR_AES_DATES_LEN_INPUT_CHECK_FAILED,
XYSSL_ERR_AES_DATES_LEN_OUT_PUT_CHECK_FAILED,
//RAND ISSUE LIST
XYSSL_ERR_RAND_DATES_GEN_OUT_FAILED,
//MGF
RE_MGF,               // Mask generation function not defined or not supported.
RE_MGF_LEN,           // Invalid mask generation function input or output length. 
XYSSL_ERR_NO_VALID_MEMORY_SPACE
};

#ifdef __cplusplus
}
#endif

#endif

