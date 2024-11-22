#include <iostream>
#include <Windows.h>


int dummyValue = 123456; 
int main() {
   
    while (true) {
        std::cout << "Dummy process started. PID: " << GetCurrentProcessId() << std::endl;
        std::cout << "Dummy value: " << dummyValue << std::endl;
        std::cout << "Dummy value address: " << &dummyValue << std::endl;
        std::cin.get();
    }

    return 0;
}