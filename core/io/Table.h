#pragma once

/// \file Table.h
/// \brief Helper container allowing to store strings in cells and print them into table
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2021

#include "objects/containers/Array.h"

NAMESPACE_SPH_BEGIN

class Table {
private:
    using Row = Array<String>;

    Array<Row> rows;

    struct {
        Size colSep;
        Size minColWidth;
    } params;

public:
    /// \brief Creates an empty table
    ///
    /// \param colSep Minimal number of characters between columns.
    /// \param minColWidth Minimal width of a column; the column gets strethed if needed.
    Table(const Size colSep = 1, const Size minColWidth = 5)
        : params{ colSep, minColWidth } {}

    /// \brief Sets the text in given cell.
    ///
    /// If the cell already exists, the previous text is replaced, otherwise a new cell is created, extending
    /// the number of cells and rows if needed.
    void setCell(const Size colIdx, const Size rowIdx, String text) {
        // extend rows
        for (Size i = rows.size(); i <= rowIdx; ++i) {
            rows.emplaceBack(this->columnCnt());
        }
        // extend columns
        if (colIdx >= this->columnCnt()) {
            for (Row& row : rows) {
                row.resize(colIdx + 1);
            }
        }
        rows[rowIdx][colIdx] = std::move(text);
    }

    Size rowCnt() const {
        return rows.size();
    }

    Size columnCnt() const {
        return rows.empty() ? 0 : rows[0].size();
    }

    /// \brief Creates the text representation of the table.
    String toString() const {
        if (rows.empty()) {
            return String();
        }
        Array<Size> colWidths(this->columnCnt());
        colWidths.fill(0);
        for (const Row& row : rows) {
            for (Size colIdx = 0; colIdx < row.size(); ++colIdx) {
                colWidths[colIdx] = max(colWidths[colIdx], Size(row[colIdx].size()));
            }
        }
        for (Size colIdx = 0; colIdx < rows[0].size(); ++colIdx) {
            colWidths[colIdx] = max(colWidths[colIdx] + params.colSep, params.minColWidth);
        }
        std::stringstream ss;
        for (const Row& row : rows) {
            for (Size colIdx = 0; colIdx < row.size(); ++colIdx) {
                ss << std::setw(colWidths[colIdx]) << row[colIdx];
            }
            ss << std::endl;
        }
        return String::fromAscii(ss.str().c_str());
    }
};

NAMESPACE_SPH_END
