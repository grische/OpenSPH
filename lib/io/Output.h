#pragma once

/// \file Output.h
/// \brief Saving and loading particle data
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2018

#include "io/Path.h"
#include "objects/utility/EnumMap.h"
#include "objects/wrappers/Expected.h"
#include "objects/wrappers/Outcome.h"
#include "physics/Constants.h"
#include "quantities/Storage.h"

NAMESPACE_SPH_BEGIN

class Deserializer;

/// \brief Helper file generating file names for output files.
class OutputFile {
private:
    mutable Size dumpNum = 0;
    Path pathMask;

public:
    OutputFile() = default;

    /// \brief Creates a new filename generator using provided path mask.
    ///
    /// If the input mask is a regular path, all calls to \ref getNextPath will simply return the input path.
    /// Note that output generated using such \ref OutputFile will override the previous one. To avoid this,
    /// the \ref pathMask can contain following wildcards:
    /// - '%d' - replaced by the dump number, starting from arbitrary index, incremented every dump.
    /// - '%t' - replaced by current simulation time (with _ instead of decimal separator).
    ///
    /// \param pathMask Path possibly containing wildcards, used to determine the actual path for the dump.
    /// \param firstDumpIdx Index of the first dump. Can be non-zero if the simulation is resumed from
    ///                     previously generated snapshot, in order to continue in the sequence.
    OutputFile(const Path& pathMask, const Size firstDumpIdx = 0);

    /// \brief Returns path to the next output file.
    ///
    /// This increments the internal counter. Function is const so that it can be used in const functions. No
    /// file is created by this.
    Path getNextPath(const Statistics& stats) const;

    /// \brief Returns true if the file mask contains (at least one) wildcard.
    ///
    /// If not, \ref getNextPath will always return the same path.
    bool hasWildcard() const;

    /// \brief Returns the file mask as given in constructor.
    Path getMask() const;
};

/// \brief Interface for saving quantities of SPH particles to a file.
///
/// Saves all values in the storage or selected few quantities, depending on implementation. It is also
/// implementation-defined whether the saved representation of storage is lossless, i.e. whether the
/// storage can be loaded without a loss in precision.
class IOutput : public Polymorphic {
protected:
    OutputFile paths;

public:
    /// \brief Constructs output given the file name of the output.
    explicit IOutput(const OutputFile& fileMask);

    /// \brief Saves data from particle storage into the file.
    ///
    /// Returns the filename of the dump, generated from file mask given in constructor.
    virtual Path dump(const Storage& storage, const Statistics& stats) = 0;
};

/// \brief Interface for loading quantities of SPH particles from a file.
///
/// This can be used to continue simulation from a previously saved snapshot.
class IInput : public Polymorphic {
public:
    /// \brief Loads data from the file into the storage.
    ///
    /// This will remove any data previously stored in storage. Function can also load additional run
    /// statistics, such as time of the snapshot, and save it to provided stats value.
    virtual Outcome load(const Path& path, Storage& storage, Statistics& stats) = 0;
};


/// \brief List of quantities we can write to text output.
///
/// Unlike QuantityId, this includes derivatives of quantities (for example velocities) and 'virtual'
/// quantities like smoothing lengths. It can be also stored as flags.
enum class OutputQuantityFlag {
    /// Positions of particles, always a vector quantity.
    POSITION = 1 << 0,

    /// Current velocities of particles, always a vector quantity.
    VELOCITY = 1 << 1,

    /// Smoothing lenghts of particles.
    SMOOTHING_LENGTH = 1 << 2,

    /// Particle masses, always a scalar quantity.
    MASS = 1 << 3,

    /// Pressure, reduced by yielding and fracture model (multiplied by 1-damage); always a scalar quantity.
    PRESSURE = 1 << 4,

    /// Density, always a scalar quantity.
    DENSITY = 1 << 5,

    /// Specific internal energy, always a scalar quantity.
    ENERGY = 1 << 6,

    /// Deviatoric stress tensor, always a traceless tensor stored in components xx, yy, xy, xz, yz.
    DEVIATORIC_STRESS = 1 << 7,

    /// Damage, reducing the pressure and deviatoric stress.
    DAMAGE = 1 << 8,

    /// Symmetric tensor correcting kernel gradient for linear consistency.
    STRAIN_RATE_CORRECTION_TENSOR = 1 << 9,

