import configparser
import os
import subprocess
from pathlib import Path
from unittest.mock import patch

import pytest
from antares_xpansion.antares_driver import AntaresDriver
from antares_xpansion.general_data_processor import (
    GeneralDataFileExceptions,
    GeneralDataProcessor,
)
from antares_xpansion.general_data_reader import IniReader, GeneralDataIniReader

from tests.build_config_reader import get_antares_solver_path

SUBPROCESS_RUN = "antares_xpansion.antares_driver.subprocess.run"


class TestGeneralDataProcessor:
    def setup_method(self):
        self.generaldata_filename = "generaldata.ini"

    def test_with_non_existing_general_data_file(self, tmp_path):

        settings_dir = self.get_settings_dir(tmp_path)
        settings_dir.mkdir()
        with pytest.raises(GeneralDataFileExceptions.GeneralDataFileNotFound):
            GeneralDataProcessor(settings_dir, True)

    def test_general_data_file_path(self, tmp_path):

        settings_dir = TestGeneralDataProcessor.get_settings_dir(tmp_path)
        settings_dir.mkdir()
        expected_path = settings_dir / self.generaldata_filename
        expected_path.touch()
        gen_data_proc = GeneralDataProcessor(settings_dir, True)
        assert gen_data_proc.general_data_ini_file == expected_path

    def test_preserve_playlist(self, tmp_path):
        settings_dir = TestGeneralDataProcessor.get_settings_dir(tmp_path)
        settings_dir.mkdir()
        gen_data_path = settings_dir / self.generaldata_filename

        with open(gen_data_path, "w") as writer:
            writer.write("[general]\nnbyears=2\nuser-playlist = true\n")
            writer.write("[playlist]\n")
            writer.write("dummy_entry = value\n")
            writer.write("playlist_year + = 1\n")
            writer.write("playlist_year + = 42\n")
            writer.write("playlist_year - = 5\n")
            writer.write("playlist_year - = 0\n")

        gen_data_proc = GeneralDataProcessor(settings_dir, True)

        gen_data_proc.change_general_data_file_to_configure_antares_execution()
        general_data_ini_file = gen_data_proc.general_data_ini_file

        xpansion_ini_reader = GeneralDataIniReader(general_data_ini_file)
        xpansion_ini_reader.get_active_years()
        config_reader = configparser.ConfigParser(strict=False)
        config_reader.read(gen_data_path)
        active_years, inactive_years = xpansion_ini_reader.get_raw_playlist()
        assert config_reader.get("playlist", "dummy_entry") == "value"
        assert active_years == [1, 42]
        assert inactive_years == [5, 0]

    def test_empty_file_should_be_populated_by_default_values(self, tmp_path):
        settings_dir = TestGeneralDataProcessor.get_settings_dir(tmp_path)
        settings_dir.mkdir()
        gen_data_path = settings_dir / self.generaldata_filename
        gen_data_path.touch()
        gen_data_proc = GeneralDataProcessor(settings_dir, True)
        gen_data_proc.change_general_data_file_to_configure_antares_execution()

        with open(gen_data_proc.general_data_ini_file, "r") as reader:
            lines = reader.readlines()

        assert len(lines) != 0
        parser = configparser.ConfigParser()
        parser.read(gen_data_proc.general_data_ini_file)
        for option in gen_data_proc._get_values_to_change_general_data_file():
            assert parser.get(option[0], option[1]) is not None

    def test_values_change_in_general_file_accurate_mode(self, tmp_path):

        settings_dir = TestGeneralDataProcessor.get_settings_dir(tmp_path)
        settings_dir.mkdir()
        gen_data_path = settings_dir / self.generaldata_filename

        is_accurate = True
        default_val = (
            "[general] \n"
            "mode = unrelevant\n"
            "year-by-year = true\n"
            "[optimization] \n"
            "include-exportmps = true\n"
            "include-tc-minstablepower = false\n"
            "include-dayahead = NO\n"
            "include-usexprs = value\n"
            "include-inbasis = value\n"
            "include-outbasis = value\n"
            "include-trace = value\n"
            "[output]\n"
            "storenewset = false\n"
            "[other preferences] \n"
            "unit-commitment-mode = fast\n"
            "[random_section] \n"
            "key1 = value1\n"
            "key2 = value2\n"
            "[input]\n"
            "import = blabla\n"

        )

        gen_data_path.write_text(default_val)
        optimization = "optimization"
        general = "general"
        random_section = "random_section"
        output = "output"
        other_preferences = "other preferences"

        expected_val = {
            (optimization, "include-exportmps"): "optim-1",
            (optimization, "include-exportstructure"): "true",
            (optimization, "include-tc-minstablepower"): "true",
            (optimization, "include-tc-min-ud-time"): "true",
            (optimization, "include-dayahead"): "true",
            (general, "mode"): "expansion",
            (output, "storenewset"): "true",
            (other_preferences, "unit-commitment-mode"): "accurate",
            (random_section, "key1"): "value1",
            (random_section, "key2"): "value2",
            ("adequacy patch", "include-adq-patch"): "false",
            (general, "year-by-year"): "false",
            (output, "synthesis"): "false",
            ("input", "import"): "",
        }

        gen_data_proc = GeneralDataProcessor(settings_dir, is_accurate)

        gen_data_proc.change_general_data_file_to_configure_antares_execution()
        general_data_ini_file = gen_data_proc.general_data_ini_file
        self.verify_that_general_data_contains_expected_vals(
            general_data_ini_file, expected_val
        )

    def test_values_change_in_general_file_fast_mode(self, tmp_path):
        study_path = tmp_path
        settings_dir = study_path / "settings"
        settings_dir.mkdir()
        general_data_path = settings_dir / self.generaldata_filename

        is_accurate = False
        default_val = (
            "[general] \n"
            "mode = expansion\n"
            "[optimization] \n"
            "include-exportmps = false\n"
            "include-tc-minstablepower = false\n"
            "include-dayahead = NO\n"
            "include-usexprs = value\n"
            "include-inbasis = value\n"
            "include-outbasis = value\n"
            "include-trace = value\n"
            "[output]\n"
            "storenewset = false\n"
            "synthesis = true\n"
            "[other preferences] \n"
            "unit-commitment-mode = dada\n"
            "[random_section] \n"
            "key1 = value1\n"
            "key2 = value2\n"
            "[input]\n"
            "import =\n"
        )

        general_data_path.write_text(default_val)

        # Removing '[' and ']' from sections name
        optimization = "optimization"
        general = "general"
        random_section = "random_section"
        other_preferences = "other preferences"
        output = "output"
        expected_val = {
            (optimization, "include-exportmps"): "optim-1",
            (optimization, "include-exportstructure"): "true",
            (optimization, "include-tc-minstablepower"): "false",
            (optimization, "include-tc-min-ud-time"): "false",
            (optimization, "include-dayahead"): "false",
            (general, "mode"): "Economy",
            (output, "storenewset"): "true",
            (other_preferences, "unit-commitment-mode"): "fast",
            (random_section, "key1"): "value1",
            (random_section, "key2"): "value2",
            ("adequacy patch", "include-adq-patch"): "false",
            (general, "year-by-year"): "false",
            (output, "synthesis"): "false",
            ("input", "import"): "",
        }

        gen_data_proc = GeneralDataProcessor(settings_dir, is_accurate)
        gen_data_proc.change_general_data_file_to_configure_antares_execution()

        self.verify_that_general_data_contains_expected_vals(
            general_data_path, expected_val
        )

    def verify_that_general_data_contains_expected_vals(
            self, general_data_ini_file, expected_val
    ):
        actual_config = configparser.ConfigParser()
        actual_config.read(general_data_ini_file)
        for (section, key) in expected_val:
            value = actual_config.get(section, key, fallback=None)
            assert value is not None
            print(f"Section {section}, key {key}, value {value}, expected {expected_val[(section, key)]}")
            assert value == expected_val[(section, key)]

        with open(general_data_ini_file, "r") as reader:
            current_section = ""
            lines = reader.readlines()
            for line in lines:
                if IniReader.line_is_not_a_section_header(line):
                    key = line.split("=")[0].strip()
                    value = line.split("=")[1].strip()
                    if (current_section, key) in expected_val:
                        assert value == expected_val[(current_section, key)]
                else:
                    current_section = line.strip()

    @staticmethod
    def get_settings_dir(antares_study_path: Path):
        return antares_study_path / "settings"


