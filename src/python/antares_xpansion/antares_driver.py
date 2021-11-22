"""
    Class to control the execution of the antares step
"""

import os
import subprocess
from datetime import datetime
from pathlib import Path

from antares_xpansion.study_output_cleaner import StudyOutputCleaner
import functools

print = functools.partial(print, flush=True)


class AntaresDriver:
    """
        Initialize Antares Driver with given binary path to Antares solver
    """

    def __init__(self, antares_exe_path: Path) -> None:

        self.antares_exe_path = antares_exe_path
        # antares study dir is set just before launch
        self.data_dir = ""

        self.output = 'output'
        self.ANTARES_N_CPU_OPTION = "--force-parallel"
        self.antares_n_cpu = 1  # default

    def launch(self, antares_study_path, antares_n_cpu: int):
        self._set_antares_n_cpu(antares_n_cpu)
        self._launch(antares_study_path)


    def _set_antares_n_cpu(self, antares_n_cpu: int):
        if antares_n_cpu >= 1:
            self.antares_n_cpu = antares_n_cpu
        else:
            print(f"WARNING! value antares_n_cpu= {antares_n_cpu} is not accepted, default value will be used.")

    def _launch(self, antares_study_path):
        self._clear_old_log()
        self.data_dir = antares_study_path
        self._run_antares()

    def _clear_old_log(self):
        logfile = str(self.antares_exe_path) + '.log'
        if os.path.isfile(logfile):
            os.remove(logfile)

    def antares_output_dir(self):
        """
            returns path to antares output data directory
        """
        return os.path.normpath(os.path.join(self.data_dir, self.output))

    def _run_antares(self):
        print("-- launching antares")

        start_time = datetime.now()

        returned_l = subprocess.run(self._get_antares_cmd(), shell=False,
                                    stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL)

        end_time = datetime.now()
        print('Antares simulation duration : {}'.format(end_time - start_time))

        if returned_l.returncode == 1:
            raise AntaresDriver.AntaresExecutionError("Error: exited antares with status {returned_l.returncode}")
        elif returned_l.returncode != 0 and returned_l.returncode != 1 :
            print("Warning: exited antares with status %d" % returned_l.returncode)
        else:
            self._set_simulation_name()
            StudyOutputCleaner.clean_antares_step((Path(self.antares_output_dir()) / self.simulation_name))

    def _set_simulation_name(self):

        self.simulation_name = ""
        # Get list of all dirs only in the given directory
        list_of_dirs_filter = filter(lambda x: os.path.isdir(os.path.join(self.antares_output_dir(), x)),
                                     os.listdir(self.antares_output_dir()))
        # Sort list of files based on last modification time in ascending order
        list_of_dirs = sorted(list_of_dirs_filter,
                              key=lambda x: os.path.getmtime(os.path.join(self.antares_output_dir(), x))
                              )
        if list_of_dirs:
            self.simulation_name = list_of_dirs[-1]

    def _get_antares_cmd(self):
        return [str(self.antares_exe_path), self.data_dir, self.ANTARES_N_CPU_OPTION, str(self.antares_n_cpu)]

    class Error(Exception):
        pass
    
    class AntaresExecutionError(Exception):
        pass
