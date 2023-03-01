#pragma once

#include "ArchiveReader.h"
#include "BendersBase.h"
#include "common.h"
#include "core/ILogger.h"

/*!
 * \class BendersSequential
 * \brief Class use run the benders algorithm in sequential
 */
class BendersSequential : public BendersBase {
 public:
  explicit BendersSequential(BendersBaseOptions const &options, Logger logger,
                             Writer writer);
  virtual ~BendersSequential() = default;
  virtual void launch();
  virtual void BuildCut();
  virtual void InitializeProblems();
  std::string BendersName() const { return "Sequential"; }

 protected:
  virtual void free();
  virtual void Run();
  [[nodiscard]] bool shouldParallelize() const final { return true; }
};
