#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <regex>
#include <map>
#include <chrono>

/*
    Set build logging verbosity to Detailed in Visual Studio (2017):
        Tools
        Options
        Projects and Solutions
        Build and Run
        MSBuild project build log file verbosity
        Detailed
    How to use extract_commands.exe:
        Run a batch build all
        Run extract_commands.exe in the stem (next to the solution)
*/

char const msc_bin_base_org[] = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Tools\\MSVC\\14.14.26428\\bin\\";
char const msc_bin_base_new[] = "%MSC_BIN_BASE%";

void script_head(std::ostream& makescript)
{
	makescript << "@echo off" << std::endl;
	makescript << "for /f \"tokens=1* delims==\" %%a in ('set') do (" << std::endl;
	makescript << "if \"%%a\" NEQ \"TMP\" (" << std::endl; /* TMP is required for the compiler */
	makescript << "if \"%%a\" NEQ \"windir\" (" << std::endl; /* windir is required for ps2pdf */
	makescript << "if \"%%a\" NEQ \"USERPROFILE\" (" << std::endl; /* USERPROFILE is required for ps2pdf */
	makescript << "if \"%%a\" NEQ \"SystemRoot\" ( set %%a=" << std::endl;
	makescript << ") ) ) ) )" << std::endl;

	makescript << "set OLDDIR=%CD%" << std::endl;
	makescript << "cd .." << std::endl;
	makescript << "set PROCTS_DIR=%CD%" << std::endl;
	makescript << "chdir /d %OLDDIR%" << std::endl;
	makescript << "echo %PROCTS_DIR%" << std::endl;

	makescript << "REM set MSC_INCLUDE_BASE=C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Tools\\MSVC\\14.14.26428\\" << std::endl;
	makescript << "REM set SDK_INCLUDE_BASE=C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.17134.0\\" << std::endl;
	makescript << "REM set MSC_BIN_BASE=" << msc_bin_base_org << std::endl;
	makescript << "REM set SDK_BIN_BASE=C:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0.17134.0\\" << std::endl;
	makescript << "REM set MSC_LIB_BASE=C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Tools\\MSVC\\14.14.26428\\lib\\" << std::endl;
	makescript << "REM set SDK_LIB_BASE=C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.17134.0\\" << std::endl;
	makescript << "set MSC_INCLUDE_BASE=%PROCTS_DIR%\\share000\\ext\\tgt\\msc\\win\\" << std::endl;
	makescript << "set SDK_INCLUDE_BASE=%PROCTS_DIR%\\share000\\ext\\tgt\\sdk\\win\\10.0.17134.0\\include\\" << std::endl;
	makescript << "set MSC_BIN_BASE=%PROCTS_DIR%\\share000\\ext\\tgt\\msc\\win\\bin\\" << std::endl;
	makescript << "set SDK_BIN_BASE=%PROCTS_DIR%\\share000\\ext\\tgt\\sdk\\win\\10.0.17134.0\\bin\\" << std::endl;
	makescript << "set MSC_LIB_BASE=%PROCTS_DIR%\\share000\\ext\\tgt\\msc\\win\\lib\\" << std::endl;
	makescript << "set SDK_LIB_BASE=%PROCTS_DIR%\\share000\\ext\\tgt\\sdk\\win\\10.0.17134.0\\lib\\" << std::endl;

	makescript << "SET INCLUDE="
		<< "%MSC_INCLUDE_BASE%include;"
		<< "%SDK_INCLUDE_BASE%shared;"
		<< "%SDK_INCLUDE_BASE%ucrt;"
		<< "%SDK_INCLUDE_BASE%um;"
		<< std::endl;
	makescript << "mkdir tgt" << std::endl
		       << "mkdir obj" << std::endl
		       << "mkdir tgt\\fig" << std::endl;
}

void script_platform_target(std::ostream& makescript, std::string const& platform, std::string const& short_target, std::string const& long_target, std::string& scomp)
{
	makescript << "SET PATH="
		<< "%MSC_BIN_BASE%Host" << platform << '\\' << short_target << ';';
	if (platform != short_target)
	{
		makescript
			<< "%MSC_BIN_BASE%Host" << platform << '\\' << platform << ';';
	}
	makescript
		<< "%SDK_BIN_BASE%" << short_target << ';'
		<< std::endl;
	makescript << "SET LIB="
		<< "%MSC_LIB_BASE%" << short_target << ';'
		<< "%SDK_LIB_BASE%ucrt\\" << short_target << ';'
		<< "%SDK_LIB_BASE%um\\" << short_target << ';'
		<< std::endl;
	makescript << "mkdir tgt\\" << long_target << std::endl
	           << "mkdir obj\\" << scomp << "\\" << long_target << std::endl;
}

