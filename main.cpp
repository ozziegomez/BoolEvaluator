// main.cpp
import Truth_table;
import Utils;
import std;

void setup_terminal();

namespace Booleval
{
    struct Var
    {
        std::string name;
        bool value;
        bool readonly;
    };     

    // sym_table
    auto sym_table = std::vector<Var>{
        // predefined constants
        {"true",  true, true}, 
        {"false", false, true}, 
    };

    // fwd decl
    auto proposition(Token_stream&) -> bool;
    auto conditional(Token_stream&) -> bool;
    auto term(Token_stream&) -> bool;
    auto factor(Token_stream&) -> bool;
    auto primary(Token_stream&) -> bool;

   
    auto is_defined(std::string_view name) -> bool
    {
        using std::ranges::any_of;
        return any_of(sym_table, [name](auto&& v) {return name == v.name;});
    }
    

    decltype(auto) get_var(std::string_view name)
    {
        using std::ranges::find;
        if (!is_defined(name))
            throw std::runtime_error{ std::format("{}: not a defined variable",name) };
        return *find(sym_table, name, &Var::name);

    }

    auto define(Token_stream& tin) -> std::tuple<std::string, bool>
    {
        // assume "let" have been called
        bool ro /*[[uninitialized]]*/;
        auto var_name = std::string{};

        if (auto id_or_qualifier = tin.get(); id_or_qualifier == readonly)
        {
            if (tin.peek() != identifier)
                throw std::runtime_error{ "expected identifier" };
            var_name = tin.get().get_name(); // consume identifier
            ro = true;
        }
        else if (id_or_qualifier == identifier)
        {
            var_name = id_or_qualifier.get_name();
            ro = false;
        }
        else throw std::runtime_error{ "expected type qualifier or identifier" };


        
        if (is_defined(var_name))
            throw std::runtime_error{ std::format("redefinition of: '{}'", var_name) };

        // look for assignment
        if(tin.peek() != assignment)
            throw std::runtime_error{ std::format("'{}' expected", assignment.get_name()) };

        auto _ = tin.get(); // consume "="

        // get value
        auto value = proposition(tin);

        sym_table.emplace_back(var_name,value, ro);// Var{var_name, value, ro}

        return { var_name, value };

    }


    auto proposition(Token_stream& tin) -> bool
    {
        auto left = conditional(tin);
        while (tin.peek() == iff)
        {
            auto _ = tin.get();
            auto right = conditional(tin);
            left = left == right;
        }
        return left;    
    }

    auto conditional(Token_stream& tin) -> bool
    {
        auto left = term(tin);

        while (tin.peek() == implies)
        {
            auto _ = tin.get();
            auto right = term(tin);
            left = !left || right;
        }
        return left;
    }

    auto term(Token_stream& tin) -> bool
    {
        auto left = factor(tin);

        while (tin.peek() == disjunction)
        {
            auto _ = tin.get();
            auto right = factor(tin);
            left = left || right;
        }
        return left;
    }

    auto factor(Token_stream& tin) -> bool
    {
        auto left = primary(tin);
        while (tin.peek() == conjunction)
        {
            auto _ = tin.get(); // consume conjunction token
            auto right = primary(tin);
            left = left && right;
        }
        return left;
    }

    
    auto primary(Token_stream& tin) -> bool
    { 
        using std::ranges::subrange;
        
        if (auto tk = tin.get(); tk == identifier)
        {
            auto name = tk.get_name();
            if (!is_defined(name)) {
                throw std::runtime_error{ std::format("identifier '{}' not defined", name)};
            }
            if (tin.peek() == assignment)
            {
                auto _ = tin.get(); // cleanly consume the "="
                // TO-DO: Check if variable is const
                if (Var& v = get_var(name); !v.readonly)
                    v.value = proposition(tin);
                else 
                    throw std::runtime_error{ std::format("trying to mutate const variable: '{}'", v.name) };
                    
            }
            return get_var(name).value;
        }
        else if (tk == negation)
        {
            return !primary(tin);
        }
        else if (tk == lparen || tk == lbrace || tk == lbracket) // openning brackets
        {
            auto expr = proposition(tin);
            auto ctk = tin.get(); // closing bracket

            // matched
            if (tk == lparen && ctk == rparen)     return expr;
            if (tk == lbrace && ctk == rbrace)     return expr;
            if (tk == lbracket && ctk == rbracket) return expr;

            // error
            if (ctk == rparen || ctk == rbrace || ctk == rbracket) {
                throw std::runtime_error{ "mismatched brackets" };
            }

            // not a token
            throw std::runtime_error{ "unexpected token" };
        }
        // not a primary
        throw std::runtime_error{ "expected a primary" };
        
    }
    ///////////////////////////////////////////////////////////////////////////////
    void stmt_terminator(Token_stream& tin)
    {
        if (auto tk = tin.get(); tk != semicolon)
            throw std::runtime_error{ "missing semicolon" };
        else
        {
            while (tk == semicolon) 
                tk = tin.get0();
            tin.put_back(tk);
        }        
    }

