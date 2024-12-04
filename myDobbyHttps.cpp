#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <thread>
#include <link.h>
#include <sys/stat.h>
#include <byopen.h>
#include <base64.h>
#include <dobby.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include "GlobalDef.h"
using namespace std;
extern "C"
{
    bool startHacking(JavaVM *vm);
}

void parseCert1(X509* x509)
{
    cout <<"--------------------" << endl;
    BIO *bio_out = BIO_new_fp(stdout, BIO_NOCLOSE);
    //PEM_write_bio_X509(bio_out, x509);//STD OUT the PEM
    X509_print(bio_out, x509);//STD OUT the details
    //X509_print_ex(bio_out, x509, XN_FLAG_COMPAT, X509_FLAG_COMPAT);//STD OUT the details
    BIO_free(bio_out);
}
//----------------------------------------------------------------------
void parseCert2(X509* x509)
{
    cout <<"--------------------" << endl;
    BIO *bio_out = BIO_new_fp(stdout, BIO_NOCLOSE);
 
    long l = X509_get_version(x509);
    BIO_printf(bio_out, "Version: %ld\n", l+1);
 
    ASN1_INTEGER *bs = X509_get_serialNumber(x509);
    BIO_printf(bio_out,"Serial: ");
    for(int i=0; i<bs->length; i++) {
        BIO_printf(bio_out,"%02x",bs->data[i] );
    }
    BIO_printf(bio_out,"\n");
 
    X509_signature_print(bio_out, x509->sig_alg, NULL);
 
    BIO_printf(bio_out,"Issuer: ");
    X509_NAME_print(bio_out,X509_get_issuer_name(x509),0);
    BIO_printf(bio_out,"\n");
 
    BIO_printf(bio_out,"Valid From: ");
    ASN1_TIME_print(bio_out,X509_get_notBefore(x509));
    BIO_printf(bio_out,"\n");
 
    BIO_printf(bio_out,"Valid Until: ");
    ASN1_TIME_print(bio_out,X509_get_notAfter(x509));
    BIO_printf(bio_out,"\n");
 
    BIO_printf(bio_out,"Subject: ");
    X509_NAME_print(bio_out,X509_get_subject_name(x509),0);
    BIO_printf(bio_out,"\n");
 
    EVP_PKEY *pkey=X509_get_pubkey(x509);
    EVP_PKEY_print_public(bio_out, pkey, 0, NULL);
    EVP_PKEY_free(pkey);
 
    X509_CINF *ci=x509->cert_info;
    X509V3_extensions_print(bio_out, (char*)"X509v3 extensions", ci->extensions, X509_FLAG_COMPAT, 0);
 
    X509_signature_print(bio_out, x509->sig_alg, x509->signature);
    BIO_free(bio_out);
}
//----------------------------------------------------------------------
void parseCert3(X509* x509)
{
    cout <<"--------------------" << endl;
    //cout << pem(x509) << endl;
    cout <<"Thumbprint: " << thumbprint(x509) << endl;
    cout <<"Version: " << certversion(x509) << endl;
    cout <<"Serial: " << serial(x509) << endl;
    cout <<"Issuer: " << issuer_one_line(x509) << endl;
    map<string,string> ifields = issuer(x509);
    for(map<string, string>::iterator i = ifields.begin(), ix = ifields.end(); i != ix; i++ )
        cout << " * " << i->first << " : " << i->second << endl;
    cout <<"Subject: "    << subject_one_line(x509) << endl;
    map<string,string> sfields = subject(x509);
    for(map<string, string>::iterator i = sfields.begin(), ix = sfields.end(); i != ix; i++ )
        cout << " * " <<  i->first << " : " << i->second << endl;
    cout <<"SignatureAlgorithm: "    << signature_algorithm(x509) << endl;
    cout <<"PublicKeyType: "    << public_key_type(x509) << public_key_ec_curve_name(x509) << endl;
    cout <<"PublicKeySize: "    << public_key_size(x509) << endl;
    cout <<"NotBefore: "    << asn1datetime_isodatetime(X509_get_notBefore(x509)) << endl;
    cout <<"NotAfter: "    << asn1datetime_isodatetime(X509_get_notAfter(x509)) << endl;
    cout <<"SubjectAltName(s):" << endl;
    vector<string> sans = subject_alt_names(x509);
    for(int i=0, ix=sans.size(); i<ix; i++) {
        cout << " " << sans[i] << endl;
    }
    cout <<"CRL URLs:" << endl;
    vector<string> crls = crl_urls(x509);
    for(int i=0, ix=crls.size(); i<ix; i++) {
        cout << " " << crls[i] << endl;
    }
    cout <<"OCSP URLs:" << endl;
    vector<string> urls = ocsp_urls(x509);
    for(int i=0, ix=urls.size(); i<ix; i++) {
        cout << " " << urls[i] << endl;
    }
}


// int SSL_read(SSL *ssl, void *buf, int num)
// int SSL_write(SSL *ssl, const void *buf, int num)
int (*oldSSL_read)(void *ssl, void *buf, int num) = 0;
int newSSL_read(void *ssl, void *buf, int num)
{
    LOGI("newSSL_read enter");
    X509 *objX509 = SSL_get_peer_certificate((const ssl_st *)ssl);
    LOGI("objX509:%p", objX509);
    parseCert2(objX509);
    // X509_print()
    return oldSSL_read(ssl, buf, num);
}

bool startHacking(JavaVM *vm)
{
    LOGI("fucking start now");
    void *SSL_readAddr = DobbySymbolResolver("libssl.so", "SSL_read");
    LOGI("SSL_readAddr:%p", SSL_readAddr);
    DobbyHook(SSL_readAddr, (dobby_dummy_func_t)newSSL_read, (dobby_dummy_func_t *)&oldSSL_read);
    return true;
}