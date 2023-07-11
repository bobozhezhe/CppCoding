#include <iostream>

int main()
{
    int res = 0;
    // for(int i=50; i<=100; i++)
        // res += i;

    int i = 50;
    while(i <= 100){
        res += i++;
    }
    std::cout << res << std::endl;

    for(int i = 10; i >= 0; i--)
        std::cout << i << std::endl;

    return 0;
}
