//
// Created by marechaljas on 09/11/22.
//

#pragma once

#include "IProblemVariablesProviderPort.h"
#include "LinkProblemsGenerator.h"
class ProblemVariablesFileAdapter : public IProblemVariablesProviderPort {
 public:
  ProblemVariablesFileAdapter(
      const ProblemData data, const std::vector<struct ActiveLink> vector_1,
      std::shared_ptr<ProblemGenerationLog::ProblemGenerationLogger>
          shared_ptr_1);
  ProblemVariables Provide() override;
  const ProblemData problem_data_;
  const std::vector<struct ActiveLink> active_links_;
  std::shared_ptr<ProblemGenerationLog::ProblemGenerationLogger> logger_;
};
