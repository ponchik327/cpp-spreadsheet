#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <regex>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

struct CellValueGetter {
    double operator()(const std::string& text) {
        try {
            static const std::regex kDoubleValuePattern("^(-?)(0|([1-9][0-9]*))(.[0-9]+)?$");

            std::smatch match;
            if (std::regex_match(text.cbegin(), text.cend(), match, kDoubleValuePattern)) {
                return std::stod(text);
            }

            throw std::runtime_error("");
        } catch (...) {
            throw FormulaError(FormulaError::Category::Value);
        }
    }

    double operator()(double value) {
        return value;
    }

    double operator()(const FormulaError& error) {
        throw error;
    }
};
namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(const std::string& expression)
        : ast_(ParseFormulaAST(expression)), 
        referenced_cells_(ast_.GetCells().begin(), ast_.GetCells().end()) {

        auto last = std::unique(referenced_cells_.begin(), referenced_cells_.end());
        referenced_cells_.erase(last, referenced_cells_.end());
        std::sort(referenced_cells_.begin(), referenced_cells_.end());
    } 
        
    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            auto get_value = [&sheet](const Position& position) {
                if (auto* cell = sheet.GetCell(position))
                    return std::visit(CellValueGetter{}, cell->GetValue());
                return 0.;
            };

            return ast_.Execute(get_value);
        } catch (const FormulaError& error_code) {
            return error_code;
        }
    }

    std::string GetExpression() const override {
        std::stringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        return referenced_cells_;
    }

private:
    FormulaAST ast_;
    std::vector<Position> referenced_cells_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(expression);
    } catch (std::exception& e) {
        throw FormulaException(e.what());
    }
}