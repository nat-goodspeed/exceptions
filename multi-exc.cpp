#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <stdexcept>

template <typename... ARGS>
std::string stringize(ARGS&&... args)
{
    std::ostringstream out;
    (out << ... << args);
    return out.str();
}

std::string indent(int n)
{
    std::ostringstream out;
    for (int i = 0; i < n; ++i)
        out << "  ";
    return out.str();
}

struct Bad: public std::runtime_error
{
    Bad(int n): std::runtime_error(stringize("Bad(", n, ")")) {}
};

struct Thrower
{
    Thrower(int n): n(n)
    {}
    ~Thrower()
    {
        std::cout << indent(n) <<  "~Thrower(" << n << ") enter" << std::endl;
        try
        {
            if (n < 5)
            {
                Thrower th(n+1);
                std::cout << indent(n)
                          << "~Thrower(" << n << ") uncaught_exceptions() = "
                          << std::uncaught_exceptions() << std::endl;
                throw Bad(n);
            }
        }
        catch (const Bad& bad)
        {
            std::cout << indent(n)
                      << "~Thrower(" << n << ") caught " << bad.what() << std::endl;
        }
        std::cout << indent(n) << "~Thrower(" << n << ") done" << std::endl;
    }
    int n;
};


int main(int argc, char *argv[])
{
    Thrower th(0);
    return 0;
}
