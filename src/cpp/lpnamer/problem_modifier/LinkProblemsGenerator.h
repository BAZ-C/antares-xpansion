#pragma once

#include <multisolver_interface/SolverAbstract.h>

#include <filesystem>
#include <memory>
#include <mutex>

#include "ActiveLinks.h"
#include "ArchiveReader.h"
#include "ArchiveWriter.h"
#include "Candidate.h"
#include "FileInBuffer.h"
#include "IProblemProviderPort.h"
#include "ProblemGenerationLogger.h"
#include "ProblemModifier.h"
#include "VariableFileReader.h"
#include "common_lpnamer.h"

const std::string CANDIDATES_INI{"candidates.ini"};
const std::string STRUCTURE_FILE{"structure.txt"};
const std::string MPS_TXT{"mps.txt"};
const std::string MPS_ZIP_FILE{"MPS_ZIP_FILE"};
const std::string ZIP_EXT{".zip"};
const std::string STUDY_FILE{"study.antares"};
using CandidateNameAndMpsFilePath =
    std::pair<std::string, std::filesystem::path>;
using ColId = unsigned int;
using Couplings = std::map<CandidateNameAndMpsFilePath, ColId>;

struct ProblemData {
  ProblemData(const std::string& problem_mps, const std::string& variables_txt);
  std::string _problem_mps;
  std::string _variables_txt;
};

class LinkProblemsGenerator {
 public:
  LinkProblemsGenerator(
      const std::vector<ActiveLink>& links, const std::string& solver_name,
      ProblemGenerationLog::ProblemGenerationLoggerSharedPointer logger)
      : _links(links), _solver_name(solver_name), logger_(logger) {}

  void treatloop(const std::filesystem::path& root, Couplings& couplings,
                 const std::vector<ProblemData>& mps_list,
                 ArchiveWriter& writer, ArchiveReader& reader);
  std::vector<ProblemData> readMPSList(
      const std::filesystem::path& mps_filePath_p) const;

 private:
  void treat(const std::filesystem::path& root, ProblemData const&,
             Couplings& couplings, ArchiveReader& reader, ArchiveWriter& writer,
             IProblemProviderPort* problem_provider) const;

  const std::vector<ActiveLink>& _links;
  std::string _solver_name;
  std::filesystem::path lpDir_ = "";
  ProblemGenerationLog::ProblemGenerationLoggerSharedPointer logger_;
  mutable std::mutex coupling_mutex_;
};
