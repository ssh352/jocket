#define _GNU_SOURCE        /* or _BSD_SOURCE or _SVID_SOURCE */
#include <linux/futex.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <sys/time.h>
#include <jni.h>
#include <sched.h>
#include "jocket_futex_Futex.h"
#include "jocket_futex_Futex_critical.h"

#ifdef __cplusplus
extern "C" {
#endif

  //#define DEBUG 1

  JNIEXPORT jlong JNICALL Java_jocket_futex_Futex_getAddress
    (JNIEnv *env, jclass cls, jobject buf)
    {
      return (jlong) (*env)->GetDirectBufferAddress(env, buf);
    }

  // for test purposes
  JNIEXPORT jint JNICALL Java_jocket_futex_Futex_getInt0
    (JNIEnv *env, jclass cls, jlong addr) 
    {
      jint* seqPtr = (jint *)addr;
      return *seqPtr;
    }

  // for test purposes
  JNIEXPORT void JNICALL Java_jocket_futex_Futex_setInt0
    (JNIEnv *env, jclass cls, jlong addr, jint val)
    {
      jint* seqPtr = (jint *)addr;
      *seqPtr = val;
    }


  //const struct timespec SECOND = { 1, 0 };

  /*
   * Called by the reader.
   */
  JNIEXPORT void JNICALL Java_jocket_futex_Futex_pause
    (JNIEnv *env, jclass cls, jlong futAddr, jlong seqAddr, jint oldseq)
    {
    JavaCritical_jocket_futex_Futex_pause(futAddr, seqAddr, oldseq);
    }

  /*
   * Critical native version...
   */
  JNIEXPORT void JNICALL JavaCritical_jocket_futex_Futex_pause
    (jlong futAddr, jlong seqAddr, jint oldseq)
    {
      unsigned int i = 0;
      jint* seqPtr = (jint *)seqAddr;
      int* futex = (int*)futAddr;

      do {
        if (i++ > 1024) {
          if (__sync_val_compare_and_swap(futex, 0, -1) == 0) {
            syscall(SYS_futex, futex, FUTEX_WAIT, -1, NULL, NULL, 0);
          }
          else  {
            // a value other than 0 indicates that data became available => no wait
            *futex = 0;
          }

        } else {
#if defined(__i386__)
          __builtin_ia32_pause();
#elif defined(__x86_64__)
          asm("pause");
#else
          sched_yield();
#endif
        }
      } while (*seqPtr == oldseq);
    }

  /*
   * Called by the writer.
   */
  JNIEXPORT void JNICALL Java_jocket_futex_Futex_signal0
    (JNIEnv *env, jclass cls, jlong addr)
    {
      jint *ptr = (jint *)addr;
      // a value of -1 implies that a thread is waiting
      if (__sync_val_compare_and_swap(ptr, 0, 1) == -1) {
        *ptr = 0;
        syscall(SYS_futex, ptr, FUTEX_WAKE, 0, NULL, NULL, 0);
      }
    }

  JNIEXPORT void JNICALL JavaCritical_jocket_futex_Futex_signal0
    (jlong addr)
    {
      jint *ptr = (jint *)addr;
      // a value of -1 implies that a thread is waiting
      if (__sync_val_compare_and_swap(ptr, 0, 1) == -1) {
        *ptr = 0;
        syscall(SYS_futex, ptr, FUTEX_WAKE, 0, NULL, NULL, 0);
      }
    }

  /*
   * DEPRECATED. pause() is now called to minimize the number of
   * JNI roundtrips.
   */
  JNIEXPORT inline void JNICALL Java_jocket_futex_Futex_await0
    (JNIEnv *env, jclass cls, jlong addr)
    {
      // TODO: add timeout parameter to avoid infinite wait
      jint* ptr = (jint*)addr;
      //const struct timespec SECOND = { 1, 0 };

      // a value other than 0 indicates that data became available => no wait
      int val = __sync_val_compare_and_swap(ptr, 0, -1); 
      if (val == 0) {
        syscall(SYS_futex, ptr, FUTEX_WAIT, -1, NULL, NULL, 0);
      }
      else  {
        *ptr = 0;
      }
    }


  // FOR TEST PURPOSES

#if defined(__i386__)
  static __inline__ unsigned long long rdtsc(void) {
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
  }

#elif defined(__x86_64__)
  static __inline__ unsigned long long rdtsc(void) {
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
  }

#endif

  JNIEXPORT jlong JNICALL Java_jocket_futex_Futex_rdtsc
    (JNIEnv *env, jclass c) {
      return (jlong) rdtsc();
    }

#ifdef __cplusplus
}
#endif
