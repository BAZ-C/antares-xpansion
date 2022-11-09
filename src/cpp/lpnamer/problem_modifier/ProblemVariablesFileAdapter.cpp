//
// Created by marechaljas on 09/11/22.
//

#include "ProblemVariablesFileAdapter.h"
ProblemVariables ProblemVariablesFileAdapter::Provide() {
  return ProblemVariables();
}
ProblemVariablesFileAdapter::ProblemVariablesFileAdapter(
    const ProblemData data, const std::vector<struct ActiveLink> vector_1,
    std::shared_ptr<ProblemGenerationLog::ProblemGenerationLogger> shared_ptr_1)
    : problem_data_(data), active_links_(vector_1), logger_(shared_ptr_1) {}