    /// ID of material, indexed from 0 to (#bodies - 1).
    MATERIAL_ID = 1 << 10,

    /// Index of particle
    INDEX = 1 << 11,
};

class ITextColumn;

/// \brief Output saving data to text (human readable) file.
class TextOutput : public IOutput {
public:
    enum class Options {
        /// Dumps all quantity values from the storage; this overrides the list of selected particles.
        DUMP_ALL = 1 << 0,

        /// Writes all numbers in scientific format
        SCIENTIFIC = 1 << 1,
    };

private:
    std::string runName;

    /// Flags of the output
    Flags<Options> options;

    /// Value columns saved into the file
    Array<AutoPtr<ITextColumn>> columns;

public:
    /// \brief Creates a new text file output.
    ///
    /// \param fileMask Path where the file will be stored. Can contain wildcards, see \ref IOutput
    ///                 constructor documentation.
    /// \param runName Arbitrary label of the simulation, saved in the file header/
    /// \param quantities List of quantities to store. Note that arbitrary quantities can be later added using
    ///                   \ref addColumn.
    /// \param options Parameters of the file, see \ref Options enum.
    TextOutput(const OutputFile& fileMask,
        const std::string& runName,
        Flags<OutputQuantityFlag> quantities,
        Flags<Options> options = EMPTY_FLAGS);

    ~TextOutput();

    /// \brief Adds a new column to be saved into the file.
    ///
    /// Unless an option is specified in constructor, the file has no columns. All quantities must be
    /// explicitly added using this function. The column is added to the right end of the text file.
    /// \param column New column to save; see \ref ITextColumn and derived classes
    /// \return Reference to itself, allowing to queue calls
    TextOutput& addColumn(AutoPtr<ITextColumn>&& column);

    virtual Path dump(const Storage& storage, const Statistics& stats) override;
};

/// \brief Input for the text file, generated by \ref TextOutput or conforming to the same format.
///
/// Note that this loader cannot be used to resume a simulation (at least not directly), as the text file does
/// not contains all the necessary data.
class TextInput : public IInput {
private:
    Array<AutoPtr<ITextColumn>> columns;

public:
    explicit TextInput(Flags<OutputQuantityFlag> quantities);

    TextInput& addColumn(AutoPtr<ITextColumn>&& column);

    virtual Outcome load(const Path& path, Storage& storage, Statistics& stats) override;
};

/// \brief Extension of text output that runs given gnuplot script on dumped text data.
class GnuplotOutput : public TextOutput {
private:
    std::string scriptPath;

public:
    GnuplotOutput(const OutputFile& fileMask,
        const std::string& runName,
        const std::string& scriptPath,
        const Flags<OutputQuantityFlag> quantities,
        const Flags<Options> flags)
        : TextOutput(fileMask, runName, quantities, flags)
        , scriptPath(scriptPath) {}

    virtual Path dump(const Storage& storage, const Statistics& stats) override;
};


/// \brief Type of simulation, stored as metadata in the binary file.
///
/// This value is used when reading the data in ssf player (to select proper visualization) and when resuming
/// a simulation (to continue in correct phase). When a new type of simulation is added, either modify this
/// enum or create and additional enum (with no conflicts with this one) and cast it.
enum class RunTypeEnum {
    /// Main SPH simulation
    SPH,

    /// Confergence phase using SPH solver, used to obtain stable initial conditions
    STABILIZATION,

    /// Pre-stabilization phase, using N-body solver to get a realistic rubble-pile body
    RUBBLE_PILE,

    /// Main N-body simulation, using initial conditions either from SPH (handoff) or manually specified
    NBODY,
};

static RegisterEnum<RunTypeEnum> sRunType({
    { RunTypeEnum::SPH, "sph", "Main SPH simulation" },
    { RunTypeEnum::STABILIZATION, "stabilization", "Convergence SPH phase" },
    { RunTypeEnum::RUBBLE_PILE, "rubble-pile", "Simulation creating rubble-pile target" },
    { RunTypeEnum::NBODY, "nbody", "Main N-body simulation" },
});

