export module Tokens;
import Utils;
import std;

namespace Booleval {

    constexpr std::string_view help_msg{
R"help(===================================================================
                     booleval Interactive Help
===================================================================
Variables & Expressions:
  let <var> = true|false;   - Define a new variable with a global 
                              truth value.
  <var> = <expression>;     - Assign the result of expr to a non-const
                              variable.
  <expression>;             - Evaluate a boolean expression directly.
  Grouping Blocks           - Use (), [], or {} to enforce order.

Operators (Listed from HIGHEST to LOWEST precedence):
  1. ~   ~p                 - Negation (NOT)
  2. &   p & q              - Conjunction (AND)
  3. |   p | q              - Disjunction (OR)
  4. ->  p -> q             - Conditional (IF/THEN)
  5. <-> p <-> q            - Biconditional (IFF)

Commands:
  tab: <expr>;              - Generate step-by-step truth table.
  val: <p1>, <p2> // <c>;   - Validate an argument. 
                              (Separate premises with commas,
                               conclusion after the '//')
  help;                     - Display this help guide.
  quit;                     - Exit the interactive environment.
===================================================================)help" };

    // Convenient type
    export using values_t = std::vector<bool>;
    // Error handling
    export class token_error : public std::runtime_error 
    { using std::runtime_error::runtime_error; };

    export class Token {
    public:
        
        enum class Kind {
            empty,
            unexpected,
            semicolon,
            truth_table,
            arg_eval,
            assignment,
            definition,
            negation,
            conjunction,
            disjunction,
            implies,
            iff,
            identifier,
            comma,
            therefore,
            quit,
            lparen,
            rparen,
            lbrace,
            rbrace,
            lbracket,
            rbracket,
            help,
            newline,
            comment,
            colon,
            readonly,
            dir, // inspection
        };

        using action_t = values_t(*)(values_t const&, [[maybe_unused]] values_t const&);
        using noargs_action_t = void(*)();
        // ctors    
        constexpr explicit Token(Kind k = Kind::empty,
            std::string_view name = "",
            action_t action = {},
            noargs_action_t action0 = {})
            : kind{ k }, name{ name }, action{ action }, action0{ action0 }
        {}

        // conversion
        [[nodiscard]] explicit operator bool() const noexcept {
            return kind != Kind::empty && kind != Kind::quit;
        }

        // conversion
        [[nodiscard]]explicit operator Kind() const noexcept {
            return kind;
        }

        auto operator()(this auto&& self, values_t const& v0, values_t const& v1)
        {
            using S = std::string;

            switch (self.kind) {
                using enum Kind;
            case negation:
            case conjunction:
            case disjunction:
            case implies:
            case iff:
                return self.action(v0, v1);
            default:
                throw std::runtime_error{ S{"calling operator() on a: "} + S{self.name} };
            }
        } 

        auto operator()(this auto&& self)
        {
            using S = std::string;

            switch (self.kind) {
                using enum Kind;
            case help:
                return self.action0();
            default:
                throw std::runtime_error{ S{"calling no args operator() on a: "} + S{self.name} };
            }
        }

        bool operator==(Token const& rgt) const noexcept
        {
            if (std::empty(name) || std::empty(rgt.name))
                return kind == rgt.kind;
            return kind == rgt.kind && name == rgt.name;
        }      

        friend bool operator==(Token const& lft, Token::Kind kind) {
            return lft.kind == kind;
        }

        friend bool operator==(Token::Kind kind, Token const& rgt) {
            return rgt.kind == kind;
        }

        friend bool operator==(Token const& lft, std::string_view rgt) {
            return lft.name == rgt;
        }
       

        friend std::ostream& operator<<(std::ostream& out, Token const&);
        auto get_name() const -> std::string { return std::string{ name }; }

