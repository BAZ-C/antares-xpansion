#pragma once

#include "IProblemProviderPort.h"
#include "LpFromAntares_with_comments.h"
class LpFromAntaresProblemProviderAdapter : public IProblemProviderPort {
 public:
  LpFromAntaresProblemProviderAdapter(const LpsFromAntares& lp_from_antares, int year, int week);
  [[nodiscard]] std::shared_ptr<Problem> provide_problem(
      const std::string& solver_name) const override;

    // Comme suggéré par Manuel on voudrait plutôt 1 seul Adapter qui crée tous les pbs pour éviter d'avoir à copier les constantes data dans chaque adapter (et manipuler directement un LpsFromAntares)
  const ConstantDataFromAntaresPtr constant_;
  const HebdoDataFromAntaresPtr hebdo_;
};