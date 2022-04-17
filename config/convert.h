#pragma once

#include <vector>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include "../system.h"

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
            node.push_back(v);
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<typename T>
class Convert<std::string, std::vector<T> >{
public:
    std::vector<T> opeator()(const std::string & s){
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
__END__