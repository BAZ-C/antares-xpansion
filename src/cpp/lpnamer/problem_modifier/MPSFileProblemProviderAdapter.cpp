//
// Created by marechaljas on 09/11/22.
//

#include "MPSFileProblemProviderAdapter.h"
std::shared_ptr<Problem> MPSFileProblemProviderAdapter::provide_problem(
    const std::string& solver_name) const {
  return std::shared_ptr<Problem>();
}
MPSFileProblemProviderAdapter::MPSFileProblemProviderAdapter(
    const std::filesystem::path root, const std::string& problem_name)
    : lp_dir_(root), problem_name_(problem_name) {}
