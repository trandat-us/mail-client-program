#ifndef UTILITY_H
#define UTILITY_H

#include <memory>
#include <array>

char* exec(const char* cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    char* rs = new char[result.length() - 1];
    for(int i = 0; i < result.length() - 1; i++)
      rs[i] = result[i];
    return rs;
}

void error(const char *msg){
  perror(msg);
  exit(1);
}

#endif