class TestAntaresDriver:
    def test_antares_cmd(self, tmp_path):
        study_dir = tmp_path
        exe_path = "/Path/to/bin1"
        antares_driver = AntaresDriver(exe_path)
        # mock subprocess.run
        with patch(SUBPROCESS_RUN, autospec=True) as run_function:
            antares_driver.launch(study_dir, 1)
            expected_cmd = [exe_path, study_dir, "--force-parallel", "1", "-z"]
            run_function.assert_called_once_with(
                expected_cmd, shell=False, stdout=-3, stderr=-3
            )

    def test_empty_study_dir(self, tmp_path):

        study_dir = tmp_path
        print(f"Study dir : {study_dir}")
        os.listdir()
        print("Testing exe")
        print(f"PAth {get_antares_solver_path()}")
        if os.path.exists(get_antares_solver_path()):
            print("Exe exists")
        else:
            print("Exe doesn't exist")
        print("Test if executable ")
        if os.path.isfile(get_antares_solver_path()) and os.access(get_antares_solver_path(), os.X_OK):
            print("File is executable")
        else:
            print("File is not executable")

        returned_l = subprocess.run([get_antares_solver_path(), "--version"], shell=False,
                                    capture_output=True)
        print(returned_l.stdout)
        print(returned_l.stderr)
        antares_driver = AntaresDriver(get_antares_solver_path())
        antares_driver.launch(study_dir, 1)
