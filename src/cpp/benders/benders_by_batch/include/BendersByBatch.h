#ifndef SRC_CPP_BENDERS_BENDERS_BY_BATCH_INCLUDE_BENDERSBYBATCH_H_
#define SRC_CPP_BENDERS_BENDERS_BY_BATCH_INCLUDE_BENDERSBYBATCH_H_
#include "BatchCollection.h"
#include "BendersSequential.h"
class BendersByBatch : public BendersSequential {
  std::vector<unsigned> random_batch_permutation_;

 public:
  BendersByBatch(BendersBaseOptions const &options, Logger logger,
                 Writer writer);
  ~BendersByBatch() override = default;
  void run() override;
  void build_cut(const std::vector<std::string> &batch_sub_problems,
                 double *sum);
  std::string BendersName() const { return "By Batch"; }

 private:
  void getSubproblemCut(SubProblemDataMap &subproblem_cut_package,
                        const std::vector<std::string> &batch_sub_problems,
                        double *sum) const;
};

#endif  // SRC_CPP_BENDERS_BENDERS_BY_BATCH_INCLUDE_BENDERSBYBATCH_H_