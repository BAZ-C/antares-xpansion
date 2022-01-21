#pragma once

#include "SlaveCut.h"
#include "Worker.h"
#include "WorkerSlave.h"
#include "WorkerMaster.h"
#include "WorkerTrace.h"
#include "BendersOptions.h"
#include "core/ILogger.h"
#include "OutputWriter.h"

class BendersBase
{

public:
    virtual ~BendersBase() = default;
    BendersBase(BendersOptions const &options, Logger &logger, Writer writer);

    WorkerMasterPtr _master;
    SlavesMapPtr _map_slaves;

    Str2Int _problem_to_id;
    BendersData _data;
    BendersOptions _options;
    StrVector _slaves;

    AllCutStorage _all_cuts_storage;
    BendersTrace _trace;
    DynamicAggregateCuts _dynamic_aggregate_cuts;

    SimplexBasisStorage _basis;
    SlaveCutId _slave_cut_id;
    ActiveCutStorage _active_cuts;

    Logger _logger;
    Writer _writer;

    virtual void launch() = 0;

protected:
    // map linking each problem name to its variables and their ids
    CouplingMap _input;
    int _nbWeeks = 0;
    virtual void free() = 0;
    virtual void run() = 0;

public:
    void init_data();

    void print_csv();
    void print_csv_iteration(std::ostream &file, int ite);
    void print_master_and_cut(std::ostream &file, int ite, WorkerMasterDataPtr &trace, Point const &xopt);
    void print_master_csv(std::ostream &stream, const WorkerMasterDataPtr &trace, Point const &xopt) const;
    void print_cut_csv(std::ostream &stream, SlaveCutDataHandler const &handler, std::string const &name, int const islaves) const;
    void print_active_cut();

    void update_best_ub();
    void bound_simplex_iter(int simplexiter);
    bool stopping_criterion();
    void update_trace();
    void check_status(AllCutPackage const &all_package) const;

    void get_master_value();
    void get_slave_cut(SlaveCutPackage &slave_cut_package);

    void compute_cut(AllCutPackage const &all_package);
    void compute_slave_cut(const SlaveCutPackage slave_cut_package);
    void compute_cut_aggregate(AllCutPackage const &all_package);
    void compute_cut_val(const SlaveCutDataHandlerPtr &handler, const Point &x0, Point &s) const;
    void build_cut_full(AllCutPackage const &all_package);

    void dynamic_aggregation(AllCutPackage const &all_package);
    void dynamic_iteration(AllCutPackage const &all_package);
    void store_current_aggregate_cut(AllCutPackage const &all_package);
    void compute_dynamic_cut_at_iter(const SlaveCutDataHandlerPtr &handler, int const nite);
    void store_iter_aggregate_cut(AllCutPackage const &all_package);
    void gather_cut();

    void get_slave_basis(SimplexBasisPackage &simplex_basis_package);
    void sort_basis(AllBasisPackage const &all_basis_package);
    void update_active_cuts();

    LogData build_log_data_from_data() const;
    void post_run_actions() const;
    Output::IterationsData output_data() const;
    Output::Iteration iteration(const WorkerMasterDataPtr &masterDataPtr_l) const;
    Output::CandidatesVec candidates_data(const WorkerMasterDataPtr &masterDataPtr_l) const;
    Output::SolutionData solution() const;
    std::string status_from_criterion() const;
};
using pBendersBase = std::shared_ptr<BendersBase>;