    private:
        Kind kind{};
        std::string_view name{};
        [[maybe_unused]] action_t action{};
        [[maybe_unused]] noargs_action_t action0{};
    };

       
    // Convenient aliases
    namespace rgs = std::ranges;
    namespace vws = std::views;
    // Operations on values_t
    const auto transf = [](auto&& op, auto const& ...vs) ->values_t {
        if constexpr (sizeof...(vs)==1) 
            return vs...[0] | vws::transform(op) | rgs::to<values_t>();
        else 
            return vws::zip_transform(op, vs...) | rgs::to<values_t>();
    };
    //*************************************************************    
    // Predifined Tokens
    //*************************************************************
    export constexpr Token empty{};
    export constexpr Token eof = empty;
    export constexpr Token newline{ Token::Kind::newline, "newline" };
    export constexpr Token colon{ Token::Kind::colon, ":" };
    export constexpr Token readonly{ Token::Kind::readonly, "ro" };
    export constexpr Token dir{ Token::Kind::dir, "dir" };
    export constexpr Token comment{ Token::Kind::comment, "#" };
    export constexpr Token identifier{ Token::Kind::identifier, "" };
    export constexpr Token unexpected{Token::Kind::unexpected, "unexpected"};
    export constexpr Token comma{Token::Kind::comma, ","};
    export constexpr Token therefore{ Token::Kind::therefore, "//" };
    export constexpr Token quit{ Token::Kind::quit, "quit" };
    export constexpr Token semicolon{ Token::Kind::semicolon, ";" };
    export constexpr Token definition{ Token::Kind::definition, "let" };
    export constexpr Token assignment { Token::Kind::assignment, "=" };
    export constexpr Token truth_table{ Token::Kind::truth_table, "tab" };
    export constexpr Token arg_eval{ Token::Kind::arg_eval, "val" };
    export constexpr Token lparen{ Token::Kind::lparen, "(" };
    export constexpr Token rparen{ Token::Kind::rparen, ")" };
    export constexpr Token lbrace{ Token::Kind::lbrace, "{" };
    export constexpr Token rbrace{ Token::Kind::rbrace, "}" };
    export constexpr Token lbracket{ Token::Kind::lbracket, "[" };
    export constexpr Token rbracket{ Token::Kind::rbracket, "]" };
    export constexpr Token help{ Token::Kind::help, "help", {},
        [] { std::println("{}", help_msg); } 
    };
    // logical operator not
    export constexpr auto negation = Token{ Token::Kind::negation, "~",
        [](values_t const& vals, values_t const& /*unused*/)->values_t {
            return transf(std::logical_not{}, vals);
        }
    };
    // logical operator and
    export constexpr auto conjunction = Token{ Token::Kind::conjunction, "&",
        [](values_t const& vals0, values_t const& vals1)->values_t {
            return transf(std::logical_and{}, vals0, vals1);
        }
    };
    // logical operator or
    export constexpr auto disjunction = Token{ Token::Kind::disjunction, "|",
        [](values_t const& vals0, values_t const& vals1)->values_t {
            return transf(std::logical_or{}, vals0, vals1);
        }
    };
    // logical operator implies 
    export constexpr auto implies = Token{ Token::Kind::implies, "->",
        [](values_t const& vals0, values_t const& vals1)->values_t {
            return transf([](auto&& a, auto&& b) { return !a || b; }, vals0, vals1);
        }
    };
    // logical operator iff
    export constexpr auto iff = Token{ Token::Kind::iff, "<->",
        [](values_t const& vals0, values_t const& vals1)->values_t {
            return transf([](auto&& a, auto&& b) { return a == b; }, vals0, vals1);
        }
    };

    
    // Stream of tokens
    export class Token_stream
    {
        Token buffer{}; // default is Empty{}
        std::istream* in_;
        std::istream& in() { return *in_; }
        std::unordered_set<std::string> idents{}; // identifiers pool

        
    public:
        // Ctor
        explicit Token_stream(std::istream& in = std::cin) :
            in_{ &in } 
        {
            idents.reserve(512);
        }
        [[nodiscard]] Token get0();
        [[nodiscard]] Token get();

