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
#include <cmath>
static std::string log_path;
static volatile sig_atomic_t interrupted = 0;
static void signal_stop(int) { interrupted = 1; }
static void fill_round_rect(SDL_Renderer *renderer,const SDL_Rect& rect,int radius,SDL_Color color) {
    SDL_SetRenderDrawColor(renderer,color.r,color.g,color.b,color.a);
    radius=std::max(0,std::min(radius,std::min(rect.w,rect.h)/2));
    for(int y=0;y<rect.h;++y) {
        int inset=0;
        if(y<radius) {
            double dy=radius-y-0.5;
            inset=radius-int(std::sqrt(std::max(0.0,double(radius*radius)-dy*dy)));
        } else if(y>=rect.h-radius) {
            double dy=y-(rect.h-radius)+0.5;
            inset=radius-int(std::sqrt(std::max(0.0,double(radius*radius)-dy*dy)));
        }
        SDL_RenderDrawLine(renderer,rect.x+inset,rect.y+y,rect.x+rect.w-inset-1,rect.y+y);
    }
}
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
    window = SDL_CreateWindow(UXAIR_DISPLAY_NAME,0,0,mode.w,mode.h,SDL_WINDOW_FULLSCREEN);
    if (window) renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
    if (!renderer) { fprintf(stderr,"[SDL] %s\n",SDL_GetError()); return false; }
    fprintf(stderr,"[SDL] graphics-window=%dx%d (logical app coordinates)\n",mode.w,mode.h);
    if(TTF_Init()!=0) return false;
    font_=TTF_OpenFont("/usr/share/fonts/LG_Display-Regular.ttf",30);
    large_=TTF_OpenFont("/usr/share/fonts/LG_Display-Regular.ttf",76);
    small_=TTF_OpenFont("/usr/share/fonts/LG_Display-Regular.ttf",23);
    pin_font_=TTF_OpenFont("/usr/share/fonts/LG_Display-Regular.ttf",112);
    pin_font_small_=TTF_OpenFont("/usr/share/fonts/LG_Display-Regular.ttf",88);
    if(pin_font_)TTF_SetFontStyle(pin_font_,TTF_STYLE_BOLD);
    if(pin_font_small_)TTF_SetFontStyle(pin_font_small_,TTF_STYLE_BOLD);
    if(!font_||!large_||!small_||!pin_font_||!pin_font_small_) {fprintf(stderr,"[font] %s\n",TTF_GetError());return false;}
    SDL_Surface *qr=SDL_LoadBMP((app_directory()+"/bmc-qr.bmp").c_str());
    if(qr) {donation_qr_=SDL_CreateTextureFromSurface(renderer,qr);SDL_FreeSurface(qr);}
    if(!donation_qr_)fprintf(stderr,"[donation] bundled QR image unavailable: %s\n",SDL_GetError());
    return true;
}
void Shell::transparent() {
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer,0,0,0,0); SDL_RenderClear(renderer); SDL_RenderPresent(renderer);
}
Action Shell::action() {
    SDL_Event e;
    static Uint32 last_motion_log_ms = 0;
    pointer_select_ = false;
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
        if (mapped != Action::None) {
            if (mapped==Action::Select && e.type==SDL_MOUSEBUTTONDOWN) {
                pointer_select_=true;pointer_x_=e.button.x;pointer_y_=e.button.y;
            }
            return mapped;
        }
    }
    return interrupted?Action::Exit:Action::None;
}
bool Shell::pointer_select(int& x, int& y) const {
    if (!pointer_select_) return false;
    x=pointer_x_;y=pointer_y_;return true;
}
static int dialog_first(const UiDialog& dialog) {
    int count=int(dialog.choices.size()),visible=std::min(5,count);
    return std::max(0,std::min(dialog.selected-2,count-visible));
}
UiHit Shell::hit_test(int x, int y, int row_count, const UiDialog *dialog) const {
    int w=0,h=0;SDL_GetRendererOutputSize(renderer,&w,&h);
    if(dialog) {
        int visible=std::min(5,int(dialog->choices.size()));
        int box_w=std::min(w*68/100,1050),box_h=150+visible*72;
        int left=(w-box_w)/2,top=(h-box_h)/2;
        if(x<left || x>=left+box_w || y<top+124 || y>=top+124+visible*72)
            return {UiHitKind::Cancel,-1};
        return {UiHitKind::DialogChoice,dialog_first(*dialog)+(y-top-124)/72};
    }
    int top=h/4,row_height=std::max(66,h/11),sidebar=w/4;
    if(x>=0 && x<sidebar && y>=top && y<top+5*row_height)
        return {UiHitKind::Category,(y-top)/row_height};
    if(x>=sidebar+24 && x<w-24 && y>=top && y<top+std::min(row_count,7)*row_height)
        return {UiHitKind::Row,(y-top)/row_height};
    return {};
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
void Shell::page(UiPage page_id,const std::string& subtitle,const std::vector<UiRow>& rows,int selected,const std::string& footer,const UiDialog *dialog) {
    int w=0,h=0;SDL_GetRendererOutputSize(renderer,&w,&h);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer,8,15,25,255);SDL_RenderClear(renderer);
    const int sidebar=w/4,panel=sidebar+32;
    SDL_Rect side={0,0,sidebar,h};
    SDL_SetRenderDrawColor(renderer,15,30,43,255);SDL_RenderFillRect(renderer,&side);
    SDL_Rect accent={0,0,w,7};
    SDL_SetRenderDrawColor(renderer,66,216,177,255);SDL_RenderFillRect(renderer,&accent);
    const char *categories[]={"Video","Audio","Diagnostics","About"};
    const char *titles[]={"Video","Audio","Diagnostics","About","Recent Sessions"};
    int index=int(page_id);if(index<0||index>4)index=0;
    int category=index==4?2:index;
    text_at("UXAIR",w/28,h/18,sidebar-40,large_,{77,226,185,255});
    text_at("AirPlay Receiver",w/28,h/8,sidebar-40,small_,{154,185,202,255});
    text_at(titles[index],panel,h/20,w-panel-35,large_,{239,247,249,255});
    text_at(subtitle,panel,h/7,w-panel-35,font_,{154,185,202,255});
    const int top=h/4;
    const int row_height=std::max(66,h/11);
    for(int i=0;i<4;++i) {
        int y=top+i*row_height;
        if(i==category) {
            SDL_Rect tab={14,y-5,sidebar-28,row_height-6};
            SDL_SetRenderDrawColor(renderer,31,67,77,255);SDL_RenderFillRect(renderer,&tab);
            SDL_Rect marker={14,y-5,6,row_height-6};
            SDL_SetRenderDrawColor(renderer,77,226,185,255);SDL_RenderFillRect(renderer,&marker);
        }
        text_at(categories[i],30,y+12,sidebar-52,font_,i==category?SDL_Color{238,249,247,255}:SDL_Color{150,175,190,255});
    }
    for(size_t i=0;i<rows.size() && i<7;++i) {
        int y=top+int(i)*row_height;
        if(int(i)==selected) {
            SDL_Rect focus={panel-8,y-6,w-panel-24,row_height-5};
            SDL_SetRenderDrawColor(renderer,34,66,76,255);SDL_RenderFillRect(renderer,&focus);
            SDL_Rect marker={panel-8,y-6,6,row_height-5};
            SDL_SetRenderDrawColor(renderer,77,226,185,255);SDL_RenderFillRect(renderer,&marker);
        }
        const SDL_Color primary=rows[i].enabled?SDL_Color{242,247,250,255}:SDL_Color{127,145,156,255};
        text_at(rows[i].label,panel+18,y+4,(w-panel)*56/100,font_,primary);
        if(!rows[i].value.empty())text_at(rows[i].value,panel+(w-panel)*65/100,y+4,(w-panel)*30/100,font_,int(i)==selected?SDL_Color{77,226,185,255}:primary);
        if(!rows[i].note.empty())text_at(rows[i].note,panel+18,y+43,w-panel-60,small_,{151,178,194,255});
    }
    text_at(footer,panel,h-54,w-panel-35,small_,{152,180,194,255});
    if(dialog) {
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer,0,5,10,190);
        SDL_Rect dim={0,0,w,h};SDL_RenderFillRect(renderer,&dim);
        int visible=std::min(5,int(dialog->choices.size()));
        int box_w=std::min(w*68/100,1050),box_h=150+visible*72;
        int left=(w-box_w)/2,dialog_top=(h-box_h)/2;
        SDL_SetRenderDrawColor(renderer,22,39,52,255);
        SDL_Rect box={left,dialog_top,box_w,box_h};SDL_RenderFillRect(renderer,&box);
        text_at(dialog->title,left+32,dialog_top+18,box_w-64,font_,{246,251,251,255});
        text_at(dialog->description,left+32,dialog_top+68,box_w-64,small_,{164,191,207,255});
        int first=dialog_first(*dialog);
        for(int i=0;i<visible;++i) {
            int option=first+i,y=dialog_top+124+i*72;
            if(option==dialog->selected) {
                SDL_Rect selected_row={left+20,y-3,box_w-40,67};
                SDL_SetRenderDrawColor(renderer,43,83,91,255);SDL_RenderFillRect(renderer,&selected_row);
            }
            text_at(dialog->choices[option],left+42,y+12,box_w-120,font_,option==dialog->selected?SDL_Color{247,253,250,255}:SDL_Color{184,205,216,255});
            if(option==dialog->current)text_at("✓",left+box_w-94,y+12,60,small_,{77,226,185,255});
        }
    }
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
void Shell::donation_panel() {
    int w=0,h=0;SDL_GetRendererOutputSize(renderer,&w,&h);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer,0,0,0,0);SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,0,4,10,168);SDL_Rect backdrop={0,0,w,h};SDL_RenderFillRect(renderer,&backdrop);
    int qr_size=std::min(500,std::min(w*45/100,h*65/100));
    int top=std::max(20,(h-(qr_size+178))/2);
    int panel_w=std::min(w-40,std::max(560,qr_size+120));
    SDL_Rect panel={(w-panel_w)/2,top,panel_w,qr_size+150};
    fill_round_rect(renderer,panel,26,{14,30,43,248});
    const std::string heading="Enjoying the app? Buy me a coffee.";
    int heading_width=0;TTF_SizeUTF8(font_,heading.c_str(),&heading_width,nullptr);
    text_at(heading,(w-heading_width)/2,top+24,w-80,font_,{244,250,250,255});
    if(donation_qr_) {
        SDL_Rect qr={(w-qr_size)/2,top+70,qr_size,qr_size};
        SDL_RenderCopy(renderer,donation_qr_,nullptr,&qr);
    } else {
        text("QR image unavailable",top+qr_size/2,large_,{255,255,255,255});
    }
    const std::string url="buymeacoffee.com/depius";
    int url_width=0;TTF_SizeUTF8(font_,url.c_str(),&url_width,nullptr);
    text_at(url,(w-url_width)/2,top+qr_size+82,w-60,font_,{77,226,185,255});
    const std::string hint="Back to return";
    int hint_width=0;TTF_SizeUTF8(small_,hint.c_str(),&hint_width,nullptr);
    text_at(hint,(w-hint_width)/2,h-48,w-40,small_,{160,184,198,255});
    SDL_RenderPresent(renderer);
}
void Shell::pairing_popup(const std::string& pin) {
    if(pin.empty())return;
    int w=0,h=0;SDL_GetRendererOutputSize(renderer,&w,&h);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer,0,0,0,0);SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,0,3,8,168);SDL_Rect backdrop={0,0,w,h};SDL_RenderFillRect(renderer,&backdrop);
    int box_w=std::min(w*82/100,1120),box_h=std::min(h*62/100,500);
    SDL_Rect panel={(w-box_w)/2,(h-box_h)/2,box_w,box_h};
    fill_round_rect(renderer,panel,28,{14,30,43,250});
    const std::string heading="AirPlay Pairing";
    int heading_width=0;TTF_SizeUTF8(large_,heading.c_str(),&heading_width,nullptr);
    text_at(heading,(w-heading_width)/2,panel.y+box_h/8,w-80,large_,{77,226,185,255});
    const std::string instruction="Enter this code on your Apple device";
    int instruction_width=0;TTF_SizeUTF8(font_,instruction.c_str(),&instruction_width,nullptr);
    text_at(instruction,(w-instruction_width)/2,panel.y+box_h*37/100,w-80,font_,{225,237,243,255});
    std::string spaced;
    for(size_t i=0;i<pin.size();++i) {if(i)spaced+="  ";spaced+=pin[i];}
    TTF_Font *display_pin_font=pin_font_;
    int pin_width=0;TTF_SizeUTF8(display_pin_font,spaced.c_str(),&pin_width,nullptr);
    if(pin_width>box_w*84/100) {display_pin_font=pin_font_small_;TTF_SizeUTF8(display_pin_font,spaced.c_str(),&pin_width,nullptr);}
    text_at(spaced,(w-pin_width)/2,panel.y+box_h*52/100,w-80,display_pin_font,{255,255,255,255});
    SDL_RenderPresent(renderer);
}
void Shell::screen(const std::string& status,const std::string& pin,int delay) {
    SDL_SetRenderDrawColor(renderer,14,25,36,255);SDL_RenderClear(renderer);
    int w,h;SDL_GetRendererOutputSize(renderer,&w,&h);
    text(UXAIR_DISPLAY_NAME,h/6,large_,{65,210,171,255});
    text(status,h/2-60,font_,{238,242,246,255});
    if(!pin.empty()) text(pin,h/2+10,large_,{255,255,255,255});
    else text(std::string("Open Screen Mirroring and select ")+UXAIR_DISPLAY_NAME,h/2+20,font_,{178,194,207,255});
    char sync[160];snprintf(sync,sizeof(sync),"◀ ▶  Audio delay: %+d ms  ·  Adjust before connecting",delay);
    text(sync,h-135,font_,{145,167,185,255});
    text("Same Wi-Fi network  ·  Quality in Settings  ·  Back to exit",h-80,font_,{145,167,185,255});
    SDL_RenderPresent(renderer);
}
Shell::~Shell() {if(donation_qr_)SDL_DestroyTexture(donation_qr_);if(font_)TTF_CloseFont(font_);if(large_)TTF_CloseFont(large_);if(small_)TTF_CloseFont(small_);if(pin_font_)TTF_CloseFont(pin_font_);if(pin_font_small_)TTF_CloseFont(pin_font_small_);TTF_Quit();if(renderer) SDL_DestroyRenderer(renderer); if(window) SDL_DestroyWindow(window); SDL_Quit(); }