void script_mkdir(std::ostream& makescript, std::string& scomp)
{
	makescript << "mkdir obj\\" << scomp << std::endl;
}

struct script_cd_comp_t
{
	explicit script_cd_comp_t(std::ostream& makescript, std::string& scomp)
		: m_makescript(makescript)
		, dont_do_it(false)
	{
		if (scomp == "makefile")
		{
			dont_do_it = true;
		}
		else
		{
			std::string adapted_comp = ((scomp == "cppmake_exe") ||
				(scomp == "cppmake_lib") ||
				(scomp == "cppmake_test_executor")) ? "cppmake" : scomp;
			makescript << "cd comp\\" << adapted_comp << std::endl;
		}
	}
	~script_cd_comp_t()
	{
		if (!dont_do_it)
		{
			m_makescript << "cd ..\\.." << std::endl;
		}
	}
	std::ostream& m_makescript;
	bool dont_do_it;
};

void script_document_compilation(std::ostream& makescript)
{
	makescript << "call make.cmd" << std::endl;
}

void script_figure_execution(std::string const& comp, std::ostream& makescript)
{
	makescript << "cd tgt\\fig" << std::endl;
	makescript << "call ..\\winx86r\\" << comp << ".exe" << std::endl;
	makescript << "cd ..\\.." << std::endl;
}

std::string replace_bin(std::string& command)
{
	std::string ret(command);
	size_t pos = command.find(msc_bin_base_org);
	if (pos != std::string::npos)
	{
		ret.replace(pos, strlen(msc_bin_base_org), msc_bin_base_new);
	}
	return ret;
}

std::string replace_procts(std::string& parameters)
{
	std::string ret(parameters);
	char const procts[] = ":\\procts";
	while (true)
	{
		size_t pos = ret.find(procts);
		if (pos == std::string::npos)
		{
			break;
		}
		ret.replace(pos-1, strlen(procts)+1, "%PROCTS_DIR%");
	}
	return ret;
}

void script_cpp_compilation(std::string& command, std::string parameters, std::ostream& makescript)
{
	makescript << "\"" << replace_bin(command) << "\" " << replace_procts(parameters) << std::endl;
}

void script_link(std::string& command, std::string parameters, std::ostream& makescript)
{
	makescript << "\"" << replace_bin(command) << "\" " << replace_procts(parameters) << std::endl;
}

void extract_cpp_commands(std::istream& log_file, std::ostream& makescript)
{
    std::string line;
    std::smatch base_match;
    std::regex  cl_regex("CL.exe");
	std::regex  tracker_regex("Tracker.exe");
	std::regex  link_regex("(link.exe)|(Lib.exe)");
	while (std::getline(log_file, line))
    {
		line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
		if (!std::regex_search(line, base_match, tracker_regex))
		{
			if (std::regex_search(line, base_match, cl_regex))
			{
				size_t end = base_match.position() + base_match.length();
				script_cpp_compilation(line.substr(0, end), line.substr(end + 1), makescript);
			}
			if (std::regex_search(line, base_match, link_regex))
			{
				size_t end = base_match.position() + base_match.length();
				std::string command(line.substr(0, end));
				std::string parameters(line.substr(end + 1));
				while (std::getline(log_file, line))
				{
					line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
					if (line == "Tracking command:")
					{
						break;
					}
					parameters += ' ';
					parameters += line;
				}
				script_link(command, parameters, makescript);
			}
		}
	}
}

using dir_map_t = std::multimap<std::filesystem::file_time_type, std::filesystem::directory_entry>;
using dir_map_pair_t = dir_map_t::value_type;

