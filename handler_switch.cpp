#include "fiber_context"
#include <iostream>
#include <stdexcept>

struct Bad: public std::runtime_error {
    Bad(): std::runtime_error("Bad") {}
};

struct Worse: public std::runtime_error {
    Worse(): std::runtime_error("Worse") {}
};

struct Context
{
    std::exception_ptr current_exception;

    fiber_context resume(fiber_context&& tofiber, Context& tocontext);
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
    // save std::current_exception() for the calling fiber
    auto current = std::current_exception();
    current_exception = current;
    if (current)
    {
        try
        {
            std::rethrow_exception(current);
        }
        catch (...)
        {
            // cancel std::current_exception()
        }
    }

    // how we resume tofiber depends on tocontext.current_exception
    if (! tocontext.current_exception)
    {
        // just resume tofiber with no std::current_exception()
        return std::move(tofiber).resume();
    }
    else
    {
        // there's a current_exception
        try
        {
            std::rethrow_exception(tocontext.current_exception);
        }
        catch (...)
        {
            return std::move(tofiber).resume();
        }
    }
}

int main(void) {
    Context fiberA_context, fiberB_context;
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
