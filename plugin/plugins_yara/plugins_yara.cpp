/*
    This file is part of Spike Guard.

    Spike Guard is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Spike Guard is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Spike Guard.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "yara_wrapper.h"
#include "plugin/plugin_interface.h"
#include "plugin/auto_register.h"

namespace plugin
{

class YaraPlugin : public IPlugin
{

public:
	YaraPlugin(const std::string& rule_file) : _rule_file(rule_file) 
	{
		if (!_engine.load_rules(_rule_file)) 
		{
			PRINT_ERROR << "Could not load " << rule_file << "!" << std::endl;
			_initialized = false;
		}
		else {
			_initialized = true;
		}
	}

	/**
	 *	@brief	Helper function designed to generically prepare a result based on a Yara scan.
	 *
	 *	@param	const sg::PE& pe The PE to scan.
	 *	const std::string& summary The summary to set if there is a match.
	 *	Result::LEVEL level The threat level to set if there is a match.
	 *	const std::string& meta_field_name The meta field name (of the yara rule) to query to
	 *									   extract results.
	 *
	 *	@return	A pResult detailing the findings of the scan.
	 */
	pResult scan(const sg::PE& pe, const std::string& summary, Result::LEVEL level, const std::string& meta_field_name)
	{
		pResult res = pResult(new Result);
		if (!_initialized) {
			return res;
		}

		yara::matches m = _engine.scan_file(pe.get_path());
		if (m.size() > 0) 
		{
			res->set_level(level);
			res->set_summary(summary);
			for (yara::matches::iterator it = m.begin() ; it != m.end() ; ++it) {
				res->add_information((*it)->operator[](meta_field_name));
			}
		}

		return res;
	}

	int get_api_version() { return 1; }

private:
	std::string _rule_file;
	yara::Yara _engine;
	bool _initialized;
};

class ClamavPlugin : public YaraPlugin
{
public:
	ClamavPlugin() : YaraPlugin("resources/clamav.yara") {}

	pResult analyze(const sg::PE& pe) {
		return scan(pe, "Matching ClamAV signature(s):", Result::MALICIOUS, "signature");
	}

	boost::shared_ptr<std::string> get_id() { 
		return boost::shared_ptr<std::string>(new std::string("clamav"));
	}

	boost::shared_ptr<std::string> get_description() { 
		return boost::shared_ptr<std::string>(new std::string("Scans the binary with ClamAV virus definitions."));
	}
};

class CompilerDetectionPlugin : public YaraPlugin
{
public:
	CompilerDetectionPlugin() : YaraPlugin("resources/compilers.yara") {}

	pResult analyze(const sg::PE& pe) {
		return scan(pe, "Matching compiler(s):", Result::NO_OPINION, "description");
	}

	boost::shared_ptr<std::string> get_id() { 
		return boost::shared_ptr<std::string>(new std::string("compilers"));
	}

	boost::shared_ptr<std::string> get_description() { 
		return boost::shared_ptr<std::string>(new std::string("Tries to determine the compiler which generated the binary."));
	}
};

AutoRegister<ClamavPlugin> auto_register_clamav;
AutoRegister<CompilerDetectionPlugin> auto_register_compiler;

}