enum class BinaryIoVersion : int64_t {
    FIRST = 0,
    V2018_04_07 = 20180407, ///< serializing (incorrectly!!) enum type
    V2018_10_24 = 20181024, ///< reverted enum (storing zero instead of hash), storing type of simulation
    LATEST = V2018_10_24,
};

/// \brief Output saving data to binary data without loss of precision.
///
/// \subsection Format specification
/// File format can store values of type int64 (called Size hereafter), double (called Float hereafter),
/// strings encoded as chars with terminating zero and padding (zero bytes). There are no other types
/// currently used. All other types (Vectors, Tensors, enums, ...) are converted to Floats or Sizes.
/// The file is divided as follows: header, quantity info, n x [material info, quantity data] where n is the
/// number of materials.
///
/// Header is always 256 bytes, storing the following:
///  - first 4 bytes are file format identifier "SPH" (including terminating zero)
///  - bytes 5-12:  time [Float] of the dump (in simulation time)
///  - bytes 13-20: total particle count [Size] in the storage (called particleCnt)
///  - bytes 21-28: number of quantities [Size] in the storage (called quantityCnt)
///  - bytes 29-36: number of materials [Size] (called materialCnt)
///  - bytes 37-44: timestep when the snapshot has been created
///  - bytes 45-52: version of the file format, see enum \ref BinaryOutput::Version.
///  - bytes 53-68: string identifying a run type, see enum \ref RunTypeEnum.
///  - bytes 69-256: padding (possibly will be used in the future)
///
/// Quantity info (summary) follows the header. It consist of quantityCnt triples of values
///  - ID of quantity [Size casted from QuantityId]
///  - Order of quantity [Size casted from OrderEnum]
///  - Value type of quantity [Size casted from ValueEum]
///
/// After than there follows material info and quantity data for every stored material. The material info
/// consist of materialCnt blocks with content:
///  - block identifier "MAT" (4 bytes with terminating zero)
///  - number of material [Size] in increasing order (from 0 up to materialCnt-1)
///  - number of BodySettings entries [Size] of the material (called paramCnt)
///  - paramCnt times:
///     . parameter ID [Size casted from BodySettingsId]
///     . type index of the value type [Size], corresponding to the type in settings variant
///     . entry value itself. The size of the value depends on its type, identified by the index read
///       previously. There is currently no upper limit for the entry size; although the largest one used is a
///       Tensor with 72 bytes, this size can be increased in the future.
///  - quantityCnt times:
///     . Quantity ID of n-th quantity [Size casted from QuantityId]
///     . Range of n-th quantity [2x Float]
///     . minimal value of n-th quantity [Float] for timestepping
/// \attention The whole material block can be missing as Storage can exist without a material. This is
/// indicated by materialCnt == 0, but also by identifier "NOMAT" (6 bytes including terminating zero)
/// in place of first "MAT" identifier if the file had material block.
///
/// The next two numbers [2x Float] are the first and one-past-last index of the particles corresponding to
/// this material. The first index for the first material is always zero, the second index for the last
/// material
/// is the particleCnt. The first index is always equal to the second index of the previous material.
/// The quantity data are stored immediately afterwards. It consist of quantityCnt blocks. Each block has 1-3
/// sub-blocks, depending on the order of n-th. Note that the orders and value types of quantities are stored
/// in the quantity info at the beginning of the file and are not repeated here.
/// Each sub-block contains directly serialized value and derivatives of n-th quantity, i.e.
///  - particleCnt times value, representing quantity value of i-th particle
///  - if quantity is of first or second order, then follows particleCnt times value, representing quantity
///    first derivative of i-th particle
///  - if quantity is of second order, then follows particleCnt times values, representing quantity second
///  derivative of i-th particle.
/// The size of a single value depends on the value type, i.e. scalar quantities are stored as Floats, Vector
/// quantities are stored as 4x Float (components X, Y, Z and H), etc.
/// The file ends immediately after the last quantity is saved.
///
/// \todo Possible todos & fixes:
///  - arbitrary precision: store doubles as floats or halfs and size_t as uint32 or uint16, based on data in
///    header
///  - deserialized materials are created using Factory::getMaterial from loaded settings. This won't be
///    correct if the material was created differently, i.e. if the material doesn't match the information in
///    the settings it holds. This should be enforced somehow.
class BinaryOutput : public IOutput {
    friend class BinaryInput;

private:
    static constexpr Size PADDING_SIZE = 188;

