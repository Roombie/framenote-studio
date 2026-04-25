#pragma once

namespace Framenote {

enum class ThemeMode {
    Dark,
    Light
};

class Theme {
public:
    static void applyDark();
    static void applyLight();
    static void apply(ThemeMode mode);
    static ThemeMode current() { return s_current; }
    static void toggle();

private:
    static ThemeMode s_current;
};

} // namespace Framenote
