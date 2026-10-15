#!/usr/bin/env python3
"""Compile the exact development dispatcher insertion against a tiny HTTP stub.

This verifies routing and response plumbing; the pinned UxPlay build separately
compiles the real binary-plist handler and HTTP server.
"""
import importlib.util
import pathlib
import subprocess

root = pathlib.Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location('instrument', root / 'tools/instrument-uxplay.py')
instrument = importlib.util.module_from_spec(spec)
spec.loader.exec_module(instrument)

prefix = r'''
#include "direct_request_diag.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GLOBAL_VERSION "test"
typedef struct { const char *type; const char *session; int body_length; } request_t;
typedef struct { int code; int finished; int length; char body[32];
                 int content_type; int server; } response_t;
typedef struct { int hls_support; } raop_t;
typedef struct { int unused; } conn_t;
static int handler_calls;
static const char *http_request_get_data(request_t *r, int *length) {
    *length=r->body_length;return NULL;
}
static const char *http_request_get_header(request_t *r,const char *name) {
    if(!strcmp(name,"Content-Type"))return r->type;
    if(!strcmp(name,"X-Apple-Session-ID"))return r->session;
    return NULL;
}
static response_t *http_response_create(void) {return calloc(1,sizeof(response_t));}
static void http_response_init(response_t *r,const char *protocol,int code,const char *message) {
    assert(!strcmp(protocol,"HTTP/1.1"));(void)message;r->code=code;
}
static void http_response_add_header(response_t *r,const char *name,const char *value) {
    if(!strcmp(name,"Content-Type"))r->content_type=!strcmp(value,"application/x-apple-binary-plist");
    if(!strcmp(name,"Server"))r->server=!strcmp(value,"AirTunes/"GLOBAL_VERSION);
}
static void http_response_finish(response_t *r,const char *body,int length) {
    assert(length>=0 && length<(int)sizeof(r->body));
    r->finished=1;r->length=length;
    if(length)memcpy(r->body,body,(size_t)length);
}
static void raop_handler_info(conn_t *conn,request_t *request,response_t *response,
                              char **body,int *length) {
    (void)conn;(void)request;++handler_calls;
    *length=8;*body=malloc(8);assert(*body);memcpy(*body,"bplist00",8);
    http_response_add_header(response,"Content-Type","application/x-apple-binary-plist");
}
static void handle(raop_t *raop,conn_t *conn,request_t *request,
                   const char *method,const char *url,const char *protocol,
                   const char *cseq,int ble,response_t **response) {
    if (!cseq && !raop->hls_support && !ble) {
'''
suffix = r'''
        return;
    }
}
int main(void) {
    raop_t raop={0};conn_t conn={0};request_t request={0};response_t *response=NULL;
    handle(&raop,&conn,&request,"GET","/info","HTTP/1.1",NULL,0,&response);
    assert(response && response->code==200 && response->finished);
    assert(response->content_type && response->server);
    assert(response->length==8 && !memcmp(response->body,"bplist00",8));
    assert(handler_calls==1);free(response);response=NULL;
    request.body_length=1;
    handle(&raop,&conn,&request,"GET","/info","HTTP/1.1",NULL,0,&response);
    assert(!response && handler_calls==1);request.body_length=0;
    handle(&raop,&conn,&request,"POST","/info","HTTP/1.1",NULL,0,&response);
    assert(!response && handler_calls==1);
    handle(&raop,&conn,&request,"GET","/info?secret=1","HTTP/1.1",NULL,0,&response);
    assert(!response && handler_calls==1);
    handle(&raop,&conn,&request,"GET","/info","RTSP/1.0",NULL,0,&response);
    assert(!response && handler_calls==1);
    request.session="private-session";
    handle(&raop,&conn,&request,"GET","/info","HTTP/1.1",NULL,0,&response);
    assert(!response && handler_calls==1);
    puts("PASS: generated HTTP /info gate and response plumbing");
    return 0;
}
'''

output = root / 'build/tests'
output.mkdir(parents=True, exist_ok=True)
source = output / 'direct-info-gate.c'
binary = output / 'direct-info-gate-test'
source.write_text(prefix + instrument.PROBE + suffix)
subprocess.run(['cc', '-std=c11', '-g', '-fsanitize=address,undefined', '-I' + str(root / 'src'),
                str(source), str(root / 'src/direct_request_diag.c'), '-o', str(binary)], check=True)
subprocess.run([str(binary)], check=True)
