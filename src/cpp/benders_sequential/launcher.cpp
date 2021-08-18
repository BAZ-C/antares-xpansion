#include "glog/logging.h"

#include "launcher.h"
#include "benders_sequential_core/Benders.h"
#include "Timer.h"
#include "JsonWriter.h"

#include "BendersOptions.h"

/*!
*  \brief Get Benders Options from command line
*
*  \param argc : number of elements
*
*  \param argv : elements on command line
*/
BendersOptions build_benders_options(int argc, char** argv) {
	BendersOptions result;
	result.read(argv[1]);
	return result;
}

/*!
*  \brief Build the input from the structure file
*
*	Function to build the map linking each problem name to its variables and their id
*
*  \param root : root of the structure file
*
*  \param summary_name : name of the structure file
*
*  \param coupling_map : empty map to increment
*
*  \note The id in the coupling_map is that of the variable in the solver responsible for the creation of
*  the structure file.
*/
CouplingMap build_input(BendersOptions const & options) {
    CouplingMap coupling_map;
	std::ifstream summary(options.get_structure_path(), std::ios::in);
	if (!summary) {
	    //TODO JMK : gestion cas d'erreur si pas de structure d'entrée
		std::cout << "Cannot open file summary " << options.get_structure_path() << std::endl;
		return coupling_map;
	}
	std::string line;

	while (std::getline(summary, line))
	{
		std::stringstream buffer(line);
		std::string problem_name;
		std::string variable_name;
		int variable_id;
		buffer >> problem_name;
		buffer >> variable_name;
		buffer >> variable_id;
		coupling_map[problem_name].push_back(variable_name);	
	}

	if (options.SLAVE_NUMBER >= 0) {
		int n(0);
		CouplingMap trimmer;
		for (auto const & problem : coupling_map) {
			if (problem.first == options.MASTER_NAME)
				trimmer.insert(problem);
			else if (n< options.SLAVE_NUMBER){
				trimmer.insert(problem);
				++n;
			}
		}
		coupling_map = trimmer;
	}
	summary.close();
	return coupling_map;
}


/*!
*  \brief Execute the Benders algorithm in sequential
*
*  \param options : set of Benders options
*/
void sequential_launch(BendersOptions const & options,  Logger & logger) {
	Timer timer;

	LOG(INFO) << "Building input" << std::endl;
    CouplingMap input = build_input(options);

	JsonWriter jsonWriter_l;
	jsonWriter_l.write_failure();
	jsonWriter_l.dump(options.OUTPUTROOT + PATH_SEPARATOR + options.JSON_NAME + ".json");

	jsonWriter_l.write(options);
	jsonWriter_l.updateBeginTime();

	LOG(INFO) << "Constructing workers..." << std::endl;

    Benders benders(logger);
	LOG(INFO) << "Running solver..." << std::endl;
    try {
	    benders.run(input,options);
	    LOG(INFO) << "Benders solver terminated." << std::endl;
    }catch (std::exception& ex) {
        std::string error = "Exception raised : " + std::string(ex.what());
        LOG(WARNING) << error << std::endl;
        logger->display_message(error);
    }

    LogData logData = defineLogDataFromBendersDataAndTrace(benders._data, benders._trace);
	logData.optimal_gap = options.GAP;

    logger->log_at_ending(logData);
	jsonWriter_l.updateEndTime();
	jsonWriter_l.write(input.size(), benders._trace, benders._data, options.GAP);
	jsonWriter_l.dump(options.OUTPUTROOT + PATH_SEPARATOR + options.JSON_NAME + ".json");

	benders.free();
	logger->log_total_duration(timer.elapsed());
}

/*!
*  \brief How to call for the algorithm
*
*  \param argc : number of arguments in command line
*/
void usage(int argc) {
	if (argc < 2) {
		std::cout << "usage is : <exe> <option_file> " << std::endl;
		BendersOptions input;
		input.write_default();
		std::exit(1);
	}
}