    bool finalize_stmt(Booleval::Token_stream& tin)
    {
        stmt_terminator(tin); // check for semicolon        
        if (auto t0 = tin.get0(); t0 == newline)
            return true;
        else tin.put_back(t0);
        return false;
    }
    ////////////////////////////////////////////////////////////////////////


    class Program
    {
    public:
        explicit Program(std::istream&);
        int run(char**, int);
        void print_banner() const;
    private:
        bool prompt = true;
        Token_stream tin;
    };
    //ctor
    Program::Program(std::istream& in = std::cin) : prompt{ true }, tin{ in } {}

    void Program::print_banner() const
    {
        println("{}{}{}", Style::magenta, R"prompt(booleval v0.2.0-alpha (Interactive Logic Engine)
Type 'help;' for more information or 'quit;' to exit
)prompt", Style::reset);
    }

    int Program::run(char **argv, int argc)
    {
        using namespace Booleval::Style;
        using namespace std;
        setup_terminal();  
        print_banner();      
        // Program arguments
        [[maybe_unused]] std::vector<char const*> args{ argv, argv+argc };
        
        while (true) try
        {
            if (prompt){
                prompt = false;
                print(std::cout,"{}{}{} ", cyan, UI::prompt1, reset);
                std::cout.flush();
            }
         
            // check if newline caused by whole line comments
            auto tk = tin.get0();

            if (tk == newline)
            {
                prompt = true;
                continue;
            }

            if (tk == eof) // EOF
                return 0;

            if (tk == quit)
            {
                stmt_terminator(tin);
                return 0;
            }

            if (tk == semicolon)
            {
                while (tk == semicolon) tk = tin.get0();

                if (tk == newline) {
                    prompt = true;
                    continue;
                }
            }

            if (truth_table == tk) 
            {
                if (auto c = tin.get(); c != colon)
                    throw std::runtime_error{ "':' expected" };
                std::println("{}", make_table(tin));
                prompt = finalize_stmt(tin);       
            }
            else if (tk == definition) // let x = true;
            {
                auto [id, result] = define(tin);
                prompt = finalize_stmt(tin);
                std::println("{}{} {} {}{}", yellow,id, UI::prompt2, result, reset);
               
            }
            else if (arg_eval == tk) 
            {
                if (auto c = tin.get(); c != colon)
                    throw std::runtime_error{ "':' expected" };
                eval_argument(tin); 
                prompt = finalize_stmt(tin);
            }
            else if (help == tk)
            {
                help();
                prompt = finalize_stmt(tin); // check for semicolon
              
            }                                                     
            else if (dir == tk)
            {
                std::print("[");
                for(int i=0; Var const& v: sym_table)
                    std::print("{}{}'{}'{}", yellow, i++?", ": "", v.name, reset);
                std::println("]");
                prompt = finalize_stmt(tin);
            }
            else
            {
                tin.put_back(tk);
                auto result = proposition(tin);
                prompt = finalize_stmt(tin);
                std::println("{}{} {}{}", yellow,UI::prompt2, result, reset);
                
            }
        }
        catch (const std::runtime_error& e)
        {
            std::println(std::cerr, "{}", Utils::format_error(e.what()));      
            //clean up mess
            tin.flush_line();
            //tin.ignore_until(newline);
            prompt = true;
        }

        return 0;

    }

    ///////////////////////////////////////////////////////////////////////////


}// Booleval

namespace std {

    template<> struct formatter<Booleval::Var> {
        constexpr auto parse(std::format_parse_context& ctx) {
            return ctx.begin();
        }

        auto format(const Booleval::Var& var, std::format_context& ctx) const {
            return std::format_to(ctx.out(),
                "{{name={}, value={}, readonly={}}}",
                var.name, var.value, var.readonly);
        }
    };


}// namespace std;


int main(int argc, char *argv[]) try
{
    auto evaluator = Booleval::Program{};
    return evaluator.run(argv + 1, argc - 1);

}
catch (...)
{
    std::println("Error: unhandled exception");
    return -1;
}

