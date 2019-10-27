#pragma once

#include "gui/objects/Bitmap.h"
#include "run/Node.h"
#include <wx/dialog.h>
#include <wx/grid.h>

NAMESPACE_SPH_BEGIN

class Config;

class BatchManager {
private:
    struct Col {
        SharedPtr<WorkerNode> node;
        std::string key;
    };

    Array<Col> cols;
    Array<std::string> rows;

    Bitmap<std::string> cells;

public:
    BatchManager() {
        this->resize(4, 3);
    }

    BatchManager clone() const {
        BatchManager copy;
        copy.cols = cols.clone();
        copy.rows = rows.clone();
        copy.cells = cells.clone();
        return copy;
    }

    Size getRunCount() const {
        return rows.size();
    }

    std::string getRunName(const Size rowIdx) const {
        if (rows[rowIdx].empty()) {
            return "Run " + std::to_string(rowIdx + 1);
        } else {
            return rows[rowIdx];
        }
    }

    Size getParamCount() const {
        return cols.size();
    }

    std::string getParamKey(const Size colIdx) const {
        return cols[colIdx].key;
    }

    SharedPtr<WorkerNode> getParamNode(const Size colIdx) const {
        return cols[colIdx].node;
    }

    std::string getCell(const Size colIdx, const Size rowIdx) const {
        return cells[Pixel(colIdx, rowIdx)];
    }

    void setRunName(const Size rowIdx, const std::string& name) {
        rows[rowIdx] = name;
    }

    void setParam(const Size colIdx, const SharedPtr<WorkerNode>& node, const std::string& key) {
        cols[colIdx].key = key;
        cols[colIdx].node = node;
    }

    void setCell(const Size colIdx, const Size rowIdx, const std::string& value) {
        cells[Pixel(colIdx, rowIdx)] = value;
    }

    void resize(const Size rowCnt, const Size colCnt) {
        cols.resize(colCnt);
        rows.resize(rowCnt);
        Bitmap<std::string> oldCells = cells.clone();
        cells.resize(Pixel(colCnt, rowCnt), "");

        const Size minRowCnt = min(cells.size().y, oldCells.size().y);
        const Size minColCnt = min(cells.size().x, oldCells.size().x);
        for (Size j = 0; j < minRowCnt; ++j) {
            for (Size i = 0; i < minColCnt; ++i) {
                cells[Pixel(i, j)] = oldCells[Pixel(i, j)];
            }
        }
    }

    void duplicateRun(const Size rowIdx) {
        const std::string runName = rows[rowIdx];
        rows.insert(rowIdx, runName);

        Bitmap<std::string> newCells(Pixel(cols.size(), rows.size()));
        for (Size j = 0; j < rows.size(); ++j) {
            for (Size i = 0; i < cols.size(); ++i) {
                const Size j0 = j <= rowIdx ? j : j - 1;
                newCells[Pixel(i, j)] = cells[Pixel(i, j0)];
            }
        }
        cells = std::move(newCells);
    }

    void deleteRun(const Size rowIdx) {
        rows.remove(rowIdx);
        Bitmap<std::string> newCells(Pixel(cols.size(), rows.size()));
        for (Size j = 0; j < rows.size(); ++j) {
            for (Size i = 0; i < cols.size(); ++i) {
                const Size j0 = j <= rowIdx ? j : j + 1;
                newCells[Pixel(i, j)] = cells[Pixel(i, j0)];
            }
        }
        cells = std::move(newCells);
    }

    /// \brief Modifies the settings of the given node hierarchy.
    ///
    /// Nodes are modified according to parameters of given run. Other parameters or nodes not specified in
    /// the manager are unchanged.
    void modifyHierarchy(const Size runIdx, WorkerNode& node);

    void load(Config& config, ArrayView<const SharedPtr<WorkerNode>> nodes);
    void save(Config& config);

private:
    void modifyNode(WorkerNode& node, const Size runIdx, const Size paramIdx);
};

class BatchDialog : public wxDialog {
private:
    BatchManager manager;
    Array<SharedPtr<WorkerNode>> nodes;

    wxGrid* grid;

public:
    BatchDialog(wxWindow* parent, const BatchManager& manager, Array<SharedPtr<WorkerNode>>&& nodes);

    ~BatchDialog();

    BatchManager& getBatch() {
        return manager;
    }

private:
    /// Loads grid data from the manager.
    void update();
};

NAMESPACE_SPH_END
