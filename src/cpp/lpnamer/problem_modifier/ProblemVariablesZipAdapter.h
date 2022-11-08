//
// Created by marechaljas on 08/11/22.
//

#pragma once

#include "LinkProblemsGenerator.h"
struct ProblemVariables {
  std::vector<std::string> variable_names;
  std::map<colId, ColumnsToChange> ntc_columns;
  std::map<colId, ColumnsToChange> direct_cost_columns;
  std::map<colId, ColumnsToChange> indirect_cost_columns;
};

class ProblemVariablesZipAdapter {
 public:
  ProblemVariablesZipAdapter(
      std::shared_ptr<ArchiveReader> reader, const ProblemData data,
      const std::vector<ActiveLink>& vector,
      const std::shared_ptr<ProblemGenerationLog::ProblemGenerationLogger> ptr);
  ProblemVariables Provide();
  std::shared_ptr<ArchiveReader> archive_reader_;
  const ProblemData problem_data_;
  const std::vector<ActiveLink>& links_;
  const std::shared_ptr<ProblemGenerationLog::ProblemGenerationLogger> logger_;
  void extract_variables(
      std::istringstream& variableFileContent,
      std::vector<std::string>& var_names,
      std::map<colId, ColumnsToChange>& p_ntc_columns,
      std::map<colId, ColumnsToChange>& p_direct_cost_columns,
      std::map<colId, ColumnsToChange>& p_indirect_cost_columns) const;
};
