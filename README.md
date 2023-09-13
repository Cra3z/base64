# base64
A lightweight header-only library about base64 for C++(Requires C++11 or newer standards)  
一个轻量级的base64的实现(要求C++11或更新的标准)
 
## 使用方式 | Usage

```c++
#include "base64.hpp"
#include <iostream>

auto main() ->int {
  // 编码 | Encode
  std::string s{"hello world"};
  std::string res1 = base64::encode(s);
  std::string res2 = base64::encode(s.begin(), s.end());
  std::string res3{};
  base64::encode_to(s, std::back_inserter(res3));
  std::cout << res1 << '\n';
  std::cout << res2 << '\n';
  std::cout << res3 << '\n';
  // 解码 | Decode
  std::string buff{};
  base64::decode_to(res1, std::back_inserter(buff));
  std::cout << buff << '\n';
  return 0;
}

```

output:  
```c++
aGVsbG8gd29ybGQ=
aGVsbG8gd29ybGQ=
aGVsbG8gd29ybGQ=
hello world
```
