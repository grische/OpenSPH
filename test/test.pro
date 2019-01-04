TEMPLATE = app
CONFIG += c++14 thread silent
CONFIG -= app_bundle
CONFIG -= qt

LIBS += ../lib/liblib.a

include(../lib/inc.pro)

INCLUDEPATH += ../lib ../../external/Catch/include
DEPENDPATH += . ../lib
LIBS += ../lib/liblib.a
PRE_TARGETDEPS += ../lib/liblib.a

SOURCES += \
    ../lib/gravity/test/BarnesHut.cpp \
    ../lib/gravity/test/BruteForceGravity.cpp \
    ../lib/gravity/test/Moments.cpp \
    ../lib/gravity/test/NBodySolver.cpp \
    ../lib/io/test/FileManager.cpp \
    ../lib/io/test/FileSystem.cpp \
    ../lib/io/test/Logger.cpp \
    ../lib/io/test/OutOfCore.cpp \
    ../lib/io/test/Output.cpp \
    ../lib/io/test/Path.cpp \
    ../lib/io/test/Serializer.cpp \
    ../lib/math/rng/test/Rng.cpp \
    ../lib/math/rng/test/VectorRng.cpp \
    ../lib/math/test/AffineMatrix.cpp \
    ../lib/math/test/Math.cpp \
    ../lib/math/test/Means.cpp \
    ../lib/math/test/Morton.cpp \
    ../lib/math/test/Quat.cpp \
    ../lib/math/test/SparseMatrix.cpp \
    ../lib/objects/containers/test/Array.cpp \
    ../lib/objects/containers/test/ArrayRef.cpp \
    ../lib/objects/containers/test/BufferedArray.cpp \
    ../lib/objects/containers/test/FlatMap.cpp \
    ../lib/objects/containers/test/FlatSet.cpp \
    ../lib/objects/containers/test/Grid.cpp \
    ../lib/objects/containers/test/List.cpp \
    ../lib/objects/containers/test/StaticArray.cpp \
    ../lib/objects/containers/test/String.cpp \
    ../lib/objects/containers/test/Tuple.cpp \
    ../lib/objects/finders/test/BruteForceFinder.cpp \
    ../lib/objects/finders/test/Finders.cpp \
    ../lib/objects/finders/test/Order.cpp \
    ../lib/objects/finders/test/Bvh.cpp \
    ../lib/objects/geometry/test/AntisymmetricTensor.cpp \
    ../lib/objects/geometry/test/Box.cpp \
    ../lib/objects/geometry/test/Domain.cpp \
    ../lib/objects/geometry/test/Indices.cpp \
    ../lib/objects/geometry/test/Multipole.cpp \
    ../lib/objects/geometry/test/Sphere.cpp \
    ../lib/objects/geometry/test/SymmetricTensor.cpp \
    ../lib/objects/geometry/test/Tensor.cpp \
    ../lib/objects/geometry/test/TracelessTensor.cpp \
    ../lib/objects/geometry/test/Vector.cpp \
    ../lib/objects/utility/test/ArrayUtils.cpp \
    ../lib/objects/utility/test/Dynamic.cpp \
    ../lib/objects/utility/test/Iterators.cpp \
    ../lib/objects/utility/test/OperatorTemplate.cpp \
    ../lib/objects/utility/test/StringUtils.cpp \
    ../lib/objects/wrappers/test/AlignedStorage.cpp \
    ../lib/objects/wrappers/test/Any.cpp \
    ../lib/objects/wrappers/test/AutoPtr.cpp \
    ../lib/objects/wrappers/test/ClonePtr.cpp \
    ../lib/objects/wrappers/test/Expected.cpp \
    ../lib/objects/wrappers/test/Finally.cpp \
    ../lib/objects/wrappers/test/Flags.cpp \
    ../lib/objects/wrappers/test/Function.cpp \
    ../lib/objects/wrappers/test/Interval.cpp \
    ../lib/objects/wrappers/test/LockingPtr.cpp \
    ../lib/objects/wrappers/test/Lut.cpp \
    ../lib/objects/wrappers/test/Optional.cpp \
    ../lib/objects/wrappers/test/Outcome.cpp \
    ../lib/objects/wrappers/test/SharedPtr.cpp \
    ../lib/objects/wrappers/test/Variant.cpp \
    ../lib/objects/wrappers/test/VectorizedArray.cpp \
    ../lib/physics/test/Damage.cpp \
    ../lib/physics/test/Eos.cpp \
    ../lib/physics/test/Integrals.cpp \
    ../lib/physics/test/TimeFormat.cpp \
    ../lib/physics/test/Yielding.cpp \
    ../lib/post/test/Analysis.cpp \
    ../lib/post/test/MarchingCubes.cpp \
    ../lib/post/test/MeshFile.cpp \
    ../lib/post/test/Point.cpp \
    ../lib/post/test/StatisticTests.cpp \
    ../lib/quantities/test/IMaterial.cpp \
    ../lib/quantities/test/Iterate.cpp \
    ../lib/quantities/test/Particle.cpp \
    ../lib/quantities/test/Quantity.cpp \
    ../lib/quantities/test/QuantityHelpers.cpp \
    ../lib/quantities/test/Storage.cpp \
    ../lib/sph/boundary/test/Boundary.cpp \
    ../lib/sph/equations/av/test/AV.cpp \
    ../lib/sph/equations/av/test/Balsara.cpp \
    ../lib/sph/equations/av/test/MorrisMonaghan.cpp \
    ../lib/sph/equations/av/test/Stress.cpp \
    ../lib/sph/equations/test/Accumulated.cpp \
    ../lib/sph/equations/test/Derivative.cpp \
    ../lib/sph/equations/test/EquationTerm.cpp \
    ../lib/sph/equations/test/Friction.cpp \
    ../lib/sph/equations/test/Heat.cpp \
    ../lib/sph/equations/test/Potentials.cpp \
    ../lib/sph/equations/test/Rotation.cpp \
    ../lib/sph/equations/test/XSph.cpp \
    ../lib/sph/initial/test/Distribution.cpp \
    ../lib/sph/initial/test/Initial.cpp \
    ../lib/sph/kernel/test/GravityKernel.cpp \
    ../lib/sph/kernel/test/Interpolation.cpp \
    ../lib/sph/kernel/test/Kernel.cpp \
    ../lib/sph/solvers/test/GravitySolver.cpp \
    ../lib/sph/solvers/test/Impact.cpp \
    ../lib/sph/solvers/test/Solvers.cpp \
    ../lib/sph/solvers/test/StandardSets.cpp \
    ../lib/sph/solvers/test/StaticSolver.cpp \
    ../lib/sph/test/Diagnostics.cpp \
    ../lib/system/test/ArgsParser.cpp \
    ../lib/system/test/ArrayStats.cpp \
    ../lib/system/test/Process.cpp \
    ../lib/system/test/Profiler.cpp \
    ../lib/system/test/Settings.cpp \
    ../lib/system/test/Statistics.cpp \
    ../lib/system/test/Timer.cpp \
    ../lib/thread/test/AtomicFloat.cpp \
    ../lib/thread/test/CheckFunction.cpp \
    ../lib/thread/test/Pool.cpp \
    ../lib/timestepping/test/TimeStepCriterion.cpp \
    ../lib/timestepping/test/TimeStepping.cpp \
    main.cpp \
    utils/Approx.cpp \
    utils/SequenceTest.cpp \
    utils/Utils.cpp \
    ../lib/sph/equations/test/Fluids.cpp \
    ../lib/physics/test/Units.cpp \
    ../lib/math/test/Functional.cpp \
    ../lib/physics/test/Functions.cpp \
    ../lib/sph/solvers/test/DensityIndependentSolver.cpp \
    ../lib/thread/test/Scheduler.cpp \
    ../lib/sph/handoff/test/Handoff.cpp \
    ../lib/sph/solvers/test/EnergyConservingSolver.cpp \
    ../lib/gravity/test/CachedGravity.cpp \
    ../lib/run/test/IRun.cpp \
    ../lib/run/test/Collision.cpp \
    ../lib/system/test/Crashpad.cpp

HEADERS += \
    utils/Utils.h \
    utils/RecordType.h \
    utils/SequenceTest.h \
    utils/Config.h
