module;

export module Truth_table;
export import Tokens;
import std;

namespace Booleval {

    export struct Base_table_tag {};
    export Base_table_tag base_table;
 

#if defined(_MSC_VER) && !defined(__clang__)
	// Temporary shim because MSVC hasn't implemented C++26's P3060R3 yet
	export template<std::integral I> constexpr auto positions(I n)
	{
		return std::views::iota(I{}, n);
	}
	template <typename T>
	auto indices(T&& depth) {
		return positions(std::forward<T>(depth));
	}
#else
	using std::views::indices;
#endif


    export class Table
    {
    public:       
        // ctors
        Table() = default;
        explicit Table(Token_stream& tin);

        explicit Table(Table const& other, Base_table_tag) :
            rows_{ other.rows_ },
            keys_{std::begin(other.var_names), std::end(other.var_names) },
            values_(std::begin(other.values_), std::begin(other.values_) + std::size(keys_)),
            var_names{ std::begin(keys_), std::end(keys_) }
        {}

        explicit Table(std::vector<std::string> rng) :
            rows_{ static_cast<int>(1ull << std::ssize(rng)) },
            keys_{ std::from_range, std::move(rng) },
            values_(std::ssize(keys_)),
            var_names{keys_.begin(), keys_.end()}
        {
            init();
        }
      
        // Access colum by a key
        auto operator[](this auto&&, std::string_view);      
       
        // Add another column/new expr
        void push_back(std::string, values_t&&);

        auto ssize() const { return std::ssize(values_); }
        auto depth() const { return rows_; }
        auto const& keys() const { return keys_; }
        auto values() const -> std::vector<values_t> const& { return values_; }

        void update_key(std::string const&, std::string);  
        
        std::string statement(this auto&&);      

        // atomic variables
        auto variable_names(this auto&& self)->const std::vector<std::string>& 
        { return self.var_names; }

        // the whole proposition
        auto prop(this auto&& self)-> const std::string 
        { return self.buffer; }

        //
        std::string parse_string(std::string);    
        
    private:
        int rows_; // depth
        std::vector<std::string> keys_;
        std::vector<values_t> values_;
        std::vector<std::string> var_names;
        std::istringstream input;
        Token_stream tin_;
        std::string buffer;
        // setup initial table
        void init();
        // fill table
        void parse();
        auto proposition() -> std::string;
        auto conditional() -> std::string;
        auto term() -> std::string;
        auto factor() -> std::string;
        auto primary() -> std::string;

    };

    // Table implementation

    Table::Table(Token_stream& tin) {

        for (auto tk = tin.peek(); semicolon != tk; tk = tin.peek()) {
            tk = tin.get(); // consume non-semicolons
            if (tk == identifier) {
                using std::ranges::find;
                if (find(var_names, tk.get_name()) == std::end(var_names))
                    var_names.push_back(tk.get_name());
            }

            buffer += tk.get_name();
        }

        // recreate input stream
        input.str(buffer);
        input.clear(); // Clears flags
        tin_ = Token_stream{ input };
        // rows
        rows_ = static_cast<int>(1ull << var_names.size());

        // init keys_ and values_
        keys_.append_range(var_names);
        values_.resize(keys_.size());

        // initial form of table
        init();
        // fill table
        parse();

    }

    void Table::init()
    {
        //using std::ranges::views::enumerate;
        using std::views::enumerate;
        using std::views::transform;
        

        for(auto&& [i, col] : enumerate(values_))
        {
            col.reserve(depth());
            auto const seg = depth() / (1 << (1 + i)); // segment
            auto const pattern = [seg](auto r) { return (r/seg)%2 == 0; };
            // add column
            col.append_range(indices(depth()) | transform(pattern)); // col.append_range
        }//~ for

    }
    
    void Table::parse()
    {
        buffer = proposition();
    }

    std::string Table::parse_string(std::string expr)
    {
        input.clear(); // Clear stream flags just in case
        input.str(expr);
        tin_ = Token_stream{ input };

        parse(); 
        return prop(); 
    }

    std::string Table::statement(this auto&& self)
    {
        const auto all_true = [](auto&& e) noexcept { return true == e;};
        using std::ranges::all_of;
        using std::ranges::none_of;
        return std::format("{}: {}", self.prop(),
            all_of(self.values_.back(), all_true) ? "tautology" :
            none_of(self.values_.back(), all_true) ? "contradiction" :
            "contingency");
    }

    void Table::update_key(std::string const& oldexpr, std::string newexpr)
    {
        using std::ranges::find;
        using std::ranges::distance;

        if (auto it = find(keys_, oldexpr); it != std::end(keys_)) {
            *it = std::move(newexpr);
        }
        else throw std::out_of_range{ std::format("{} is not a key", oldexpr) };
    }

