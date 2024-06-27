#include "fiber_context"
#include <iostream>
#include <stack>
#include <stdexcept>

struct Bad: public std::runtime_error {
    Bad(): std::runtime_error("Bad") {}
};

struct Worse: public std::runtime_error {
    Worse(): std::runtime_error("Worse") {}
};

struct Context
{
    Context(const std::string& name): name(name) {}
    std::string name;
    std::stack<std::exception_ptr> current_exceptions;

    fiber_context resume(fiber_context&& tofiber, Context& tocontext);
    fiber_context rethrows(fiber_context&& tofiber);
};

// This illustrates a possible way to implement fiber-local exception
// information. Pretend
// fiber_context resume(fiber_context&& tofiber, Context& tocontext);
// is instead
// fiber_context fiber_context::resume() &&;
// where the Context location for each fiber is known to the fiber_context
// library.
fiber_context Context::resume(fiber_context&& tofiber, Context& tocontext)
{
    // save each std::current_exception() for the calling fiber
    while (auto current = std::current_exception())
    {
        current_exceptions.push(current);
        try
        {
            std::rethrow_exception(current);
        }
        catch (const std::exception& exc) // (...)
        {
            // cancel this std::current_exception()
            std::cout << name << "::resume() canceling " << exc.what() << std::endl;
        }
    }
    std::cout << name << "::resume() saved " << current_exceptions.size() << " exceptions" << std::endl;

    // how we resume tofiber depends on tocontext.current_exceptions
    return tocontext.rethrows(std::move(tofiber));
}

fiber_context Context::rethrows(fiber_context&& tofiber)
{
    if (current_exceptions.empty())
    {
        // no (more) exceptions in the exception stack
        std::cout << name << "::rethrows() resuming target" << std::endl;
        return std::move(tofiber).resume();
    }

    // here there's at least one more exception on the current_exceptions stack
    std::cout << name << "::rethrows() has " << current_exceptions.size() << " more exceptions" << std::endl;
    try
    {
        auto next = std::move(current_exceptions.top());
        current_exceptions.pop();
        std::rethrow_exception(next);
    }
    catch (const std::exception& exc) // (...)
    {
        // switch context from within this catch block
        std::cout << name << "::rethrows() throwing " << exc.what() << std::endl;
        return rethrows(std::move(tofiber));
    }
}

int main(void) {
    Context fiberA_context("main"), fiberB_context("fiberB");
    // 0. fiberB is prepared but not yet resumed
    fiber_context fiberB{
        [&fiberA_context, &fiberB_context]
        (fiber_context &&fiberA) {
        try {
            // 3. fiberB throws Worse
            throw Worse();
        } catch (const std::exception& caught) {
            // 4. fiberB catches Worse, resumes default fiber
            //fiberA = std::move(fiberA).resume();
            fiberA = fiberB_context.resume(std::move(fiberA), fiberA_context);
        }
        // 8. fiberB terminates by resuming default fiber
        return std::move(fiberA);
    }};
    std::string thrown{ "Nothing" };
    try {
        try {
            Bad myBad;
            thrown = myBad.what();
            // 1. default fiber throws Bad
            throw myBad;
        } catch (const std::exception& caught) {
            // 2. default fiber catches Bad, enters fiberB
            //fiberB = std::move(fiberB).resume();
            fiberB = fiberA_context.resume(std::move(fiberB), fiberB_context);
            // 5. the currently handled exception is...?
            throw;
        }
    } catch (const std::exception& caught) {
        // 6. which exception is caught?
        std::cout << "Situation went from " << thrown << " to " << caught.what()
                  << std::endl;
    }
    // 7. default fiber resumes fiberB to let it terminate
    //fiberB = std::move(fiberB).resume();
    fiberB = fiberA_context.resume(std::move(fiberB), fiberB_context);
}
