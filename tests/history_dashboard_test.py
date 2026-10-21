from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
shell = (root / "src/shell.cpp").read_text(encoding="utf-8")
navigation = (root / "src/ui_navigation.h").read_text(encoding="utf-8")
settings = (root / "src/settings.h").read_text(encoding="utf-8")

assert "HistoryPeriod" not in main + shell + navigation
assert '"Last 30 days",false' in shell
assert '"Last 12 months",true' in shell
assert "data.daily_bars" in shell and "data.monthly_bars" in shell
assert 'label=b.label+(b.partial?"*":"")' in shell
assert '"STARTED (UTC)"' in shell and '"DURATION"' in shell and '"SESSION"' in shell
assert "page_id!=UiPage::History&&page_id!=UiPage::System" in shell
assert "TTF_SizeUTF8(font_,rows[i].label.c_str()" in shell
assert "TTF_SizeUTF8(small_,rows[i].note.c_str()" in shell
assert "int width = 1920;" in settings and "int height = 1080;" in settings and "int max_fps = 30;" in settings
assert 'settings.width==1920?"1080p"' in main
assert 'std::to_string(settings.max_fps)+" FPS"' in main
print("PASS: permanent 30-day/monthly charts, partial-month marks, stable columns and About icon sizing")