        void put_back(Token);

        Token peek()
        {
            if (buffer == empty) 
                buffer = get();
            return buffer;
        }

        std::string as_string_until(Token const& tk);

        void ignore_until(Token delim)
        {
            for (Token tk = get(); tk != delim; )
                tk = get();
        } 

        void flush_line();

    };

    std::string Token_stream::as_string_until(Token const& tk)
    {
        std::string expr;

        while (tk != peek()) 
            expr += get().get_name();
        return expr;
    }

    void Token_stream::flush_line()
    {
        buffer = empty;

        if (in().fail()) in().clear();

        in().ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    // Doesn't skip white newlines
    Token Token_stream::get0()
    {
        if (buffer != empty) {
            Token tk = buffer;
            buffer = empty;
            return tk;
        }
        //using Char = int;

        char ch = static_cast<char>(in().get());

        while (Utils::is_space(ch))
        {
            if (ch == '\n') return newline;
            ch = static_cast<char>(in().get());
        }

        if (in().fail()) return eof; 

        // read comments
        if (ch == '#')
        {
            //ignore until newline
            in().ignore(Utils::streamsize_max, '\n');
            return newline;
            
        }


        // Safe character classification casting
        if (Utils::is_identifier(ch, true)) {
            std::string name{ ch };

            while (in().get(ch)) {
                if (Utils::is_identifier(ch)) {
                    name += ch;
                }
                else {
                    in().putback(ch);
                    break;
                }
            }

            // keywords
            if (name == dir)         return dir;
            if (name == readonly)    return readonly;
            if (name == quit)        return quit;
            if (name == definition)  return definition;
            if (name == truth_table) return truth_table;
            if (name == arg_eval)    return arg_eval;
            if (name == help)        return help;

            // keep the identifier alive
            auto [it, _] = idents.insert(std::move(name));
            return Token{ Token::Kind::identifier, *it };
        }
        else {
            switch (ch) {
            case ';': return semicolon;
            case ':': return colon;
            case '=': return assignment;
            case '(': return lparen;
            case ')': return rparen;
            case '{': return lbrace;
            case '}': return rbrace;
            case '[': return lbracket;
            case ']': return rbracket;
            case '~': return negation;
            case '&': return conjunction;
            case '|': return disjunction;
            case ',': return comma;
            case '-': {
                if (in().peek() == '>') {
                    in().get(); // consume '>'
                    return implies;
                }
                throw token_error{ std::format("unexpected: -{}",
                    static_cast<char>(in().peek())) };
            }
            case '<': {
                if (in().peek() == '-') {
                    in().get(); // consume '-'
                    if (in().peek() == '>') {
                        in().get(); // consume '>'
                        return iff;
                    }
                    // It's definitely an error. Let's see what the bad character actually was:
                    char bad = static_cast<char>(in().peek());
                    throw token_error{ std::format("invalid operator: <-{}", bad) };
                }
                throw token_error{ "unexpected token: '<'" };
            }
            case '/': {
                if (in().peek() == '/') {
                    in().get(); // consume second '/'
                    return therefore;
                }
                throw token_error{ std::format("unexpected token: '/{}'",
                    static_cast<char>(in().peek())) };
            }
            default: throw token_error{ std::format("unexpected token: '{}'", ch) };

            }
        }
    }
  

    // Skip newlines
    Token Token_stream::get()
    {
        while (true){
            if (auto tok = get0(); tok == newline)
                continue;      
            else return tok;
        }

    }

    void Token_stream::put_back(Token tk)
    {
        if (buffer != empty)
            throw token_error{ "putting back into a full buffer" };
        buffer = tk;
    } 

    // clean up mess
    

}//: Booleval

// Format capability for Booleval::Token
template <> struct std::formatter<Booleval::Token> {

   /* constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }*/

    auto format(Booleval::Token const& tk, std::format_context& ctx) const {
      
        return std::format_to(ctx.out(), "Token{{name=<{}>}}", tk.get_name());
    }
};