#include <tuple>
#include "log_formatter.h"
#include "format_manager.h"

__SUN__

LogFormatter::LogFormatter(const std::string & pattern) : m_pattern(pattern) {
    this->init();
}
/* 模式解析逻辑 */
void LogFormatter::init(){
		std::vector<std::tuple<std::string, std::string, int>> vec;		// str, format, type
		std::string nstr;

		int i, j, n = m_pattern.size();
		for (i = 0; i < n; ++i) {
			if ('%' != m_pattern[i]) {
				nstr.append(1, m_pattern[i]);
				continue;
			}
			if (i + 1 < n && '%' == m_pattern[i + 1]) {
				nstr.append(1, '%');
				++i;
				continue;
			}

			bool status = false;		// status - false:转译串 true:格式串
			int fmtBegin = 0;

			std::string pat, fmt;
			for (j = i + 1; j < n; ++j) {
				if (!status && (!std::isalpha(m_pattern[j])) && m_pattern[j] != '{' && m_pattern[j] != '}') {
					pat = m_pattern.substr(i + 1, j - i - 1);
					break;
				}
				if (!status) {
					if ('{' == m_pattern[j]) {
						pat = m_pattern.substr(i + 1, j - i - 1);
						status = true;
						fmtBegin = j;
						continue;
					}
				}
				if (status) {
					if ('}' == m_pattern[j]) {
						fmt = m_pattern.substr(fmtBegin, j - fmtBegin - 1);
						status = false;
						++j;
						break;
					}
				}
			}

			if (j == n && pat.empty())
				pat = m_pattern.substr(i + 1);

			if (!status) {
				if (!nstr.empty()) {
					vec.push_back(std::make_tuple(nstr, std::string(), 0));
					nstr.clear();
				}
				vec.push_back(std::make_tuple(pat, fmt, 1));
				i = j - 1;
			}
			if (status) {
				//std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
				m_error = true;
				vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
			}
		}
		if (!nstr.empty())
			vec.push_back(std::make_tuple(nstr, std::string(), 0));

		FormatManager::ptr manager = FormatManager::instance();
		for (auto & i : vec) {
			if (0 == std::get<2>(i)) {
				m_formats.push_back(Format::ptr(new StringFormat(std::get<0>(i))));
				continue;
			}
			if (manager->has(std::get<0>(i)))
				m_formats.push_back(manager->get(std::get<0>(i), std::get<1>(i)));
			else {
				m_error = true;
				m_formats.push_back(Format::ptr(new StringFormat("<<error format %" + std::get<0>(i) + ">>")));
			}
		}
}

std::string LogFormatter::format(LogEvent::ptr event){
    std::stringstream os;
	for (auto i : m_formats)
		i->format(os, event);
	return os.str();
}
__END__