    RunTypeEnum runTypeId;

public:
    explicit BinaryOutput(const OutputFile& fileMask, const RunTypeEnum runTypeId = RunTypeEnum::SPH);

    virtual Path dump(const Storage& storage, const Statistics& stats) override;
};

/// \brief Input for the binary file, generated by \ref BinaryOutput.
///
/// Storage loaded by this class can be used to continue a simulation.
class BinaryInput : public IInput {
public:
    virtual Outcome load(const Path& path, Storage& storage, Statistics& stats) override;

    struct Info {
        /// Number of quantities in the file
        Size quantityCnt;

        /// Number of particles in the file
        Size particleCnt;

        /// Number of materials in the file
        Size materialCnt;

        /// Information about stored quantities
        struct QuantityInfo {

            /// ID of the quantity
            QuantityId id;

            /// Order (=number or derivatives)
            OrderEnum order;

            /// Value type of the quantity
            ValueEnum value;
        };

        Array<QuantityInfo> quantityInfo;

        /// Run time of the snapshot
        Float runTime;

        /// Current timestep of the run
        Float timeStep;

        /// Type of the simulation.
        ///
        /// Not present in older versions of the format.
        Optional<RunTypeEnum> runType;

        /// Format version of the file
        BinaryIoVersion version;
    };

    /// \brief Opens the file and reads header info without reading the rest of the file.
    Expected<Info> getInfo(const Path& path) const;
};

struct PkdgravParams {

    /// Conversion formula from SPH particles to hard spheres in pkdgrav
    enum class Radius {
        /// Compute sphere radius using R = h/3 formula
        FROM_SMOOTHING_LENGTH,

        /// Compute sphere radius so that its mass is equivalent to the mass of SPH particle
        FROM_DENSITY
    };

    Radius radius = Radius::FROM_DENSITY;

    /// Conversion factors for pkdgrav
    struct Conversion {

        Float mass = Constants::M_sun;

        Float distance = Constants::au;

        Float velocity = Constants::au * 2._f * PI / (365.25_f * 86400._f);

    } conversion;

    /// Adds additional rotation of all particles around the origin. This can be used to convert a
    /// non-intertial coordinate system used in the code to intertial system.
    Vector omega = Vector(0._f);

    /// Threshold of internal energy; particles with higher energy are considered a vapor and
    /// we discard them in the output
    Float vaporThreshold = 1.e6_f;

    /// Color indices of individual bodies in the simulations. The size of the array must be equal (or bigger)
    /// than the number of bodies in the storage.
    Array<Size> colors{ 3, 13 };
};


/// \brief Dumps data into a file that can be used as an input for pkdgrav code by Richardson et al.
///
/// SPH particles are converted into hard spheres, moving with the velocity of the particle and no angular
/// velocity. Quantities are converted into G=1 units.
/// See \cite Richardson_etal_2000.
class PkdgravOutput : public IOutput {
private:
    /// Parameters of the SPH->pkdgrav conversion
    PkdgravParams params;

public:
    PkdgravOutput(const OutputFile& fileMask, PkdgravParams&& params);

    virtual Path dump(const Storage& storage, const Statistics& stats) override;

private:
    INLINE Float getRadius(const Float h, const Float m, const Float rho) const {
        switch (params.radius) {
        case PkdgravParams::Radius::FROM_DENSITY:
            // 4/3 pi r^3 rho = m
            return cbrt(3._f * m / (4._f * PI * rho));
        case PkdgravParams::Radius::FROM_SMOOTHING_LENGTH:
            return h / 3._f;
        default:
            NOT_IMPLEMENTED;
        }
    }
};

/// \brief Input for pkdgrav output files (ss.xxxxx.bt).
///
/// Mainly indended for analysis of the result, creating histograms, etc. Cannot be used to continue a
/// simulation.
class PkdgravInput : public IInput {
public:
    virtual Outcome load(const Path& path, Storage& storage, Statistics& stats) override;
};

class NullOutput : public IOutput {
public:
    NullOutput()
        : IOutput(Path("file")) {} // we don't dump anything, but IOutput asserts non-empty path

    virtual Path dump(const Storage& UNUSED(storage), const Statistics& UNUSED(stats)) override {
        return Path();
    }
};

NAMESPACE_SPH_END
