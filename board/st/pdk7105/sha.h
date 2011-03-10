/*---------------------------------------------------------------------------
//
//	Copyright(C) SMIT Corporation, 2000-2010.
//
//  File	:	sha.h
//	Purpose	:	SHA ALGORITHM
//	History :
//				2008-03-06 created by X.J.MENG.
//
---------------------------------------------------------------------------*/
#ifndef XYSSL_SHA_H
#define XYSSL_SHA_H

#ifdef __cplusplus
extern "C"{
#endif

#define SHA1_KEY_LENGTH            20
#define SHA256_KEY_LENGTH        32
extern unsigned char sha256_key[SHA256_KEY_LENGTH];

/////////////////////////////////////////////////////////////////////////
///////                     Api for CIPLUS                     //////////   
/////////////////////////////////////////////////////////////////////////


//sha1 algorithm
unsigned long soft_sha_1(unsigned char *i_pdata, 
						 	 unsigned long  i_udatalen,
           				   			unsigned char *o_pdata);


//sha256 algorithm
unsigned long soft_sha_256(unsigned char *i_pdata, 
							 	 unsigned long  i_udatalen,
           					 			unsigned char *o_pdata);


#ifdef __cplusplus
}
#endif

#endif

