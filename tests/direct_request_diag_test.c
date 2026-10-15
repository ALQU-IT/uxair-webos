#include "direct_request_diag.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void absent(const char *line) {
    const char *secrets[]={"PrivateAlbum","auth-secret","token-123","cookie-secret",
                           "session-uuid","iphone-name","https://example.invalid", "?"};
    size_t i;
    for(i=0;i<sizeof(secrets)/sizeof(secrets[0]);++i)assert(!strstr(line,secrets[i]));
}

int main(void) {
    char line[256];
    assert(direct_info_eligible("GET","/info","HTTP/1.1",NULL,0,0));
    assert(!direct_info_eligible("POST","/info","HTTP/1.1",NULL,0,0));
    assert(!direct_info_eligible("GET","/info?token=auth-secret","HTTP/1.1",NULL,0,0));
    assert(!direct_info_eligible("GET","/info/other","HTTP/1.1",NULL,0,0));
    assert(!direct_info_eligible("GET","/info","RTSP/1.0",NULL,0,0));
    assert(!direct_info_eligible("GET","/info","HTTP/1.1",NULL,0,1));
    assert(!direct_info_eligible("GET","/info","HTTP/1.1","application/x-apple-binary-plist",0,0));
    assert(!direct_info_eligible("GET","/info","HTTP/1.1",NULL,1,0));
    assert(!direct_info_eligible("GET","/info","HTTP/1.1",NULL,33554433,0));
    assert(!direct_info_eligible(NULL,NULL,NULL,NULL,-1,0));

    FILE *response_capture=tmpfile();assert(response_capture);
    int response_saved=dup(STDERR_FILENO);assert(response_saved>=0);
    assert(dup2(fileno(response_capture),STDERR_FILENO)>=0);
    direct_request_diag_log_result("GET","/info?token=auth-secret",NULL,
                                   "HTTP/1.1",0,0,200);
    fflush(stderr);
    assert(dup2(response_saved,STDERR_FILENO)>=0);close(response_saved);
    rewind(response_capture);
    assert(fgets(line,sizeof(line),response_capture));
    assert(strstr(line,"method=GET path=info content_type=none"));
    assert(strstr(line,"category=control result=200"));
    assert(strstr(line,"info_query=other"));
    absent(line);

    direct_request_diag_format(line,sizeof(line),"GET","/info",NULL,"HTTP/1.1",0,0);
    assert(strstr(line,"result=0 info_query=none"));
    direct_request_diag_format(line,sizeof(line),"GET","/info?txtAirPlay",NULL,"HTTP/1.1",0,0);
    assert(strstr(line,"result=0 info_query=txt-airplay"));
    direct_request_diag_format(line,sizeof(line),"GET","/info?txtRAOP",NULL,"HTTP/1.1",0,0);
    assert(strstr(line,"result=0 info_query=txt-raop"));

    direct_request_diag_format(line,sizeof(line),"POST",
        "/play?token=token-123&name=PrivateAlbum",
        "application/x-apple-binary-plist; name=iphone-name", "HTTP/1.1", 20000000,1);
    assert(strstr(line,"method=POST path=play content_type=binary-plist"));
    assert(strstr(line,"body_bytes=16777216 body_capped=1"));
    assert(strstr(line,"protocol=HTTP/1.1 session_header=1 category=video result=0"));
    absent(line);

    direct_request_diag_format(line,sizeof(line),"PUT",
        "/photo?session=session-uuid&url=https://example.invalid",
        "image/heic; filename=PrivateAlbum", "HTTP/1.1", 4137,0);
    assert(strstr(line,"path=photo content_type=image-heic body_bytes=4137"));
    assert(strstr(line,"category=photo result=0"));
    absent(line);

    direct_request_diag_format(line,sizeof(line),"POST auth-secret",
        "/auth-secret/PrivateAlbum?cookie=cookie-secret",
        "custom/auth-secret", "HTTP/1.1 iphone-name", -1,1);
    assert(strstr(line,"method=other path=other content_type=other body_bytes=-1"));
    assert(strstr(line,"protocol=other session_header=1 category=unknown"));
    absent(line);

    direct_request_diag_format(line,sizeof(line),NULL,NULL,NULL,NULL,0,0);
    assert(strstr(line,"method=unknown path=other content_type=none"));

    FILE *capture=tmpfile();assert(capture);
    int saved=dup(STDERR_FILENO);assert(saved>=0);
    assert(dup2(fileno(capture),STDERR_FILENO)>=0);
    direct_request_diag_body_limit();
    int i;
    for(i=0;i<400;++i)direct_request_diag_log("PUT","/photo?token=token-123",
        "image/jpeg; filename=PrivateAlbum","HTTP/1.1",1024,0);
    fflush(stderr);
    assert(dup2(saved,STDERR_FILENO)>=0);close(saved);
    rewind(capture);
    int records=0,suppressions=0,body_limits=0;
    while(fgets(line,sizeof(line),capture)) {
        absent(line);
        if(strstr(line,"further_requests=omitted"))++suppressions;
        else if(strstr(line,"result=body_limit"))++body_limits;
        else {assert(strstr(line,"[direct-probe] method=PUT path=photo"));++records;}
    }
    fclose(capture);
    /* The earlier response marker uses one slot in the process-wide cap. */
    assert(records==190 && body_limits==1 && suppressions==1);
    puts("PASS: strict HTTP info gate, probe redaction, body limit and 192-record cap");
    return 0;
}
