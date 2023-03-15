#include "BendersByBatch.h"

#include <algorithm>
#include <functional>
#include <mutex>
#include <numeric>

#include "BatchCollection.h"
#include "RandomBatchShuffler.h"
#include "glog/logging.h"

BendersByBatch::BendersByBatch(BendersBaseOptions const &options, Logger logger,
                               Writer writer, mpi::environment &env,
                               mpi::communicator &world)
    : BendersMpi(options, logger, writer, env, world) {}

void BendersByBatch::InitializeProblems() {
  MatchProblemToId();

  BuildMasterProblem();
  const auto &coupling_map_size = coupling_map.size();
  std::vector<std::string> problem_names;
  for (const auto &[problem_name, _] : coupling_map) {
    problem_names.emplace_back(problem_name);
  }
  auto batch_size =
      Options().BATCH_SIZE == 0 ? coupling_map_size : Options().BATCH_SIZE;
  batch_collection_.SetLogger(_logger);
  batch_collection_.SetBatchSize(batch_size);
  batch_collection_.SetSubProblemNames(problem_names);
  batch_collection_.BuildBatches();
  BroadCast(batch_collection_, rank_0);
  // Dispatch subproblems to process
  auto problem_count = 0;
  for (const auto &batch : batch_collection_.BatchCollections()) {
    for (const auto &problem_name : batch.sub_problem_names) {
      // In case there are more subproblems than process
      if (auto process_to_feed = problem_count % WorldSize();
          process_to_feed ==
          Rank()) {  // Assign  [problemNumber % processCount] to processID

        const auto subProblemFilePath = GetSubproblemPath(problem_name);
        AddSubproblem({problem_name, coupling_map[problem_name]});
        AddSubproblemName(problem_name);
      }
      problem_count++;
    }
  }
}
void BendersByBatch::BroadcastSingleSubpbCostsUnderApprox() {
  DblVector single_subpb_costs_under_approx(_data.nsubproblem);
  if (Rank() == rank_0) {
    single_subpb_costs_under_approx = GetAlpha_i();
  }

  BroadCast(single_subpb_costs_under_approx.data(), _data.nsubproblem, rank_0);
  SetAlpha_i(single_subpb_costs_under_approx);
}
void BendersByBatch::Run() {
  PreRunInitialization();

  auto number_of_batch = batch_collection_.NumberOfBatch();
  random_batch_permutation_.resize(number_of_batch);
  unsigned batch_counter = 0;

  auto current_batch_id = 0;
  int number_of_sub_problem_resolved = 0;
  double cumulative_subproblems_timer_per_iter = 0;
  while (batch_counter < number_of_batch) {
    _data.it++;
    _data.ub = 0;
    SetSubproblemCost(0);
    auto remaining_epsilon = AbsoluteGap();

    if (Rank() == rank_0) {
      if (SwitchToIntegerMaster(_data.is_in_initial_relaxation)) {
        _logger->LogAtSwitchToInteger();
        ActivateIntegrityConstraints();
        ResetDataPostRelaxation();
      }

      _logger->log_at_initialization(_data.it +
                                     GetNumIterationsBeforeRestart());
      _logger->display_message("\tSolving master...");
      get_master_value();
      _logger->log_master_solving_duration(get_timer_master());

      ComputeXCut();
      _logger->log_iteration_candidates(bendersDataToLogData(_data));

      random_batch_permutation_ = RandomBatchShuffler(number_of_batch)
                                      .GetCyclicBatchOrder(current_batch_id);
    }
    BroadcastXCut();
    BroadcastSingleSubpbCostsUnderApprox();
    BroadCast(random_batch_permutation_.data(),
              random_batch_permutation_.size(), rank_0);
    batch_counter = 0;
    cumulative_subproblems_timer_per_iter = 0;

    while (batch_counter < number_of_batch) {
      current_batch_id = random_batch_permutation_[batch_counter];
      const auto &batch = batch_collection_.GetBatchFromId(current_batch_id);
      const auto &batch_sub_problems = batch.sub_problem_names;
      double batch_subproblems_costs_contribution_in_gap_per_proc = 0;
      double batch_subproblems_costs_contribution_in_gap = 0;
      Timer walltime;
      BuildCut(batch_sub_problems,
               &batch_subproblems_costs_contribution_in_gap_per_proc);
      Reduce(batch_subproblems_costs_contribution_in_gap_per_proc,
             batch_subproblems_costs_contribution_in_gap, std::plus<double>(),
             rank_0);
      Reduce(GetSubproblemsCpuTime(), cumulative_subproblems_timer_per_iter,
             std::plus<double>(), rank_0);

      if (Rank() == rank_0) {
        number_of_sub_problem_resolved += batch_sub_problems.size();
        remaining_epsilon -= batch_subproblems_costs_contribution_in_gap;
        SetSubproblemsWalltime(walltime.elapsed());
      }
      BroadCast(remaining_epsilon, rank_0);
      if (remaining_epsilon > 0) {
        batch_counter++;
      } else
        break;
    }
    BroadCast(batch_counter, rank_0);
    SetSubproblemsCumulativeCpuTime(cumulative_subproblems_timer_per_iter);
    _logger->number_of_sub_problem_resolved(number_of_sub_problem_resolved);
    _logger->LogSubproblemsSolvingCumulativeCpuTime(
        GetSubproblemsCumulativeCpuTime());
    _logger->LogSubproblemsSolvingWalltime(GetSubproblemsWalltime());
  }
  if (Rank() == rank_0) {
    compute_ub();
    update_best_ub();
    _logger->log_at_iteration_end(bendersDataToLogData(_data));
    UpdateTrace();
    SaveCurrentBendersData();
    _data.elapsed_time = GetBendersTime();

    CloseCsvFile();
    EndWritingInOutputFile();
    write_basis();
  }
}

