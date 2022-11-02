//
// Created by marechaljas on 02/11/22.
//

#pragma once

#include "ColumnToChange.h"
#include "LinkProblemsGenerator.h"
#include "Problem.h"

struct ProblemData;
class MyAdapter {
 public:
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
  [[nodiscard]] std::shared_ptr<Problem> get_solver_ptr(
      const std::string& solver_name, const std::filesystem::path& lpDir,
      const ProblemData& problemData) const;
  void provide_problem(
      const ProblemData& problemData, ArchiveReader& reader,
      const MyAdapter& adapter, std::vector<std::string>& var_names,
      std::map<colId, ColumnsToChange>& p_ntc_columns,
      std::map<colId, ColumnsToChange>& p_direct_cost_columns,
      std::map<colId, ColumnsToChange>& p_indirect_cost_columns,
      std::shared_ptr<Problem>& in_prblm, const std::string& solver_name,
      const std::filesystem::path& lpDir,
      const ProblemGenerationLog::ProblemGenerationLoggerSharedPointer& logger,
      const std::vector<ActiveLink>& links) const;
};
