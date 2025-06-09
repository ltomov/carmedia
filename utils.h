#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <string>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;

void say(string);


template <class T, class A>
T join(const A &begin, const A &end, const T &t)
{
  T result;
  for (A it=begin;
       it!=end;
       it++)
  {
    if (!result.empty())
      result.append(t);
    result.append(*it);
  }
  return result;
}

string normalizePath(string);
std::vector<std::string> splitByLength(std::string const & s, size_t count);

#endif