/*!
 * \brief Build subproblem cut
 * Method to build subproblem cuts
 * and add them to the Master problem
 */
void BendersByBatch::BuildCut(
    const std::vector<std::string> &batch_sub_problems,
    double *batch_subproblems_costs_contribution_in_gap_per_proc) {
  SubProblemDataMap subproblem_data_map;
  Timer subproblems_timer_per_proc;
  GetSubproblemCut(subproblem_data_map, batch_sub_problems,
                   batch_subproblems_costs_contribution_in_gap_per_proc);

  SetSubproblemsCpuTime(subproblems_timer_per_proc.elapsed());
  std::vector<SubProblemDataMap> gathered_subproblem_map;
  Gather(subproblem_data_map, gathered_subproblem_map, rank_0);
  SetSubproblemsWalltime(subproblems_timer_per_proc.elapsed());
  for (const auto &subproblem_map : gathered_subproblem_map) {
    for (auto &&[_, subproblem_data] : subproblem_map) {
      SetSubproblemCost(GetSubproblemCost() + subproblem_data.subproblem_cost);
    }
  }
  for (const auto &subproblem_map : gathered_subproblem_map) {
    BuildCutFull(subproblem_map);
  }
}

/*!
 *  \brief Solve and store optimal variables of all Subproblem Problems
 *
 *  Method to solve and store optimal variables of all Subproblem Problems
 * after fixing trial values
 *
 *  \param subproblem_data_map : map storing for each subproblem its cut
 */
void BendersByBatch::GetSubproblemCut(
    SubProblemDataMap &subproblem_data_map,
    const std::vector<std::string> &batch_sub_problems,
    double *batch_subproblems_costs_contribution_in_gap_per_proc) const {
  *batch_subproblems_costs_contribution_in_gap_per_proc = 0;
  const auto &sub_pblm_map = GetSubProblemMap();

  for (const auto &[name, worker] : sub_pblm_map) {
    if (std::find(batch_sub_problems.cbegin(), batch_sub_problems.cend(),
                  name) != batch_sub_problems.cend()) {
      Timer subproblem_timer;
      SubProblemData subproblem_data;
      worker->fix_to(_data.x_cut);
      worker->solve(subproblem_data.lpstatus, Options().OUTPUTROOT,
                    Options().LAST_MASTER_MPS + MPS_SUFFIX);
      worker->get_value(subproblem_data.subproblem_cost);  // solution phi(x,s)
      worker->get_subgradient(
          subproblem_data.var_name_and_subgradient);  // dual pi_s
      *batch_subproblems_costs_contribution_in_gap_per_proc +=
          subproblem_data.subproblem_cost - GetAlpha_i()[ProblemToId(name)];
      worker->get_splex_num_of_ite_last(subproblem_data.simplex_iter);
      subproblem_data.subproblem_timer = subproblem_timer.elapsed();
      subproblem_data_map[name] = subproblem_data;
    }
  }
}