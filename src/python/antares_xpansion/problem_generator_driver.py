"""
    Class to control the Problem Generation
"""

import shutil
import os
import subprocess
from typing import List
import zipfile

import sys
from datetime import datetime
from dataclasses import dataclass
from pathlib import Path

from antares_xpansion.xpansion_utils import read_and_write_mps
from antares_xpansion.study_output_cleaner import StudyOutputCleaner
from antares_xpansion.yearly_weight_writer import YearlyWeightWriter
from antares_xpansion.xpansion_study_reader import XpansionStudyReader
from antares_xpansion.flushed_print import flushed_print


@dataclass
class ProblemGeneratorData:
    keep_mps: bool
    additional_constraints: str
    user_weights_file_path: Path
    weight_file_name_for_lp: str
    lp_namer_exe_path: Path
    active_years: List


class ProblemGeneratorDriver:
    class BasicException(Exception):
        pass

    class MpsZipFileException(BasicException):
        pass

    class AreaFileException(BasicException):
        pass

    class IntercoFilesException(BasicException):
        pass

    class OutputPathError(BasicException):
        pass

    class LPNamerExeError(BasicException):
        pass

    class LPNamerPathError(BasicException):
        pass

    class LPNamerExecutionError(BasicException):
        pass

    def __init__(self, problem_generator_data: ProblemGeneratorData) -> None:

        self.lp_namer_exe_path = Path(problem_generator_data.lp_namer_exe_path)
        self.LP_NAMER = self.lp_namer_exe_path.name
        self.keep_mps = problem_generator_data.keep_mps
        self.additional_constraints = problem_generator_data.additional_constraints
        self.user_weights_file_path = problem_generator_data.user_weights_file_path
        self.weight_file_name_for_lp = problem_generator_data.weight_file_name_for_lp
        self.active_years = problem_generator_data.active_years
        self.MPS_TXT = "mps.txt"
        self.is_relaxed = False
        self._lp_path = None
        self.mps_zip_filename = "MPS_ZIP"
        self.zip_ext = "zip"
        self.mps_zip_file = self.mps_zip_filename+"."+self.zip_ext

    def launch(self, output_path: Path, is_relaxed: bool):
        """
            problem generation step : getnames + lp_namer
        """
        self._clear_old_log()
        flushed_print("-- Problem Generation")
        self.output_path = output_path

        self._get_names()

        self.is_relaxed = is_relaxed
        self._lp_step()

    def set_output_path(self, output_path):

        if output_path.exists():
            self._output_path = output_path
            self._lp_path = os.path.normpath(
                os.path.join(self._output_path, 'lp'))
        else:
            raise ProblemGeneratorDriver.OutputPathError(
                f"{output_path} not found")

    def get_output_path(self):
        return self._output_path

    def _clear_old_log(self):
        if (os.path.isfile(str(self.lp_namer_exe_path) + '.log')):
            os.remove(str(self.lp_namer_exe_path) + '.log')

    def _get_names(self):
        """
            produces a .txt file describing the weekly problems:
            each line of the file contains :
             - mps file name
             - variables file name
             - constraints file name

            produces a file named with xpansionConfig.MPS_TXT
        """

        mps_txt = read_and_write_mps(self.output_path)
        with open(os.path.normpath(os.path.join(self.output_path, self.MPS_TXT)), 'w') as file_l, zipfile.ZipFile(self.output_path / self.mps_zip_file, "w") as mps_zip:
            for line in mps_txt.items():
                mps_sub_problem_file = line[1][0]
                mps_sub_problem_file_path = self.output_path / mps_sub_problem_file
                file_l.write(mps_sub_problem_file + ' ' + line[1]
                             [1] + ' ' + line[1][2] + '\n')
                mps_zip.write(mps_sub_problem_file_path,
                              mps_sub_problem_file, compress_type=zipfile.ZIP_DEFLATED)
                os.remove(mps_sub_problem_file_path)

        self._check_and_copy_area_file()
        self._check_and_copy_interco_file()

    def _check_and_copy_area_file(self):
        self._check_and_copy_txt_file(
            "area", ProblemGeneratorDriver.AreaFileException)

    def _check_and_copy_interco_file(self):
        self._check_and_copy_txt_file(
            "interco", ProblemGeneratorDriver.IntercoFilesException)

    def _check_and_copy_txt_file(self, prefix, exception_to_raise: BasicException):
        self._check_and_copy_file(prefix, "txt", exception_to_raise)

    def _check_and_copy_file(self, prefix, extension, exception_to_raise: BasicException):
        glob_path = Path(self.output_path)
        files = [str(pp) for pp in glob_path.glob(prefix + "*" + extension)]
        if len(files) == 0:
            raise exception_to_raise("No %s*.txt file found" % prefix)

        elif len(files) > 1:
            raise exception_to_raise(
                "More than one %s*.txt file found" % prefix)

        shutil.copy(files[0], os.path.normpath(
            os.path.join(self.output_path, prefix + '.' + extension)))

    def _lp_step(self):
        """
            copies area and interco files and launches the lp_namer

            produces a file named with xpansionConfig.MPS_TXT
        """

        if os.path.isdir(self._lp_path):
            shutil.rmtree(self._lp_path)
        os.makedirs(self._lp_path)

        shutil.move(str(self.output_path/self.mps_zip_file), self._lp_path)
        if self.weight_file_name_for_lp:
            XpansionStudyReader.check_weights_file(
                self.user_weights_file_path, len(self.active_years))
            weight_list = XpansionStudyReader.get_years_weight_from_file(
                self.user_weights_file_path)
            YearlyWeightWriter(Path(self.output_path)).create_weight_file(weight_list, self.weight_file_name_for_lp,
                                                                          self.active_years)

        with open(self.get_lp_namer_log_filename(), 'w') as output_file:

            start_time = datetime.now()
            returned_l = subprocess.run(self._get_lp_namer_command(), shell=False,
                                        stdout=output_file,
                                        stderr=output_file)

            end_time = datetime.now()
            flushed_print('Post antares step duration: {}'.format(
                end_time - start_time))

            if returned_l.returncode != 0:
                raise ProblemGeneratorDriver.LPNamerExecutionError(
                    "ERROR: exited lpnamer with status %d" % returned_l.returncode)
            elif not self.keep_mps:
                StudyOutputCleaner.clean_lpnamer_step(Path(self.output_path))

    def get_lp_namer_log_filename(self):
        if not self._lp_path:
            raise ProblemGeneratorDriver.LPNamerPathError(
                "Error output path is not given")
        return os.path.join(self._lp_path, os.path.splitext(self.LP_NAMER)[0] + '.log')

    def _get_lp_namer_command(self):

        is_relaxed = 'relaxed' if self.is_relaxed else 'integer'
        if not self.lp_namer_exe_path.is_file():
            raise ProblemGeneratorDriver.LPNamerExeError(
                f"LP namer exe: {self.lp_namer_exe_path} not found")

        return [self.lp_namer_exe_path, "-o", str(self.output_path), "-f", is_relaxed, "-e",
                self.additional_constraints]

    output_path = property(get_output_path, set_output_path)
