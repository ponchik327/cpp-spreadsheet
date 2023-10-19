#pragma once

#include <functional>
#include <vector>
#include <iostream>

#include "cell.h"
#include "common.h"

class Sheet : public SheetInterface {
public:
    using Storage = std::vector<std::vector<std::unique_ptr<CellInterface>>>;

    Sheet() = default;
    ~Sheet() override = default;

    void SetCell(Position position, std::string text) override;

    const CellInterface* GetCell(Position position) const override;
    CellInterface* GetCell(Position position) override;

    void ClearCell(Position position) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:  
    Storage data_;

    void ResizeStorage(Position position);

    template <class Getter>
    void Print(std::ostream& out, const Getter getter) const {
        auto [rows_count, columns_count] = GetPrintableSize();

        for (int row_id = 0; row_id < rows_count; ++row_id) {
            for (int col_id = 0; col_id < columns_count; ++col_id) {
                if (col_id != 0)
                    out << '\t';
                if (auto* cell = GetCell({row_id, col_id}))
                    getter(cell);
            }
            out << '\n';
        }
    }
};