    auto Table::operator[](this auto&& self, std::string_view expr)
    {
        using std::ranges::find;
        using std::ranges::distance;

        if (auto it = find(self.keys_, expr); !(std::end(self.keys_) == it))
            return self.values_[it - std::begin(self.keys_)];
        // expr is not in the table
        throw std::out_of_range{ std::string{expr} + std::string{": not in the table" } };
    }

    // proposition: handles lowest precedence Iff (<->)
    auto Table::proposition() -> std::string {
        auto left = conditional(); // Falls through to conditional

        while (tin_.peek() == iff)
        {
            auto _ = tin_.get();// read peeked token
            
            auto right = conditional();
            auto newexpr = std::format("{} {} {}", left, iff.get_name(), right);
            this->push_back(newexpr, iff((*this)[left], (*this)[right]));
            left = newexpr;       
        }
        return left;
    }

    // conditional: handles Implication (->)
    auto Table::conditional() -> std::string {
        auto left = term(); // Falls through to term

        while (tin_.peek()== implies)
        {
            auto _ = tin_.get(); // read
            auto right = term();
            auto newexpr = std::format("{} {} {}", left, implies.get_name(), right);
            push_back(newexpr, implies((*this)[left], (*this)[right]));
            left = newexpr;
          
        }
        return left;
    }

    // term: handles logical OR (|)
    auto Table::term() -> std::string
    {
        auto left = factor();

        while (tin_.peek()==disjunction)
        {
            auto _ = tin_.get();
            auto right = factor();
            auto newexpr = std::format("{} {} {}", left, disjunction.get_name(), right);
            push_back(newexpr, disjunction((*this)[left], (*this)[right]));
            left = newexpr;         
        }
        return left;
    }

    // factor: handles logical AND (&)
    auto Table::factor() -> std::string
    {
        auto left = primary();

        while (tin_.peek()==conjunction)
        {
            auto _ = tin_.get();
            auto right = primary();
            auto newexpr = std::format("{} {} {}", left, conjunction.get_name(), right);
            push_back(newexpr, conjunction((*this)[left], (*this)[right]));
            left = newexpr;          
        }
        return left;
    }

    

auto Table::primary() -> std::string
{
    /*if (auto tok = tin_.get(); tok == Token::Kind::identifier) {*/
    if (auto tok = tin_.get(); tok == identifier) {
        return tok.get_name();
    }
    else if (tok == negation) {
        auto expr = primary();
        auto vals = (*this)[expr];
        expr = std::format("{}{}", negation.get_name(), expr);
        push_back(expr, negation(vals, values_t{}));
        return expr;
    }
    else if (tok == lparen || tok == lbrace || tok == lbracket)
    {
        auto expr = proposition();
        bool matched = false;
        using std::ranges::contains;

        if (auto rght = tin_.get(); rght == rparen || rght == rbrace || rght == rbracket)
        {
            if (tok == lparen && rght == rparen) 
            {
                if (!contains(var_names, expr)) {
                    update_key(expr, std::format("({})", expr));
                    expr = std::format("({})", expr);
                }
                matched = true;
            }
            else if (tok == lbrace && rght == rbrace) 
            {
                if ((!contains(var_names, expr))) {
                    update_key(expr, std::format("{{{}}}", expr));
                    expr = std::format("{{{}}}", expr);
                }
                matched = true;
            }
            else if (tok == lbracket && rght == rbracket) 
            {
                if ((!contains(var_names, expr))) {
                    update_key(expr, std::format("[{}]", expr));
                    expr = std::format("[{}]", expr);
                }
                matched = true;
            }

            if (!matched)
                throw token_error{ "mismatched bracket" }; // throw parse_error
            return expr;
        }
        else throw token_error{ "expected a right bracket" };
    }
    else
        throw token_error{ "expected a primary" };
}//:~ primary

	//auto Table::primary() -> std::string
	//{
	//	if (auto tok = tin_.get(); tok == identifier) {
	//		return tok.get_name();
	//	}
	//	else if (tok == negation) {
	//		auto expr = primary();
	//		auto vals = (*this)[expr];
	//		expr = std::format("{}{}", negation.get_name(), expr);
	//		push_back(expr, negation(vals, values_t{}));
	//		return expr;
	//	}
	//	else if (tok == lparen || tok == lbrace || tok == lbracket)
	//	{
	//		auto expr = proposition();
	//		bool matched = false;
	//		std::string framed_expr = expr;