dir_map_t make_dir_map(std::filesystem::path obj_dir)
{
	dir_map_t dir_map;
	for (std::filesystem::directory_entry const& comp : std::filesystem::directory_iterator(obj_dir))
	{
		std::string scomp(comp.path().filename().string());
		for (std::filesystem::directory_entry const& target : std::filesystem::directory_iterator(comp))
		{
			std::string starget(target.path().filename().string());

			if (starget == "winx86r") /* I assume that the order / build dependencies are the same for other targets */
			{
				for (std::filesystem::directory_entry const& file : std::filesystem::directory_iterator(target))
				{
					if (file.path().filename() == scomp + ".log")
					{
						dir_map.insert(dir_map_pair_t(file.last_write_time(), comp));
					}
				}
			}
		}
	}
	return dir_map;
}

void script_cpp_components(dir_map_t const& dir_map, std::ostream& makescript)
{
	for (dir_map_pair_t const& pair : dir_map)
	{
		std::filesystem::directory_entry const& comp = pair.second;
		std::string scomp(comp.path().filename().string());
		script_mkdir(makescript, scomp);
		for (std::filesystem::directory_entry const& target : std::filesystem::directory_iterator(comp))
		{
			std::string starget(target.path().filename().string());

			if ((starget == "winx64d") ||
				(starget == "winx64r") ||
				(starget == "winx86d") ||
				(starget == "winx86r"))
			{
				char sztarget[4] = { starget[3], starget[4], starget[5], '\0' };
				script_platform_target(makescript, "x86" /*=host*/, sztarget, starget, scomp);
				for (std::filesystem::directory_entry const& file : std::filesystem::directory_iterator(target))
				{
					if (file.path().filename() == scomp + ".log")
					{
						script_cd_comp_t script_cd_comp(makescript, scomp);
						std::ifstream log_file(file);
						extract_cpp_commands(log_file, makescript);
					}
				}
			}
		}
	}
}

void script_figures(dir_map_t const& dir_map, std::ostream& makescript)
{
	for (dir_map_pair_t const& pair : dir_map)
	{
		std::string scomp(pair.second.path().filename().string());
		if (!scomp.compare(0, 4, "fig_"))
		{
			script_figure_execution(scomp, makescript);
		}
	}
}

void script_doc_components(std::filesystem::path const& comp_dir, std::ostream& makescript)
{
	for (std::filesystem::directory_entry const& comp : std::filesystem::directory_iterator(comp_dir))
	{
		std::string scomp(comp.path().filename().string());
		if (!scomp.compare(0, 4, "doc_") && (scomp != "doc_shared"))
		{
			script_cd_comp_t script_cd_comp(makescript, scomp);
			script_document_compilation(makescript);
		}
	}
}

void script_test(dir_map_t const& dir_map, std::ostream& makescript)
{
	bool has_test = false;
	for (dir_map_pair_t const& pair : dir_map)
	{
		std::string scomp(pair.second.path().filename().string());
		if(scomp == "test")
		{
			has_test = true;
			break;
		}
	}
	if (has_test)
	{
		makescript << "set result=SUCCEEDED" << std::endl;
		makescript << "cd tgt\\winx64r" << std::endl;
		makescript << "call test.exe" << std::endl;
		makescript << "if %errorlevel% NEQ 0 set result=FAILED" << std::endl;
		makescript << "cd ..\\.." << std::endl;
		makescript << "cd tgt\\winx86r" << std::endl;
		makescript << "call test.exe" << std::endl;
		makescript << "if %errorlevel% NEQ 0 set result=FAILED" << std::endl;
		makescript << "cd ..\\.." << std::endl;
		makescript << "echo == All Tests: %result% ===============================================" << std::endl;
	}
}

int main()
{
	std::filesystem::path stem_dir = std::filesystem::current_path();
	std::filesystem::path comp_dir = stem_dir / "comp";
	std::filesystem::path obj_dir = stem_dir / "obj";
	if (!std::filesystem::exists(comp_dir))
	{
		std::cerr << "Directory: '" << comp_dir << "' does not exist. Is the working directory the stem?" << std::endl;
		return -1;
	}
	if (!std::filesystem::exists(obj_dir))
	{
		std::cerr << "Directory: '" << obj_dir << "' does not exist" << std::endl;
		return -1;
	}
	dir_map_t dir_map = make_dir_map(obj_dir);
	// now start making the script
	std::ofstream makescript("make.cmd");
	script_head(makescript);
	script_cpp_components(dir_map, makescript);
	script_figures(dir_map, makescript);
	script_doc_components(comp_dir, makescript);
	script_test(dir_map, makescript);
	makescript.close();
	std::cout << "Ready" << std::endl;
	return 0;
}

