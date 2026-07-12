// Utils.ixx
export module Utils;

import std;


export namespace Booleval::Style
{

    // Standard colors
    constexpr std::string_view bright{ "\033[1m" };
    constexpr std::string_view cyan{ "\033[36m" };
    constexpr std::string_view magenta{ "\033[35m" };
    constexpr std::string_view red{ "\033[31m" };
    constexpr std::string_view green{ "\033[32m" };
    constexpr std::string_view yellow{ "\033[33m" };
    constexpr std::string_view blue{ "\033[34m" };
    constexpr std::string_view white{ "\033[37m" };

    // High-Intensity (Bright) Foreground Colors
    constexpr std::string_view bright_red{ "\033[91m" };
    constexpr std::string_view bright_green{ "\033[92m" };
    constexpr std::string_view bright_yellow{ "\033[93m" };
    constexpr std::string_view bright_blue{ "\033[94m" };
    constexpr std::string_view bright_magenta{ "\033[95m" };
    constexpr std::string_view bright_cyan{ "\033[96m" };
    constexpr std::string_view bright_white{ "\033[97m" };

    constexpr std::string_view reset{ "\033[0m" };
    constexpr std::string_view bold{ "\033[1m" };  // Thick, vibrant text
    constexpr std::string_view dim{ "\033[2m" };  // Faint/muted (Great for output)
    constexpr std::string_view italic{ "\033[3m" };  // Good for types/comments
    constexpr std::string_view underline{ "\033[4m" };  // Good for errors

    constexpr std::string_view orange{ "\033[38;5;208m" }; // Bright programming orange
    constexpr std::string_view purple{ "\033[38;5;135m" }; // Deep violet
    constexpr std::string_view pink{ "\033[38;5;212m" }; // Neon pink accent
    constexpr std::string_view teal{ "\033[38;5;43m" };  // Deep sea green/blue
    constexpr std::string_view gold{ "\033[38;5;178m" }; // Soft dark yellow
    constexpr std::string_view lime{ "\033[38;5;118m" }; // Electric vibrant green
    constexpr std::string_view charcoal{ "\033[38;5;241m" }; // Middle gray   


}

export namespace Booleval::UI
{
    constexpr std::string_view prompt1{ "\N{RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK}" };
    constexpr std::string_view prompt2{ "\N{RIGHTWARDS DOUBLE ARROW}" };
}

// General utility functions
export namespace Booleval::Utils
{
    auto is_space(char ch) -> bool { return std::isspace(static_cast<unsigned char>(ch)); }

    auto is_identifier(char ch, bool init = false) -> bool {
        return (init ? std::isalpha(static_cast<unsigned char>(ch)) :
            std::isalnum(static_cast<unsigned char>(ch))) || ch == '_';
    }
    
    auto format_error(std::string_view message) -> std::string
    {
        return std::format("{}[Error]: {}{}", Style::bright_red, message, Style::reset);
    }

    // Max stream size
    constexpr auto streamsize_max = std::numeric_limits<std::streamsize>::max();


}