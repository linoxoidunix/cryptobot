#include <fstream>
#include <iostream>
#include <regex>

class A{
    public:
    A(){
        int y = 0;
    };
    int a = 6;
};

class B : public A{
    int g = 5;
    public:
    B():A(){
        int z = 0;
    };
}   ;


int main(int argc, char** argv){
    B b;
    std::cout << "asdasdasdasd" << "\n"; 
    int x = 10;
    auto dfvdffgg = std::string(argv[0]);
    std::cout << std::string(dfvdffgg) << "\n";
    return 0;
}