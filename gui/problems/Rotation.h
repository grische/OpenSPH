#pragma once

/// \file Rotation.h
/// \brief Asteroid rotation and rotational fission
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2017

#include "gui/windows/MainWindow.h"
#include "io/LogFile.h"
#include "quantities/Storage.h"
#include "run/Run.h"

NAMESPACE_SPH_BEGIN

class EquationHolder;

class AsteroidRotation : public Abstract::Run {
private:
    Controller* model;
    Float period;

public:
    /// \param period Rotational period of asteroid in hours
    AsteroidRotation(Controller* model, const Float period);

    virtual void setUp() override;

protected:
    virtual void tearDown() override;

private:
    void setInitialStressTensor(Storage& smaller, EquationHolder& equations);
};

NAMESPACE_SPH_END
