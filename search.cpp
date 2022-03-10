#include <fstream>
#include <string>
#include <stdint.h>
#include <cassert>
#include <vector>
#include <iostream>

bool readfile(const std::string &file, std::vector<uint8_t> &data) {
  std::ifstream in(file,std::ios::binary);
  if (!in.seekg (0, std::ios::end)) return false;
  ssize_t size = in.tellg();
  data.resize(size);
  if (!in.seekg (0, std::ios::beg)) return false;
  if (!in.read ((char*)&data[0],size)) return false;

  return true;
}

//
// data restricted to [begin,end) matches pattern at [at,at+pattern.length())
//
bool matches(const std::string &pattern,ssize_t at,
	     const std::vector<uint8_t> &data,
	     ssize_t begin, ssize_t end)
{
  ssize_t n=pattern.length();
  return (begin <= at && at+n <= end) && 
    pattern.compare(0,n,(char*)&data[at],n) == 0;
}

//
// find first occurance of pattern in data restricted to [begin,end)
//
ssize_t find1(const std::string &pattern,
	    const std::vector<uint8_t> &data,
	    ssize_t begin, ssize_t end)
{
  if (end-begin < pattern.length()) {
    return ssize_t(std::string::npos);
  }

  if (pattern.length() == 0) {
    return begin;
  }
  
  ssize_t last[256] = { ssize_t(std::string::npos) };
  ssize_t prev[pattern.length()];
  for (ssize_t i=0; i<pattern.length(); ++i) {
    last[pattern[i]]=i;
    prev[i]=(i > 0) ?
      pattern.rfind(pattern[i],i-1)
      : std::string::npos;
  }

  for (ssize_t i=begin; i < end; i += pattern.size()) {
    for (ssize_t at = last[data[i]];
	 at != std::string::npos;
	 at = prev[at]) {
      if (matches(pattern,i-at,data,begin,end)) {
	return i - at;
      }
    }
  }

  return std::string::npos;
}


struct Finder {
  //
  // recursive/iteratative unified struct
  //

  static const ssize_t npos = ssize_t(std::string::npos);
  const std::string &pattern;
  const int n;
  const std::vector<uint8_t> &data;
  std::vector<ssize_t> last;
  std::vector<ssize_t> prev;

  Finder(const std::string &_pattern,
       const std::vector<uint8_t> &_data)
    :
  pattern(_pattern),
  n(pattern.length()),
  data(_data),
  last(256,ssize_t(std::string::npos)),
  prev(pattern.length(),ssize_t(std::string::npos))
  {
    for (ssize_t i=0; i<n; ++i) {
      last[pattern[i]]=i;
      prev[i]=(i > 0) ?
	ssize_t(pattern.rfind(pattern[i],i-1))
	: npos;
    }
  }

  ssize_t search(ssize_t begin, ssize_t end, ssize_t pos) {
    for (ssize_t at = last[data[pos]]; at != npos; at = prev[at]) {
      if (matches(pattern,pos-at,data,begin,end)) {
	return pos - at;
      }
    }
    return npos;
  }

  ssize_t iterate(ssize_t begin, ssize_t end) {
    ssize_t ans = npos;

    for (ssize_t i=begin+(n-1); i < end; i += n) {
      ans = search(begin,end,i);
      if (ans != npos) break;
    }

    return ans;
  }

  ssize_t recurse(ssize_t begin, ssize_t end) {
    if (end-begin < n) {
      return npos;
    }
    ssize_t pos = (begin+end)/2;
    size_t ans = recurse(begin,pos);
    if (ans != npos) return ans;
    ans = search(begin,end,pos);
    if (ans != npos) return ans;    
    return recurse(pos+1,end);
  }
};
  
ssize_t find2(const std::string &pattern,
	     const std::vector<uint8_t> &data,
	     ssize_t begin, ssize_t end,
	     bool is_recursive=false) {
  if (end-begin < pattern.length()) {
    return Finder::npos;
  }
  if (pattern.length() == 0) {
    return begin;
  }
  Finder finder(pattern,data);
  return is_recursive ? finder.recurse(begin,end) : finder.iterate(begin,end);
}

ssize_t find(const std::string &pattern,
	     const std::vector<uint8_t> &data,
	     ssize_t begin, ssize_t end)
{
  ssize_t res1 = find1(pattern,data,begin,end);
  ssize_t res2rec = find2(pattern,data,begin,end,true);
  ssize_t res2itr = find2(pattern,data,begin,end,false);
  assert(res1 == res2itr);
  assert(res1 == res2rec);
  return res1;
}

struct lastly {
  void (*fini)();
  lastly(void (*_fini)()) : fini(_fini) {}
  ~lastly() { fini(); }
};

bool testreadfile() {
  lastly cleanup([] { remove("sample.dat"); });

  std::vector<uint8_t> data;
  for (ssize_t sz = 0; sz < 100'000; sz += (sz < 10) ? 1 : 997) {
    {
      std::ofstream sample("sample.dat",std::ios::binary);
      for (ssize_t i = 0; i<sz; ++i) {
	sample.put((sz-i) % 256);
      }
    }
    {
      assert(readfile("sample.dat", data));
      assert(data.size() == sz);
      for (ssize_t i = 0; i<sz; ++i) {
	assert(data[i]==((sz-i) % 256));
      }
    }
  }
  return true;
}

bool testcompare() {
  std::string abc="abc";
  const char *c123="123";
  for (int i=0; i<=3; ++i) {
    for (int j=0; j<=3; ++j) {
      assert (abc.compare(i,0,c123+j,0) == 0);
    }
  }
  return true;
}

const std::vector < std::string > patterns(
{
  "",
  "a","b","c",
  "ab","bc","ca",
  "abc","bca","cab",
  "abca","bcab","cabc",
  "abcab","bcabc","cabcab",
  "abcabc","bcabca","cabcab"
});

bool testmatches()
{
  for (int datalen=0; datalen <= 10; ++datalen) {
    std::vector<uint8_t> data(datalen,0);
    std::string datastr;
    assert(data.size()==datalen);
    for (int i=0; i<datalen; ++i) {
      data[i]="abc"[i % 3];
      datastr.push_back(data[i]);
    }
    for (auto pattern : patterns) {
      for (int begin = 0; begin <= datalen; ++begin) {
	for (int end = begin; end <= datalen; ++end) {
	  for (int at = -2; at <= datalen+2; ++at) {
	    bool expect = (begin <= at && at + pattern.length() <= end);
	    expect = expect && datastr.substr(at,pattern.length()) == pattern;
	    bool result = matches(pattern,at,data,begin,end);
	    if (result != expect) {
	      std::cout << "matches(" << pattern << "," << at << "," << datastr << "," << begin << "," << end << ") = " << result << " != " << expect << std::endl;
	      assert(false);
	    }
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
    for (auto pattern : patterns) {
      for (int begin = 0; begin <= datalen; ++begin) {
	for (int end = begin; end <= datalen; ++end) {
	  ssize_t expect = datastr.substr(begin,end-begin).find(pattern);
	  if (expect != ssize_t(std::string::npos)) {
	    expect += begin;
	  }
	  ssize_t result = find(pattern,data,begin,end);
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
  testcompare() && std::cout << "compare ok" << std::endl;
  testmatches() && std::cout << "matches ok" << std::endl;
  testfind() && std::cout << "find ok" << std::endl;

  return 0;
}
