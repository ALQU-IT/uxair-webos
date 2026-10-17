#include "hevc_gate.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <unistd.h>

int main() {
    char path[]="/tmp/hevc-gate-test-XXXXXX";
    char *dir=mkdtemp(path);
    assert(dir);
    const std::string folder(dir);
    std::string reason; int source=0;
    hevc_probe::GateIdentity identity;
    identity.boot_id="12345678-1234-1234-1234-123456789abc";
    identity.commit="0123456789abcdef0123456789abcdef01234567";
    identity.device=1; identity.inode=2; identity.size=3; identity.mtime=4;

    const std::string stale=folder+"/1080p-decoder-pass.flag";
    int fd=open(stale.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
    assert(fd>=0); close(fd);
    assert(!hevc_probe::consume_uhd_gate(folder,identity,10000,source,reason));
    assert(reason=="missing"); // Legacy pass flags never authorize UHD.

    assert(hevc_probe::clear_uhd_gate(folder,reason));
    assert(hevc_probe::write_uhd_gate(folder,identity,10000,123,reason));
    hevc_probe::GateIdentity other=identity; other.boot_id[0]='a';
    assert(!hevc_probe::consume_uhd_gate(folder,other,10001,source,reason));
    assert(reason=="different-boot-or-binary");
    assert(!hevc_probe::consume_uhd_gate(folder,identity,10001,source,reason));

    assert(hevc_probe::write_uhd_gate(folder,identity,10000,123,reason));
    assert(!hevc_probe::consume_uhd_gate(folder,identity,310001,source,reason));
    assert(reason=="expired-or-invalid-time");
    assert(hevc_probe::write_uhd_gate(folder,identity,10000,123,reason));
    other=identity; other.commit[0]='a';
    assert(!hevc_probe::consume_uhd_gate(folder,other,10001,source,reason));
    assert(reason=="different-boot-or-binary");

    assert(hevc_probe::write_uhd_gate(folder,identity,10000,123,reason));
    assert(hevc_probe::consume_uhd_gate(folder,identity,10001,source,reason));
    assert(source==123 && reason=="accepted");
    assert(!hevc_probe::consume_uhd_gate(folder,identity,10002,source,reason));
    assert(reason=="missing");
    const std::string token=folder+"/hevc-uhd-gate.bin";
    assert(symlink(stale.c_str(),token.c_str())==0);
    assert(!hevc_probe::consume_uhd_gate(folder,identity,10003,source,reason));
    assert(reason=="open-failed");
    assert(hevc_probe::clear_uhd_gate(folder,reason));
    fd=open(token.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
    assert(fd>=0); assert(write(fd,"bad",3)==3); close(fd);
    assert(!hevc_probe::consume_uhd_gate(folder,identity,10004,source,reason));
    assert(reason=="unsafe-or-truncated");
    assert(unlink(stale.c_str())==0);
    assert(rmdir(folder.c_str())==0);
}