	//		if (auto rght = tin_.get(); rght == rparen || rght == rbrace || rght == rbracket)
	//		{
	//			if (tok == lparen && rght == rparen) {
	//				if (std::ranges::find(var_names, expr) == std::end(var_names)) {
	//					framed_expr = std::format("({})", expr);
	//					if (std::ranges::find(keys_, framed_expr) == std::end(keys_)) {
	//						push_back(framed_expr, values_t{ (*this)[expr] });
	//					}
	//				}
	//				matched = true;
	//			}
	//			else if (tok == lbrace && rght == rbrace) {
	//				if (std::ranges::find(var_names, expr) == std::end(var_names)) {
	//					framed_expr = std::format("{{{}}}", expr);
	//					if (std::ranges::find(keys_, framed_expr) == std::end(keys_)) {
	//						push_back(framed_expr, values_t{ (*this)[expr] });
	//					}
	//				}
	//				matched = true;
	//			}
	//			else if (tok == lbracket && rght == rbracket) {
	//				if (std::ranges::find(var_names, expr) == std::end(var_names)) {
	//					framed_expr = std::format("[{}]", expr);
	//					if (std::ranges::find(keys_, framed_expr) == std::end(keys_)) {
	//						push_back(framed_expr, values_t{ (*this)[expr] });
	//					}
	//				}
	//				matched = true;
	//			}

	//			if (!matched)
	//				throw token_error{ "mismatched bracket" };

	//			return framed_expr; // Returns the correctly bracketed string string wrapper
	//		}
	//		else throw token_error{ "expected a right bracket" };
	//	}
	//	else
	//		throw token_error{ "expected a primary" };
	//}
 
    void Table::push_back(std::string expr, values_t&& vals)
    {
       
        using std::ranges::contains;
        // Ignore duplicates
        if (!contains(keys_, expr))
        {
            keys_.push_back(std::move(expr));
            values_.emplace_back(std::move(vals)); 
        }
    }


    // Argument Evaluation
    class Arg_evaluator
    {
    public:
        Arg_evaluator() = default;
        explicit Arg_evaluator(Token_stream& tin);

        auto as_base() const { return Table{ base_, base_table }; }
        auto const& premises() const { return premises_; }        
        auto depth() const { return base_.depth(); }

        bool operator[](std::string_view k, int row) const 
        {
            return base_[k][row];
        }
    private:
        // last premise is conclusion
        std::vector<std::string> premises_;
        Table base_;
    };

    // implementation
    export Arg_evaluator::Arg_evaluator(Token_stream& tin) 
    {
        std::vector<std::string> prems{};
        std::string prem;
        std::vector<std::string> atomic_vars{};
        for (auto tk = tin.peek(); tk != semicolon; tk = tin.peek())
        {
            tk = tin.get(); // consume non=semicolons
            using std::ranges::contains;
            if (tk == identifier)
            {
                auto n = tk.get_name();
                if(!contains(atomic_vars, n))
                    atomic_vars.push_back(n);
                prem += n;
            }
            else if (tk == comma || tk == therefore)
            {
                prems.push_back(prem);
                prem.clear();
            }
            else prem += tk.get_name();
        }

        if (!std::empty(prem))
        {
            prems.push_back(prem);
        }

        // Construct base by moving
        base_ = Table{ atomic_vars };

        for (auto&& p : prems)
        {
            Table t0{ base_, base_table };
            auto col = t0.parse_string(p);
            base_.push_back(col, t0[col]);
            this->premises_.push_back(col);
        }
    }

}//: Booleval namesoace


// Format capability for Booleval::Table
template <> struct std::formatter<Booleval::Table> {

    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(Booleval::Table const& tt, std::format_context& ctx) const {
        
#if defined(_MSC_VER) && !defined(__clang__)
        using Booleval::indices;
#else 
        using std::views::indices;
#endif

        namespace vs = std::views;
        namespace rg = std::ranges;
        // Build Header 
        auto header = vs::enumerate(tt.keys())
            | vs::transform([](auto&& tup) {
            auto&& [i, k] = tup;
            return std::format("{}{:^{}}", i++ ? " | " : "  ", k, k.size() + 2);
            })
            | vs::join | rg::to<std::string>();

        // Print header
        std::format_to(ctx.out(), "{}\n", header);

        // Separator
        auto const sep = std::format("{:=^{}}", "", header.size());
        std::format_to(ctx.out(), "{}\n", sep);

        // Print Rows
        for (auto row : indices(tt.depth())) {
            for (int i = 0; auto const& key : tt.keys()) 
            {
                std::format_to(ctx.out(), "{}{:^{}}", 
                    i++ ? " | " : "  ", static_cast<bool>(tt[key][row]) ? "T" : "F", 
                    key.size() + 2);
            }
            std::format_to(ctx.out(), "\n");
        }
        return std::format_to(ctx.out(), "{}", sep);
    }
};

