//
// Created by s90365 on 23/08/2021.
//
#include "MasterProblemBuilder.h"

#include <solver_utils.h>
#include <algorithm>
#include <unordered_map>

MasterProblemBuilder::MasterProblemBuilder(const std::string& master_formulation)
{
	_master_formulation = master_formulation;
}

std::shared_ptr<SolverAbstract> MasterProblemBuilder::build(const std::string& solverName, const std::vector<Candidate>& candidates)
{
	_indexOfNvar.clear();
	_indexOfPmaxVar.clear();

	SolverFactory factory;
	auto master_l = factory.create_solver(solverName);

	addVariablesPmaxOnEachCandidate(candidates, master_l);

	std::vector<Candidate> candidatesInteger;

	std::copy_if(candidates.begin(), candidates.end(),
		back_inserter(candidatesInteger),
		[](Candidate cand) { return cand.is_integer(); });

	if (_master_formulation == "integer")
	{
		addNvarOnEachIntegerCandidate(candidatesInteger, master_l);

		addPmaxConstraint(candidatesInteger, master_l);
	}

	return master_l;
}

void MasterProblemBuilder::addPmaxConstraint(const std::vector<Candidate>& candidatesInteger, SolverAbstract::Ptr& master_l)
{
	
	int n_integer = candidatesInteger.size();
	if (n_integer > 0)
	{
		std::vector<double> dmatval;
		std::vector<int> colind;
		std::vector<char> rowtype;
		std::vector<double> rhs;
		std::vector<int> rstart;
		dmatval.reserve(2 * n_integer);
		colind.reserve(2 * n_integer);
		rowtype.reserve(n_integer);
		rhs.reserve(n_integer);
		rstart.reserve(n_integer + 1);

		int positionInIntegerCandidadeList(0);
		int nbColPmaxVar = _indexOfPmaxVar.size();

		for (const auto& candidate : candidatesInteger)
		{
			int pmaxVarColumNumber = getPmaxVarColumnNumberFor(candidate);
			int nVarColumNumber = nbColPmaxVar + positionInIntegerCandidadeList;

			// pMax  - n unit_size = 0
			rstart.push_back(dmatval.size());
			rhs.push_back(0);
			rowtype.push_back('E');

			colind.push_back(pmaxVarColumNumber);
			dmatval.push_back(1);

			colind.push_back(nVarColumNumber);
			dmatval.push_back(-candidate.unit_size());

			positionInIntegerCandidadeList++;
		}
		rstart.push_back(dmatval.size());

		solver_addrows(master_l, rowtype, rhs, {}, rstart, colind, dmatval);
	}
}

int MasterProblemBuilder::getPmaxVarColumnNumberFor(const Candidate& candidate)
{
	const auto& it = _indexOfPmaxVar.find(candidate._name);
	if (it == _indexOfPmaxVar.end())
	{
		std::string message = "There is no Pvar for the candidate " + candidate._name;
		throw std::runtime_error(message);
	}
	return it->second;
}

void MasterProblemBuilder::addNvarOnEachIntegerCandidate(const std::vector<Candidate>& candidatesInteger, SolverAbstract::Ptr& master_l)
{
	
	int nbNvar = candidatesInteger.size();
	if (nbNvar > 0)
	{
		std::vector<double> zeros(nbNvar, 0.0);
		std::vector<int> mstart(nbNvar, 0);
		std::vector<char> integer_type(nbNvar, 'I');
		std::vector<std::string> colNames(0);
		std::vector<double> max_unit;
		
		for (int i = 0; i < candidatesInteger.size(); i++)
		{
			const auto& candidate = candidatesInteger.at(i);
			max_unit.push_back(candidate.max_unit());
		}

		solver_addcols(master_l, zeros, mstart, {}, {}, zeros, max_unit, integer_type, colNames);
	}
}

void MasterProblemBuilder::addVariablesPmaxOnEachCandidate(const std::vector<Candidate>& candidates, SolverAbstract::Ptr& master_l)
{
	int nbCandidates = candidates.size();

	if (nbCandidates > 0)
	{
		std::vector<int> mstart(nbCandidates, 0);
		std::vector<double> obj_candidate(nbCandidates, 0);
		std::vector<double> lb_candidate(nbCandidates, -1e20);
		std::vector<double> ub_candidate(nbCandidates, 1e20);
		std::vector<char> coltypes_candidate(nbCandidates, 'C');
		std::vector<std::string> candidate_names(nbCandidates);

		for (int i = 0; i < candidates.size(); i++) {
			const auto& candidate = candidates.at(i);
			obj_candidate[i] = candidate.obj();
			lb_candidate[i] = candidate.lb();
			ub_candidate[i] = candidate.ub();
			candidate_names[i] = candidate._name;
			_indexOfPmaxVar[candidate._name] = i;
		}

		solver_addcols(master_l, obj_candidate, mstart, {}, {}, lb_candidate, ub_candidate, coltypes_candidate, candidate_names);
	}
}