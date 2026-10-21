from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
shell = (root / "src/shell.cpp").read_text(encoding="utf-8")
navigation = (root / "src/ui_navigation.h").read_text(encoding="utf-8")
settings = (root / "src/settings.h").read_text(encoding="utf-8")

assert "HistoryPeriod" not in main + shell + navigation
assert "data.daily_title,false" in shell
assert "data.monthly_title,true" in shell
assert "data.daily_bars" in shell and "data.monthly_bars" in shell
assert 'if(b.partial)text_at("*"' in shell
assert '"STARTED (UTC)"' in shell and '"DURATION"' in shell and '"SESSION"' in shell
assert "page_id!=UiPage::History&&page_id!=UiPage::System" in shell
assert "TTF_SizeUTF8(font_,rows[i].label.c_str()" in shell
assert "TTF_SizeUTF8(small_,rows[i].note.c_str()" in shell
assert "int width = 1920;" in settings and "int height = 1080;" in settings and "int max_fps = 30;" in settings
assert 'settings.width==1920?"1080p"' in main
assert 'std::to_string(settings.max_fps)+" FPS"' in main
assert 'UsagePeriod::ThisMonth' in main and 'UsagePeriod::ThisYear' in main
assert 'history_day' in main
assert 'SDL_RenderSetClipRect(renderer,&plot)' in shell
assert 'SDL_RenderSetClipRect(renderer,had_clip?&previous_clip:nullptr)' in shell
assert 'Matej Skuhala (@depius)' in main and 'Based on UxPlay by FDH2; uxplay-webOS by @danlflh' in main
print("PASS: calendar charts, partial-month marks, stable columns and About icon sizing")