// Format capability for Booleval::Arg_Evaluator
template <> struct std::formatter<Booleval::Arg_evaluator> {

    constexpr auto parse(std::format_parse_context& ctx) 
    {
        return ctx.begin();
    }

    auto format(Booleval::Arg_evaluator const& ae, std::format_context& ctx) const
    {
#if defined(_MSC_VER) && !defined(__clang__)
        using Booleval::indices;
#else
        using std::views::indices;
#endif
        namespace vs = std::views;
        namespace rg = std::ranges;
        ///////////////////////////////////////////////////////////
        constexpr std::string_view bright_red{ "\033[91m" };
        constexpr std::string_view bright_green{ "\033[92m" };
        constexpr std::string_view reset{ "\033[0m" };
        ///////////////////////////////////////////////////////////
        auto premises = ae.premises() | vs::take(size(ae.premises()) - 1);
        auto conclusion = ae.premises().back(); 

        // Are all the premises on this row true?
        auto const all_prems_on_row =
            [&](auto row) { return rg::all_of(premises, [&](auto&& pre) { return ae[pre, row]; });};

        // Calculate the prefix width for "row X: " dynamically
        auto const prefix = std::format("row {}: ", ae.depth());        

        auto header = std::format("{:<{}}", "", size(prefix))
            + (vs::enumerate(premises)
            | vs::transform([](auto const& tup) {
            auto const& [i, p] = tup;
            return std::format("{}{:^{}}", i ? " , " : "   ",p,  size(p) + 2);
                })
            | vs::join | rg::to<std::string>())
            + std::format(" // {:^{}}", conclusion, size(conclusion) + 2);
       
        // Print header
        std::format_to(ctx.out(), "{}\n", header);

        // Print separator
        std::format_to(ctx.out(), "{:-^{}}\n", "", size(header));
        //---------------------------------------------------------------------------------
        // Print rows
        rg::for_each(indices(ae.depth()), [&](auto row) {
            //====================================================================================
            auto const valid = all_prems_on_row(row) && ae[conclusion, row];
            auto const counterexample = all_prems_on_row(row) && !ae[conclusion, row];
            // Row Prefix
            // All premises and conclusion true
            if(valid)
                std::format_to(ctx.out(), "{:<{}}", std::format("{}row {}{}: ", 
                    bright_green ,row + 1, reset), size(prefix));
            else if (counterexample)
                std::format_to(ctx.out(), "{:<{}}", std::format("{}row {}{}: ",
                    bright_red, row + 1, reset), size(prefix));
            else std::format_to(ctx.out(), "{:<{}}", std::format("row {}: ", row + 1), 
                size(prefix));

            //====================================================================================
            // Inner Premises
            rg::for_each(vs::enumerate(premises), [&](auto const& tup) {
                auto const& [i, p] = tup;
                
                std::format_to(ctx.out(), "{}{:^{}}", i ? " | " : "   ", ae[p, row] ? "T" : "F", 
                    size(p) + 2);
                });
            //====================================================================================
            // Conclusion
            if(valid)
                std::format_to(ctx.out(), " // {:^{}} {}\n", ae[conclusion, row] ? "T" : "F", 
                    size(conclusion) + 2, "\N{WHITE HEAVY CHECK MARK}");
            else if(counterexample)
                std::format_to(ctx.out(), " // {:^{}} {}\n", ae[conclusion, row] ? "T" : "F",
                    size(conclusion) + 2, "\N{CROSS MARK}");
            else std::format_to(ctx.out(), " // {:^{}}\n", ae[conclusion, row] ? "T" : "F",
                size(conclusion) + 2);
            });   
        //-----------------------------------------------------------------------------------
        // Predicate
        auto const is_fallacy = [&](auto row) {
            return !ae[conclusion, row] && rg::all_of(premises, [&](auto&& pre){ return ae[pre,row]; });
        };

        // If no fallacies
        if (rg::none_of(indices(ae.depth()), is_fallacy)) {
            return std::format_to(ctx.out(), "\n{}Conclusion logically follows. The argument is valid{}\n",
                bright_green, reset);
        }
        else {
            std::format_to(ctx.out(), "\nInvalid argument {}(fallacy found){}\n", bright_red, reset);
            return std::format_to(ctx.out(), "Counterexample matching rows: {}{:n}{}", bright_red,
                indices(ae.depth())
                | vs::filter(is_fallacy)
                | vs::transform([](auto row) { return row + 1; }), reset);
        }
    }   
};

namespace Booleval
{
    // Here so it can "see" std::formatter
    export auto make_table(Token_stream& tin) -> std::string
    {
        Table table{ tin };
        return std::format("{}\n{}", table, table.statement());
    }

    export auto eval_argument(Token_stream& tin)
    {
        Arg_evaluator ae{ tin };
        // Print atomic variables
        std::println("Base variables:\n{}", ae.as_base());
        // Print 
        std::println("{}", ae);
    }
}