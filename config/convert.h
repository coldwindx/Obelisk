#pragma once

#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include "system.h"

__OBELISK__

template<typename F, typename T>
class Convert{
public:
    T operator()(const F & v){
        return boost::lexical_cast<T>(v);
    }
};

template<typename T>
class Convert<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T> & vc){
        YAML::Node node;
        for(auto & v : vc)
            node.push_back(YAML::Load(Convert<T, std::string>()(v)));
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<typename T>
class Convert<std::string, std::vector<T> >{
public:
    std::vector<T> operator()(const std::string & s){
        YAML::Node node = YAML::Load(s);
        std::vector<T> vc;
        std::stringstream ss;
        for(size_t i = 0, n = node.size(); i < n; ++i){
            ss.str("");
            ss << node[i];
            vc.push_back(Convert<std::string, T>()(ss.str()));
        }
        return vc;
    }
};

template<typename T>
class Convert<std::list<T>, std::string>{
public:
    std::string operator()(const std::list<T> & l){
        YAML::Node node;
		for (auto & i : l)
			node.push_back(YAML::Load(Convert<T, std::string>()(i)));

		std::stringstream ss;
		ss << node;
		return ss.str();
    }
};

template<typename T>
class Convert<std::string, std::list<T> >{
public:
    std::list<T> operator()(const std::string& s){
        YAML::Node node = YAML::Load(s);
        std::list<T> l;
        std::stringstream ss;
		for (size_t i = 0; i < node.size(); ++i) {
			ss.str("");
			ss << node[i];
			l.push_back(Convert<std::string, T>()(ss.str()));
		}
		return l;
    }
};

template<typename T>
class Convert<std::set<T>, std::string>{
public:
    std::string operator()(const std::set<T> & st){
        YAML::Node node;
        for(auto & v : st)
            node.push_back(YAML::Load(Convert<T, std::string>()(v)));
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<typename T>
class Convert<std::string, std::set<T> >{
public:
    std::set<T> operator()(const std::string& s){
        YAML::Node node = YAML::Load(s);
        std::set<T> st;
        std::stringstream ss;
        for(size_t i = 0, n = node.size(); i < n; ++i){
            ss.str("");
            ss << node[i];
            st.insert(Convert<std::string, T>()(ss.str()));
        }
        return st;
    }
};


template<typename T>
class Convert<std::string, std::unordered_set<T> > {
public:
	std::unordered_set<T> operator()(const std::string & v) {
		YAML::Node node = YAML::Load(v);
		std::unordered_set<T> vec;
		std::stringstream ss;
		for (size_t i = 0; i < node.size(); ++i) {
			ss.str("");
			ss << node[i];
			vec.insert(Convert<std::string, T>()(ss.str()));
		}
		return vec;
	}
};

template<typename T>
class Convert<std::unordered_set<T>, std::string> {
public:
	std::string operator()(const std::unordered_set<T> & v) {
		YAML::Node node;
		for (auto & i : v)
			node.push_back(YAML::Load(Convert<T, std::string>()(i)));

		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};


template<typename T>
class Convert<std::string, std::map<std::string, T> > {
public:
	std::map<std::string, T> operator()(const std::string & v) {
		YAML::Node node = YAML::Load(v);
		std::map<std::string, T> mp;
		std::stringstream ss;
		for (auto it = node.begin(); it != node.end(); ++it) {
			ss.str("");
			ss << it->second;
			mp.insert(std::make_pair(it->first.Scalar(), Convert<std::string, T>()(ss.str())));
		}
		return mp;
	}
};

template<typename T>
class Convert<std::map<std::string, T>, std::string> {
public:
	std::string operator()(const std::map<std::string, T> & v) {
		YAML::Node node;
		for (auto & i : v)
			node[i.first] = YAML::Load(Convert<T, std::string>()(i.second));

		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};


template<typename T>
class Convert<std::string, std::unordered_map<std::string, T> > {
public:
	std::unordered_map<std::string, T> operator()(const std::string & v) {
		YAML::Node node = YAML::Load(v);
		std::unordered_map<std::string, T> mp;
		std::stringstream ss;
		for (auto it = node.begin(); it != node.end(); ++it) {
			ss.str("");
			ss << it->second;
			mp.insert(std::make_pair(it->first.Scalar(), Convert<std::string, T>()(ss.str())));
		}
		return mp;
	}
};

template<typename T>
class Convert<std::unordered_map<std::string, T>, std::string> {
public:
	std::string operator()(const std::unordered_map<std::string, T> & v) {
		YAML::Node node;
		for (auto & i : v)
			node[i.first] = YAML::Load(Convert<T, std::string>()(i.second));

		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};
__END__