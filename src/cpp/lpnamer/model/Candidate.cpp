#include "Candidate.h"

Candidate::Candidate(const CandidateData& data, const LinkProfile& profile):
    _profile (profile),
    _name (data.name),
    _annual_cost_per_mw (data.annual_cost_per_mw),
    _max_investment (data.max_investment),
    _unit_size (data.unit_size),
    _max_units (data.max_units)
{}

double Candidate::direct_profile(size_t timeStep) const{
    return _profile.getDirectProfile(timeStep);
}

double Candidate::indirect_profile(size_t timeStep) const{
    return _profile.getIndirectProfile(timeStep);
}

double Candidate::obj()const {
    return _annual_cost_per_mw;
}

double Candidate::lb() const {
    return 0;
}

double Candidate::ub() const {
    double val = _max_investment;
    if(val == 0.0) {
        val = unit_size() * max_unit();
    }
    return val;
}

double Candidate::unit_size() const {
    return _unit_size;
}
double Candidate::max_unit() const {
    return _max_units;
}

bool Candidate::is_integer()const {
    return _unit_size!=0.0;
}

std::string Candidate::get_name() const{
    return _name;
}

void Candidate::set_name(const std::string& name){
    _name = name;
}