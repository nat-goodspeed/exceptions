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
    template <typename... ARGS>
    void msg(ARGS&&... args)
    {
        ((std::cout << indent(n) << "~Thrower(" << n << ") ") << ... << args) << std::endl;
    }
    ~Thrower()
    {
        msg("enter");
        try
        {
            if (n < 5)
            {
                Thrower th(n+1);
                msg("uncaught_exceptions() = ", std::uncaught_exceptions());
                throw Bad(n);
            }
        }
        catch (const Bad& bad)
        {
            msg("caught ", bad.what());
        }
        msg("done");
    }
    int n;
};


int main(int argc, char *argv[])
{
    Thrower th(0);
    return 0;
}
