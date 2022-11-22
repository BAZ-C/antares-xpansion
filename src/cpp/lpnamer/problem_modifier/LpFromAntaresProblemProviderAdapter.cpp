#include "LpFromAntaresProblemProviderAdapter.h"

#include "multisolver_interface/SolverAbstract.h"
#include "multisolver_interface/SolverFactory.h"

LpFromAntaresProblemProviderAdapter::LpFromAntaresProblemProviderAdapter(
    const LpsFromAntares& lp_from_antares, int year, int week)
    : constant_(lp_from_antares._constant),
      hebdo_(lp_from_antares._hebdo.at({year, week})) {}

std::shared_ptr<Problem> LpFromAntaresProblemProviderAdapter::provide_problem(
    const std::string& solver_name) const {
  SolverFactory factory;
  auto in_prblm = std::make_shared<Problem>(factory.create_solver(solver_name));

  in_prblm->add_cols(constant_->NombreDeVariables, 0,
                     hebdo_->CoutLineaire.data(), constant_->Mdeb.data(), {},
                     {}, hebdo_->Xmin.data(), hebdo_->Xmax.data());
  in_prblm->add_rows(constant_->NombreDeContraintes,
                     constant_->NombreDeCoefficients, hebdo_->Sens.data(),
                     hebdo_->SecondMembre.data(), {}, constant_->Mdeb.data(),
                     constant_->IndicesColonnes.data(),
                     constant_->CoefficientsDeLaMatriceDesContraintes.data());

    // On peut ajouter la partie qui renomme les variables ici si on stocke les données du type de variabes dans ConstantDataFromAntares, i.e. en définissant une autre implémentation de IProblemVariablesProviderPort
  return in_prblm;
}