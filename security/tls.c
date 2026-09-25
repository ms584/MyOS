#include "tls.h"
#include <stdlib.h>
#include "security.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/platform.h"
#include "certs/trust_anchors.h"

void tls_port_init(void);
static bool ready;
static mbedtls_time_t current_time(mbedtls_time_t *out){
    mbedtls_time_t now=(mbedtls_time_t)security_unix_time();
    if(out)*out=now;return now;
}
static int send_bytes(void *ctx,const unsigned char *buf,size_t len){
    (void)ctx;return net_tcp_write(buf,len);
}
static int recv_bytes(void *ctx,unsigned char *buf,size_t len){
    (void)ctx;return net_tcp_read(buf,len);
}
static bool append(char *dst,size_t cap,size_t *length,const char *source){
    while(*source){if(*length+1>=cap)return false;dst[(*length)++]=*source++;}
    return true;
}
bool tls_http_fetch(const uint8_t ip[4],uint16_t port,const char *host,const char *path,
                    net_http_sink sink,void *context){
    if(!ip||!port||!host||!path||!sink||!security_time_ready()||
       !security_entropy_ready()||!net_configured())return false;
    if(!ready){
        if(mbedtls_platform_set_calloc_free(calloc,free)!=0||
           mbedtls_platform_set_time(current_time)!=0)return false;
        tls_port_init();ready=true;
    }
    mbedtls_ssl_context ssl;mbedtls_ssl_config config;
    mbedtls_entropy_context entropy;mbedtls_ctr_drbg_context drbg;
    mbedtls_x509_crt ca;
    mbedtls_ssl_init(&ssl);mbedtls_ssl_config_init(&config);
    mbedtls_entropy_init(&entropy);mbedtls_ctr_drbg_init(&drbg);
    mbedtls_x509_crt_init(&ca);
    bool opened=false,success=false;
    if(mbedtls_x509_crt_parse_der(&ca,isrg_root_x1,sizeof(isrg_root_x1))!=0||
       mbedtls_x509_crt_parse_der(&ca,digicert_global_root_g2,sizeof(digicert_global_root_g2))!=0)goto done;
#if TEST_CASE == 28
    if(mbedtls_x509_crt_parse_der(&ca,test_ca,sizeof(test_ca))!=0)goto done;
#endif
    static const unsigned char personalization[]="MyOS Phase 28 TLS";
    if(mbedtls_ctr_drbg_seed(&drbg,mbedtls_entropy_func,&entropy,
        personalization,sizeof(personalization)-1)!=0)goto done;
    if(mbedtls_ssl_config_defaults(&config,MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,MBEDTLS_SSL_PRESET_DEFAULT)!=0)goto done;
    mbedtls_ssl_conf_authmode(&config,MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&config,&ca,0);
    mbedtls_ssl_conf_rng(&config,mbedtls_ctr_drbg_random,&drbg);
    if(mbedtls_ssl_setup(&ssl,&config)!=0||
       mbedtls_ssl_set_hostname(&ssl,host)!=0)goto done;
    if(!net_tcp_open(ip,port))goto done;
    opened=true;mbedtls_ssl_set_bio(&ssl,0,send_bytes,recv_bytes,0);
    int result=mbedtls_ssl_handshake(&ssl);
    if(result!=0||mbedtls_ssl_get_verify_result(&ssl)!=0)goto done;
    char request[512];size_t n=0;
    if(!append(request,sizeof(request),&n,"GET ")||
       !append(request,sizeof(request),&n,path)||
       !append(request,sizeof(request),&n," HTTP/1.0\r\nHost: ")||
       !append(request,sizeof(request),&n,host)||
       !append(request,sizeof(request),&n,"\r\nConnection: close\r\n\r\n"))goto done;
    for(size_t i=0;i<n;){result=mbedtls_ssl_write(&ssl,(unsigned char *)request+i,n-i);
        if(result<=0)goto done;i+=(size_t)result;}
    size_t total=0;unsigned char buffer[2048];
    for(;;){
        result=mbedtls_ssl_read(&ssl,buffer,sizeof(buffer));
        if(result==0||result==MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY){success=total>0;break;}
        if(result<0)break;
        total+=(size_t)result;
        if(total>65536||!sink(context,buffer,(size_t)result))break;
    }
done:
    if(opened)net_tcp_close();
    mbedtls_ssl_free(&ssl);mbedtls_ssl_config_free(&config);
    mbedtls_x509_crt_free(&ca);mbedtls_ctr_drbg_free(&drbg);
    mbedtls_entropy_free(&entropy);
    return success;
}
