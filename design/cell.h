#pragma once

#include <optional>
#include <unordered_set>

#include "common.h"
#include "formula.h"

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

    bool IsReferenced() const;

    class CellValueInterface;
    class EmptyCellValue;
    class TextCellValue;
    class FormulaCellValue;

    using CellValueIntefaceUPtr = std::unique_ptr<CellValueInterface>;

private:
    SheetInterface& sheet_;
    CellValueIntefaceUPtr value_{nullptr};

    mutable CellsStorage descending_cells_;
    mutable CellsStorage ascending_cells_;

    void InstantiateCellsIfNotExists(const CellValueIntefaceUPtr& current);
    bool HasCircularDependency(const Cell* reference, const CellValueIntefaceUPtr& current,
                               CellsStorage& visited) const;
    void RemoveOldConnections();
    void EstablishNewConnections(FormulaCellValue* formula_cell);
    void InvalidateReferencedCellsCache(CellsStorage& visited) const;


    const Cell* GetCell(Position position) const;
    void GetReferencedCellsImpl(std::vector<Position>& referenced, CellsStorage& visited) const;
    CellValueIntefaceUPtr TryCreateCell(std::string text, SheetInterface& sheet) const;
    FormulaCellValue* TryInterpretAsFormula(const CellValueIntefaceUPtr& cell) const;

};