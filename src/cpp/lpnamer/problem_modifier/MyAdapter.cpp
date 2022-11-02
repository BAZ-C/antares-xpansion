//
// Created by marechaljas on 02/11/22.
//

#include "MyAdapter.h"

#include <algorithm>
#include <cassert>
#include <execution>

#include "LinkProblemsGenerator.h"
#include "VariableFileReader.h"
#include "helpers/StringUtils.h"
#include "solver_utils.h"
std::istringstream MyAdapter::reader_extract(const ProblemData& problemData,
                                             ArchiveReader& reader) const {
  auto variableFileContent =
      reader.ExtractFileInStringStream(problemData._variables_txt);
  return variableFileContent;
}
void MyAdapter::reader_extract_file(const ProblemData& problemData,
                                    ArchiveReader& reader,
                                    std::filesystem::path lpDir) const {
  reader.ExtractFile(problemData._problem_mps, lpDir);
}

void MyAdapter::extract_variables(
    std::istringstream& variableFileContent,
    std::vector<std::string>& var_names,
    std::map<colId, ColumnsToChange>& p_ntc_columns,
    std::map<colId, ColumnsToChange>& p_direct_cost_columns,
    std::map<colId, ColumnsToChange>& p_indirect_cost_columns,
    const std::vector<ActiveLink>& links,
    ProblemGenerationLog::ProblemGenerationLoggerSharedPointer logger) const {
  // List of variables
  VariableFileReadNameConfiguration variable_name_config;
  variable_name_config.ntc_variable_name = "ValeurDeNTCOrigineVersExtremite";
  variable_name_config.cost_origin_variable_name =
      "CoutOrigineVersExtremiteDeLInterconnexion";
  variable_name_config.cost_extremite_variable_name =
      "CoutExtremiteVersOrigineDeLInterconnexion";

  auto variableReader = VariableFileReader(variableFileContent, links,
                                           variable_name_config, logger);
  var_names = variableReader.getVariables();
  p_ntc_columns = variableReader.getNtcVarColumns();
  p_direct_cost_columns = variableReader.getDirectCostVarColumns();
  p_indirect_cost_columns = variableReader.getIndirectCostVarColumns();
}