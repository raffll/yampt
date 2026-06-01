#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main(int argc, char * argv[])
{
    Catch::Session session;

    if (argc == 1)
    {
        const char * default_argv[] = { argv[0], "[u]" };
        return session.run(2, default_argv);
    }

    return session.run(argc, argv);
}
