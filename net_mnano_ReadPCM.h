/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class net_mnano_ReadPCM */

#ifndef _Included_net_mnano_ReadPCM
#define _Included_net_mnano_ReadPCM
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     net_mnano_ReadPCM
 * Method:    pcmOpen
 * Signature: (ILjava/lang/String;III)I
 */
JNIEXPORT jint JNICALL Java_net_mnano_ReadPCM_pcmOpen
  (JNIEnv *, jclass, jint, jstring, jint, jint, jint);

/*
 * Class:     net_mnano_ReadPCM
 * Method:    readPcmStart
 * Signature: ([B[B[B[B[B[B[B[B)I
 */
JNIEXPORT jint JNICALL Java_net_mnano_ReadPCM_readPcmStart
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray, jbyteArray, jbyteArray, jbyteArray, jbyteArray, jbyteArray);

/*
 * Class:     net_mnano_ReadPCM
 * Method:    readPcmStop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_net_mnano_ReadPCM_readPcmStop
  (JNIEnv *, jclass);

/*
 * Class:     net_mnano_ReadPCM
 * Method:    playPcmOpen
 * Signature: (III)I
 */
JNIEXPORT jint JNICALL Java_net_mnano_ReadPCM_playPcmOpen
  (JNIEnv *, jclass, jint, jint,jint,jint);

/*
 * Class:     net_mnano_ReadPCM
 * Method:    playPcmStart
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL Java_net_mnano_ReadPCM_playPcmStart
  (JNIEnv *, jclass, jbyteArray);

/*
 * Class:     net_mnano_ReadPCM
 * Method:    playPcmStop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_net_mnano_ReadPCM_playPcmStop
  (JNIEnv *, jclass);


#ifdef __cplusplus
}
#endif
#endif
