#include "cell.h"

#include <optional>
#include <string>

// ------------------------------------ EMPTY CELL VALUE ------------------------------

class EmptyCellValue : public CellValueInterface {
public:  
    EmptyCellValue() 
        : CellValueInterface(CellValueInterface::Type::Empty) {
    }

    Value GetValue() const override {
        return 0.;
    }

    Value GetRawValue() const override {
        return 0.;
    }

    std::string GetText() const override {
        using namespace std::string_literals;
        return ""s;
    }
};

// ------------------------------------ TEXT CELL VALUE ------------------------------

class TextCellValue : public CellValueInterface {
public:  
    explicit TextCellValue(std::string text)
        : CellValueInterface(CellValueInterface::Type::Text)
        , text_(std::move(text)) {
    }

    Value GetValue() const override {
        return (!text_.empty() && text_.front() == ESCAPE_SIGN) ? text_.substr(1) : text_;
    }

    Value GetRawValue() const override {
        return text_;
    }

    std::string GetText() const override {
        return text_;
    }

private:  
    std::string text_;
};

// ------------------------------------ FORMULA CELL VALUE ------------------------------

class FormulaCellValue : public CellValueInterface {
public: 
    struct FormulaEvaluationGetter {
        CellInterface::Value operator()(double value) {
            return value;
        }

        CellValueInterface::Value operator()(FormulaError error) {
            return error;
        }
    };
 
    FormulaCellValue(std::string text, SheetInterface& sheet)
        : CellValueInterface(CellValueInterface::Type::Formula)
        , formula_(ParseFormula(std::move(text)))
        , sheet_(sheet) {
    }

    Value GetValue() const override {
        if (cache_.has_value())
            return cache_.value();

        auto evaluation_result = formula_->Evaluate(sheet_);
        if (auto* value = std::get_if<double>(&evaluation_result))
            cache_ = *value;

        return std::visit(FormulaEvaluationGetter{}, evaluation_result);
    }

    Value GetRawValue() const override {
        return GetValue();
    }

    std::string GetText() const override {
        using namespace std::string_literals;
        return "="s + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const {
        return formula_->GetReferencedCells();
    }

    bool IsCacheValid() const {
        return cache_.has_value();
    }

    void InvalidateCache() {
        cache_.reset();
    }

private:  
    std::unique_ptr<FormulaInterface> formula_{nullptr};
    SheetInterface& sheet_;
    mutable std::optional<double> cache_;
};

// ------------------------------------ MAIN CELL CLASS ------------------------------

Cell::Cell(SheetInterface& sheet) : sheet_(sheet) {}

void Cell::Set(std::string text) {
    
    auto tmp = TryCreateCell(text, sheet_);

    if (CellsStorage visited{this}; HasCircularDependency(this, tmp, visited)) {
        throw CircularDependencyException("");
    }

    value_ = std::move(tmp);
    
    if (!descending_cells_.empty()) {
        RemoveOldConnections();
    }
    
    if (auto* formula_cell = TryInterpretAsFormula(value_)) {
        EstablishNewConnections(formula_cell);
    }
    
    if (!ascending_cells_.empty()) {
        CellsStorage visited{this};
        InvalidateReferencedCellsCache(visited);
    }
}

void Cell::Clear() {
    if (value_) {
        if (!descending_cells_.empty()) {
            RemoveOldConnections();
        }

        if (!ascending_cells_.empty()) {
            CellsStorage visited{this};
            InvalidateReferencedCellsCache(visited);
        }

        value_.reset();
    }
    value_ = std::make_unique<EmptyCellValue>();
}

Cell::Value Cell::GetValue() const {
    return value_->GetValue();
}

std::string Cell::GetText() const {
    return value_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return TryInterpretAsFormula(value_)->GetReferencedCells();
}

Cell::CellValueIntefaceUPtr Cell::TryCreateCell(std::string text, SheetInterface& sheet) const {
    using namespace std::string_literals;

    try {
        if (text.size() > 1 && text.front() == FORMULA_SIGN) {
            return std::make_unique<FormulaCellValue>(text.substr(1), sheet);
        } else if (!text.empty()) {
            return std::make_unique<TextCellValue>(std::move(text));
        } else {
            return std::make_unique<EmptyCellValue>();
        }
    } catch (std::exception&) {
        throw FormulaException("Error in Set() method call for text: "s + text);
    }
}

FormulaCellValue* Cell::TryInterpretAsFormula(const CellValueIntefaceUPtr& cell) const {
    return (cell && cell->GetType() == CellValueInterface::Type::Formula) ? dynamic_cast<FormulaCellValue*>(cell.get())
                                                                          : nullptr;
}

bool Cell::HasCircularDependency(const Cell* reference, const CellValueIntefaceUPtr& current,
                                 CellsStorage& visited) const {
    auto* formula_cell = TryInterpretAsFormula(current);

    if (!formula_cell) {
        return false;
    }

    for (const auto& position : formula_cell->GetReferencedCells()) {
        auto* cell = GetCell(position);

        if (cell == reference) {
            return true;
        }

        if (visited.count(cell) == 0) {
            visited.insert(cell);

            if (cell->HasCircularDependency(reference, cell->value_, visited)) {
                return true;
            }
        }
    }

    return false;
}

void Cell::InvalidateReferencedCellsCache(CellsStorage& visited) const {
    for (const auto* cell : ascending_cells_) {
        if (visited.count(cell) > 0) {
            return;
        }

        visited.insert(cell);

        if (auto* formula_cell = TryInterpretAsFormula(cell->value_); formula_cell && formula_cell->IsCacheValid()) {
            formula_cell->InvalidateCache();
            cell->InvalidateReferencedCellsCache(visited);
        }
    }
}

const Cell* Cell::GetCell(Position position) const {
    return dynamic_cast<const Cell*>(sheet_.GetCell(position));
}

void Cell::RemoveOldConnections() {
    for (auto* cell : descending_cells_) {
        cell->ascending_cells_.erase(this);
    }
    descending_cells_.clear();
}

void Cell::EstablishNewConnections(FormulaCellValue* formula_cell) {
    for (auto& position : formula_cell->GetReferencedCells()) {
        auto* cell = GetCell(position);

        descending_cells_.insert(cell);
        cell->ascending_cells_.insert(this);
    }
}
