#pragma once

#include <optional>
#include <unordered_set>

#include "common.h"
#include "formula.h"

// ------------------------------------ CELL VALUE INTERFACE ------------------------------

class CellValueInterface {
public:  
    using Value = CellInterface::Value;

    enum class Type { 
        Empty, 
        Text, 
        Formula 
    };

    explicit CellValueInterface(Type type) 
        : type_(type) {
    }
    virtual ~CellValueInterface() = default;


    virtual Value GetValue() const = 0;
    virtual Value GetRawValue() const = 0;
    virtual std::string GetText() const = 0;

    Type GetType() const {
        return type_;
    }

private:  
    Type type_;
};

// ------------------------------------ CELL ------------------------------

class FormulaCellValue;
class Cell : public CellInterface {
public:  
    using CellsStorage = std::unordered_set<const Cell*>;

    explicit Cell(SheetInterface& sheet);
    ~Cell() override = default;

    void Set(std::string text) override;
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    using CellValueIntefaceUPtr = std::unique_ptr<CellValueInterface>;

private:
    SheetInterface& sheet_;
    CellValueIntefaceUPtr value_{nullptr};

    mutable CellsStorage descending_cells_;
    mutable CellsStorage ascending_cells_;
    
    bool HasCircularDependency(const Cell* reference, const CellValueIntefaceUPtr& current,
                               CellsStorage& visited) const;
    void RemoveOldConnections();
    void EstablishNewConnections(FormulaCellValue* formula_cell);
    void InvalidateReferencedCellsCache(CellsStorage& visited) const;


    const Cell* GetCell(Position position) const;
    CellValueIntefaceUPtr TryCreateCell(std::string text, SheetInterface& sheet) const;
    FormulaCellValue* TryInterpretAsFormula(const CellValueIntefaceUPtr& cell) const;

};