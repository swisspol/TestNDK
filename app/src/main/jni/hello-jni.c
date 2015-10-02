#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <jni.h>
#include <android/log.h>

#include <curl/curl.h>
#include <openssl/opensslconf.h>
#include <openssl/crypto.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h>
#include <openssl/evp.h>

static pthread_mutex_t* _openSSLMutexes;

static void _OpenSSLLockingCallback(int mode, int n, const char* file, int line) {
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&_openSSLMutexes[n]);
    } else {
        pthread_mutex_unlock(&_openSSLMutexes[n]);
    }
}

static void _TestCURL(JNIEnv* env, jobject objectOrClass, jstring arg) {
    const char* path = (*env)->GetStringUTFChars(env, arg, NULL);

    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        char errbuf[CURL_ERROR_SIZE];
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);  // Should already be the default anyway
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);  // Should already be the default anyway
        curl_easy_setopt(curl, CURLOPT_CAINFO, path);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Dark Secret Ninja/1.0");
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, "https://news.ycombinator.com/");
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
            char buffer[1024];
            snprintf(buffer, 1024, "CODE = %i", code);
            __android_log_write(ANDROID_LOG_INFO, "JNI", buffer);
        } else {
            __android_log_write(ANDROID_LOG_ERROR, "JNI", errbuf);
        }
        curl_easy_cleanup(curl);
    } else {
        abort();
    }

    (*env)->ReleaseStringUTFChars(env, arg, path);
}

// https://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/types.html#wp276
static JNINativeMethod methods[] = {
        {"testCURL", "(Ljava/lang/String;)V", (void*)&_TestCURL},
};

// From http://developer.android.com/training/articles/perf-jni.html#native_libraries
// This is the recommended approach, but not the only approach. Explicit registration is not required,
// nor is it necessary that you provide a JNI_OnLoad function. You can instead use "discovery" of native methods
// that are named in a specific way (see the JNI spec for details), though this is less desirable
// because if a method signature is wrong you won't know about it until the first time the method is actually used.

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if ((*vm)->GetEnv(vm, &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    jclass class = (*env)->FindClass(env, "com/example/hellojni/HelloJni");
    if (class == NULL) {
        abort();
    }

    jmethodID mid = (*env)->GetStaticMethodID(env, class, "log", "(Ljava/lang/String;)V");
    (*env)->CallStaticVoidMethod(env, class, mid, (*env)->NewStringUTF(env, "HIT ME"));

    if ((*env)->RegisterNatives(env, class, methods, sizeof(methods)/sizeof(methods[0])) != 0) {
        abort();
    }

#ifndef OPENSSL_THREADS
#error OpenSSL built without threads
#endif
    int count = CRYPTO_num_locks();
    _openSSLMutexes = (pthread_mutex_t*)calloc(count, sizeof(pthread_mutex_t));
    for (int i = 0; i < count; ++i) {
        if (pthread_mutex_init(&_openSSLMutexes[i], NULL)) {
            abort();
        }
    }
    CRYPTO_set_locking_callback(_OpenSSLLockingCallback);
    ERR_load_crypto_strings();
    ERR_load_BIO_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);

    return JNI_VERSION_1_6;
}

jstring Java_com_example_hellojni_HelloJni_stringFromJNI(JNIEnv* env, jobject thiz, jbyteArray arg) {
    jsize length = (*env)->GetArrayLength(env, arg);
    jbyte* bytes = (*env)->GetByteArrayElements(env, arg, NULL);

    char* buffer = malloc(1024);
    snprintf(buffer, 1024, "Hello %.*s", length, bytes);
    jstring temp = (*env)->NewStringUTF(env, buffer);  // Must be modified UTF8 (http://developer.android.com/training/articles/perf-jni.html#UTF_8_and_UTF_16_strings)
    free(buffer);

    // http://developer.android.com/training/articles/perf-jni.html#arrays
    (*env)->ReleaseByteArrayElements(env, arg, bytes, JNI_ABORT);
    return temp;
}
