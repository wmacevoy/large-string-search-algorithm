#include <fstream>
#include <string>
#include <stdint.h>
#include <cassert>
#include <vector>
#include <iostream>

bool readfile(const std::string &file, std::vector<uint8_t> &data) {
  std::ifstream in(file,std::ios::binary);
  if (!in.seekg (0, std::ios::end)) return false;
  std::size_t size = in.tellg();
  data.resize(size);
  if (!in.seekg (0, std::ios::beg)) return false;
  if (!in.read ((char*)&data[0],size)) return false;

  return true;
}

//
// data restricted to [begin,end) matches pattern at [at,at+pattern.length())
//
bool matches(const std::string &pattern,std::size_t at,
	     const std::vector<uint8_t> &data,
	     std::size_t begin, std::size_t end)
{
  std::size_t n=pattern.length();
  if (begin <= at && at+n <= end) {
    return pattern.compare(0,n,(char*)&data[at],n) == 0;
  }
  return false;
}

//
// find first occurance of pattern in data restricted to [begin,end)
//
size_t find(const std::string &pattern,
	    const std::vector<uint8_t> &data,
	    size_t begin=0, size_t end=std::string::npos)
{
  if (begin < 0) {
    begin=0;
  }

  if (end == std::string::npos || end > data.size()) {
    end = data.size();
  }

  if (pattern.length() == 0) {
    return begin <= end ? begin : std::string::npos;
  }
  
  std::size_t last[256] = { std::string::npos };
  std::size_t prev[pattern.length()];
  for (std::size_t i=0; i<pattern.size(); ++i) {
    last[pattern[i]]=i;
    prev[i]=(i > 0) ? pattern.rfind(pattern[i],i-1) : std::string::npos;
  }

  for (std::size_t i=begin; i < end; i += pattern.size()) {
    for (std::size_t at = last[data[i]];
	 at != std::string::npos;
	 at = prev[at]) {
      if (matches(pattern,i-at,data,begin,end)) {
	return i - at;
      }
    }
  }

  return std::string::npos;
}

struct lastly {
  void (*fini)();
  lastly(void (*_fini)()) : fini(_fini) {}
  ~lastly() { fini(); }
};

bool testreadfile() {
  lastly cleanup([] { remove("sample.dat"); });

  std::vector<uint8_t> data;
  for (std::size_t sz = 0; sz < 100'000; sz += (sz < 10) ? 1 : 997) {
    {
      std::ofstream sample("sample.dat",std::ios::binary);
      for (std::size_t i = 0; i<sz; ++i) {
	sample.put((sz-i) % 256);
      }
    }
    {
      assert(readfile("sample.dat", data));
      assert(data.size() == sz);
      for (std::size_t i = 0; i<sz; ++i) {
	assert(data[i]==((sz-i) % 256));
      }
    }
  }
  return true;
}



bool testmatches()
{
  for (int datalen=0; datalen <= 10; ++datalen) {
    std::vector<uint8_t> data(datalen,0);
    assert(data.size()==datalen);
    for (int i=0; i<datalen; ++i) {
      data[i]="abc"[i % 3];
    }
    for (std::string pattern : {"", "a","b","c","ab","bc","ca","abc","bca","cab","abca","bcab","cabc","abcab","bcabc","cabcab","abcabc","bcabca","cabcab"}) {
      for (int begin = 0; begin < datalen; ++begin) {
	for (int end = 0; end <= datalen; ++end) {
	  for (int at = 0; at < datalen; ++at) {
	    bool expect = (begin <= at && at+pattern.length() <= end);
	    for (int i=0; i<pattern.length(); ++i) {
	      expect = expect &&
		(begin <= at+i && at+i < end) &&
		(data[at+i] == pattern[i]);
	    }
	    assert(matches(pattern,at,data,begin,end)==expect);
	  }
	}
      }
    }
  }
  return true;
}


bool testfind()
{
  for (int datalen=0; datalen <= 10; ++datalen) {
    std::vector<uint8_t> data(datalen,0);
    std::string datastr;
    assert(data.size()==datalen);
    for (int i=0; i<datalen; ++i) {
      data[i]="abc"[i % 3];
      datastr.push_back(data[i]);
    }
    for (std::string pattern : {"", "a","b","c","ab","bc","ca","abc","bca","cab","abca","bcab","cabc","abcab","bcabc","cabcab","abcabc","bcabca","cabcab"}) {
      for (int begin = 0; begin < datalen; ++begin) {
	for (int end = 0; end <= datalen; ++end) {
	  std::size_t expect = std::string::npos;
	  std::size_t at = begin;
	  do {
	    if (matches(pattern,at,data,begin,end)) {
	      expect = at;
	      break;
	    }
	    ++at;
	  } while (at < end);

	  std::size_t result = find(pattern,data,begin,end);
	  if (result != expect) {
	    std::cout << "find(" << pattern << "," << datastr << "," << begin << "," << end << ") = " << result << " != " << expect << std::endl;
	    assert(false);
	  }

	}
      }
    }
  }
  return true;
}

int main() {
  testreadfile() && std::cout << "readfile ok" << std::endl;
  testmatches() && std::cout << "matches ok" << std::endl;
  testfind() && std::cout << "find ok" << std::endl;

  return 0;
}
