//
// Created by marechaljas on 02/11/22.
//

#pragma once

#include "IProblemProviderPort.h"
#include "LinkProblemsGenerator.h"
class MyAdapter : public IProblemProviderPort {
 public:
  explicit MyAdapter(std::filesystem::path lp_dir, ProblemData data);
  std::istringstream reader_extract(const ProblemData& problemData,
                                    ArchiveReader& reader) const;
  void reader_extract_file(const ProblemData& problemData,
                           ArchiveReader& reader,
                           std::filesystem::path lpDir) const;
  void extract_variables(
      std::istringstream& variableFileContent,
      std::vector<std::string>& var_names,
      std::map<colId, ColumnsToChange>& p_ntc_columns,
      std::map<colId, ColumnsToChange>& p_direct_cost_columns,
      std::map<colId, ColumnsToChange>& p_indirect_cost_columns,
      const std::vector<ActiveLink>& links,
      ProblemGenerationLog::ProblemGenerationLoggerSharedPointer logger) const;
  const std::filesystem::path lp_dir_;
  const ProblemData problem_data_;
  [[nodiscard]] std::shared_ptr<Problem> provide_problem(
      const std::string& solver_name) const override;
};
