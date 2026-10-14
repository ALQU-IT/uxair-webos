#include "shell.h"
#include "identity.h"
#include <SDL_webOS.h>
#include <SDL_ttf.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>
static std::string log_path;
static volatile sig_atomic_t interrupted = 0;
static void signal_stop(int) { interrupted = 1; }
int64_t now_ns() { timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return int64_t(t.tv_sec)*1000000000 + t.tv_nsec; }
std::string app_directory() {
    char p[4096]; ssize_t n = readlink("/proc/self/exe", p, sizeof(p)-1);
    if (n <= 0) return ".";
    p[n] = 0; std::string s(p); return s.substr(0, s.rfind('/'));
}
void start_log(const char *name) {
    std::string p = app_directory() + "/" + name;
    log_path=p;
    rename(p.c_str(), (p+".previous").c_str());
    freopen(p.c_str(), "w", stderr);
    setvbuf(stderr, nullptr, _IOLBF, 0);
    // Proprietary drivers emit high-volume frame diagnostics to stdout.
    // Keep application/errors on stderr, without filling app storage with them.
    int nullfd=open("/dev/null",O_WRONLY);
    if(nullfd>=0){dup2(nullfd,STDOUT_FILENO);close(nullfd);}
}
void rotate_log() {
    struct stat s;
    if(!log_path.empty() && fstat(fileno(stderr),&s)==0 && s.st_size>1024*1024) {
        rename(log_path.c_str(),(log_path+".previous").c_str());
        freopen(log_path.c_str(),"w",stderr);setvbuf(stderr,nullptr,_IOLBF,0);
    }
}
bool Shell::open() {
    signal(SIGTERM, signal_stop); signal(SIGINT, signal_stop);
    input_ = InputMapper(static_cast<SDL_Scancode>(SDL_WEBOS_SCANCODE_BACK),
                         static_cast<SDL_Scancode>(SDL_WEBOS_SCANCODE_EXIT));
    setenv("EGL_PLATFORM", "wayland", 0); setenv("XDG_RUNTIME_DIR", "/tmp/xdg", 0);
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK, "true");
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_EXIT, "true");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "[SDL] %s\n",SDL_GetError()); return false; }
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(0,&mode) != 0) return false;
    window = SDL_CreateWindow(UXPLAY_DISPLAY_NAME,0,0,mode.w,mode.h,SDL_WINDOW_FULLSCREEN);
    if (window) renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
    if (!renderer) { fprintf(stderr,"[SDL] %s\n",SDL_GetError()); return false; }
    fprintf(stderr,"[SDL] window %dx%d\n",mode.w,mode.h);
    if(TTF_Init()!=0) return false;
    font_=TTF_OpenFont("/usr/share/fonts/LG_Display-Regular.ttf",30);
    large_=TTF_OpenFont("/usr/share/fonts/LG_Display-Regular.ttf",76);
    small_=TTF_OpenFont("/usr/share/fonts/LG_Display-Regular.ttf",23);
    if(!font_||!large_||!small_) {fprintf(stderr,"[font] %s\n",TTF_GetError());return false;}
    return true;
}
void Shell::transparent() {
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer,0,0,0,0); SDL_RenderClear(renderer); SDL_RenderPresent(renderer);
}
Action Shell::action() {
    SDL_Event e;
    static Uint32 last_motion_log_ms = 0;
    while (SDL_PollEvent(&e)) {
        const Uint32 now = SDL_GetTicks();
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            if (!e.key.repeat)
                fprintf(stderr,"[input] key %s keycode=%ld scancode=%d repeat=%u\n",
                        e.type == SDL_KEYDOWN ? "down" : "up",
                        static_cast<long>(e.key.keysym.sym),
                        static_cast<int>(e.key.keysym.scancode), e.key.repeat);
        } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
            fprintf(stderr,"[input] mouse %s button=%u clicks=%u x=%d y=%d\n",
                    e.type == SDL_MOUSEBUTTONDOWN ? "down" : "up",
                    e.button.button, e.button.clicks, e.button.x, e.button.y);
        } else if (e.type == SDL_MOUSEMOTION &&
                   (last_motion_log_ms == 0 || Uint32(now - last_motion_log_ms) >= 5000)) {
            fprintf(stderr,"[input] pointer motion x=%d y=%d buttons=%u\n",
                    e.motion.x, e.motion.y, e.motion.state);
            last_motion_log_ms = now;
        }
        const Action mapped = input_.map(e, now);
        if (mapped != Action::None) return mapped;
    }
    return interrupted?Action::Exit:Action::None;
}
bool Shell::pump() {Action a=action();return a!=Action::Back&&a!=Action::Exit;}
void Shell::text(const std::string& s,int y,TTF_Font *font,SDL_Color color) {
    SDL_Surface *surface=TTF_RenderUTF8_Blended(font,s.c_str(),color);if(!surface)return;
    SDL_Texture *texture=SDL_CreateTextureFromSurface(renderer,surface);
    int w,h;SDL_GetRendererOutputSize(renderer,&w,&h);
    int width=std::min(surface->w,w-80),height=surface->h;
    SDL_Rect rect={(w-width)/2,y,width,height};
    if(texture) {SDL_RenderCopy(renderer,texture,nullptr,&rect);SDL_DestroyTexture(texture);}
    SDL_FreeSurface(surface);
}
void Shell::text_at(const std::string& s,int x,int y,int max_width,TTF_Font *font,SDL_Color color) {
    SDL_Surface *surface=TTF_RenderUTF8_Blended(font,s.c_str(),color);if(!surface)return;
    SDL_Texture *texture=SDL_CreateTextureFromSurface(renderer,surface);
    int width=std::min(surface->w,max_width);
    SDL_Rect source={0,0,width,surface->h};
    SDL_Rect rect={x,y,width,surface->h};
    if(texture) { SDL_RenderCopy(renderer,texture,&source,&rect); SDL_DestroyTexture(texture); }
    SDL_FreeSurface(surface);
}
void Shell::page(UiPage page_id,const std::string& subtitle,const std::vector<UiRow>& rows,int selected,const std::string& footer) {
    int w=0,h=0;SDL_GetRendererOutputSize(renderer,&w,&h);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer,10,17,28,255);SDL_RenderClear(renderer);
    SDL_Rect band={0,0,w,std::max(8,h/10)};
    SDL_SetRenderDrawColor(renderer,17,35,49,255);SDL_RenderFillRect(renderer,&band);
    const char *titles[]={"UxPlay webOS Dev","Quality settings","Usage statistics","Live diagnostics","Capabilities","Recent sessions"};
    int index=int(page_id);if(index<0||index>5)index=0;
    text_at(titles[index],w/16,h/28,w*7/8,large_,{80,219,180,255});
    text_at(subtitle,w/16,h/7,w*7/8,font_,{210,226,235,255});
    const int top=h/4;
    const int row_height=std::max(68,h/12);
    for(size_t i=0;i<rows.size() && i<7;++i) {
        int y=top+int(i)*row_height;
        if(int(i)==selected) {
            SDL_Rect focus={w/20,y-7,w*9/10,row_height-5};
            SDL_SetRenderDrawColor(renderer,33,68,79,255);SDL_RenderFillRect(renderer,&focus);
            SDL_Rect marker={w/20,y-7,7,row_height-5};
            SDL_SetRenderDrawColor(renderer,80,219,180,255);SDL_RenderFillRect(renderer,&marker);
        }
        const SDL_Color primary=rows[i].enabled?SDL_Color{242,247,250,255}:SDL_Color{127,145,156,255};
        text_at(rows[i].label,w/14,y,w*5/10,font_,primary);
        if(!rows[i].value.empty())text_at(rows[i].value,w*2/3,y,w/4,font_,primary);
        if(!rows[i].note.empty())text_at(rows[i].note,w/14,y+38,w*4/5,small_,{150,173,187,255});
    }
    text_at(footer,w/16,h-62,w*7/8,small_,{152,180,194,255});
    SDL_RenderPresent(renderer);
}
void Shell::overlay(const std::vector<std::string>& lines) {
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer,0,0,0,0);SDL_RenderClear(renderer);
    if(lines.empty())return;
    int w=0,h=0;SDL_GetRendererOutputSize(renderer,&w,&h);
    int x=w/30,y=h/25,box_w=std::min(w*3/5,950),box_h=34+int(lines.size())*38;
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,8,18,28,208);
    SDL_Rect box={x,y,box_w,box_h};SDL_RenderFillRect(renderer,&box);
    for(size_t i=0;i<lines.size();++i)
        text_at(lines[i],x+20,y+15+int(i)*38,box_w-40,small_,{239,249,246,255});
    SDL_RenderPresent(renderer);
}
void Shell::screen(const std::string& status,const std::string& pin,int delay) {
    SDL_SetRenderDrawColor(renderer,14,25,36,255);SDL_RenderClear(renderer);
    int w,h;SDL_GetRendererOutputSize(renderer,&w,&h);
    text(UXPLAY_DISPLAY_NAME,h/6,large_,{65,210,171,255});
    text(status,h/2-60,font_,{238,242,246,255});
    if(!pin.empty()) text(pin,h/2+10,large_,{255,255,255,255});
    else text(std::string("Open Screen Mirroring and select ")+UXPLAY_DISPLAY_NAME,h/2+20,font_,{178,194,207,255});
    char sync[160];snprintf(sync,sizeof(sync),"◀ ▶  Audio delay: %+d ms  ·  Adjust before connecting",delay);
    text(sync,h-135,font_,{145,167,185,255});
    text("Same Wi-Fi network  ·  Quality in Settings  ·  Back to exit",h-80,font_,{145,167,185,255});
    SDL_RenderPresent(renderer);
}
Shell::~Shell() {if(font_)TTF_CloseFont(font_);if(large_)TTF_CloseFont(large_);if(small_)TTF_CloseFont(small_);TTF_Quit();if(renderer) SDL_DestroyRenderer(renderer); if(window) SDL_DestroyWindow(window); SDL_Quit(); }
